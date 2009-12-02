/*	$NetBSD: clvmd-gulm.c,v 1.1.1.2 2009/12/02 00:27:02 haad Exp $	*/

/*
 * Copyright (C) 2002-2003 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This provides the interface between clvmd and gulm as the cluster
 * and lock manager.
 *
 * It also provides the "liblm" functions too as it's hard (and pointless)
 * to seperate them out when using gulm.
 *
 * What it does /not/ provide is the communications between clvmd daemons
 * on the cluster nodes. That is done in tcp-comms.c
 */

#include <pthread.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <utmpx.h>
#include <syslog.h>
#include <assert.h>
#include <libdevmapper.h>
#include <ccs.h>
#include <libgulm.h>

#include "locking.h"
#include "lvm-logging.h"
#include "clvm.h"
#include "clvmd-comms.h"
#include "lvm-functions.h"
#include "clvmd.h"
#include "clvmd-gulm.h"

/* Hash list of nodes in the cluster */
static struct dm_hash_table *node_hash;

/* hash list of outstanding lock requests */
static struct dm_hash_table *lock_hash;

/* Copy of the current quorate state */
static uint8_t gulm_quorate = 0;
static enum {INIT_NOTDONE, INIT_DONE, INIT_WAITQUORATE} init_state = INIT_NOTDONE;

/* Number of active nodes */
static int num_nodes;

static char *cluster_name;
static int in_shutdown = 0;

static pthread_mutex_t lock_start_mutex;
static volatile int lock_start_flag;

struct node_info
{
    enum {NODE_UNKNOWN, NODE_DOWN, NODE_UP, NODE_CLVMD} state;
    char name[GULM_MAX_CLUSTER_MEMBER_NAME_LEN];
};

struct lock_wait
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int status;
};

/* Forward */
static int read_from_core_sock(struct local_client *client, char *buf, int len, const char *csid,
			       struct local_client **new_client);
static int read_from_lock_sock(struct local_client *client, char *buf, int len, const char *csid,
			       struct local_client **new_client);
static int get_all_cluster_nodes(void);
static int _csid_from_name(char *csid, const char *name);
static void _cluster_closedown(void);

/* In tcp-comms.c */
extern struct dm_hash_table *sock_hash;

static int add_internal_client(int fd, fd_callback_t callback)
{
    struct local_client *client;

    DEBUGLOG("Add_internal_client, fd = %d\n", fd);

    /* Add a GULM file descriptor it to the main loop */
    client = malloc(sizeof(struct local_client));
    if (!client)
    {
	DEBUGLOG("malloc failed\n");
	return -1;
    }

    memset(client, 0, sizeof(struct local_client));
    client->fd = fd;
    client->type = CLUSTER_INTERNAL;
    client->callback = callback;
    add_client(client);

    /* Set Close-on-exec */
    fcntl(fd, F_SETFD, 1);

    return 0;
}

/* Gulm library handle */
static gulm_interface_p gulm_if;
static lg_core_callbacks_t core_callbacks;
static lg_lockspace_callbacks_t lock_callbacks;

static void badsig_handler(int sig)
{
    DEBUGLOG("got sig %d\n", sig);
    _cluster_closedown();
    exit(0);
}

static void _reread_config(void)
{
        /* Re-read CCS node list */
	DEBUGLOG("Re-reading CCS config\n");
	get_all_cluster_nodes();
}

static int _init_cluster(void)
{
    int status;
    int ccs_h;
    int port = 0;
    char *portstr;

    /* Get cluster name from CCS */
    ccs_h = ccs_force_connect(NULL, 0);
    if (ccs_h < 0)
    {
	syslog(LOG_ERR, "Cannot login in to CCSD server\n");
	return -1;
    }

    ccs_get(ccs_h, "//cluster/@name", &cluster_name);
    DEBUGLOG("got cluster name %s\n", cluster_name);

    if (!ccs_get(ccs_h, "//cluster/clvm/@port", &portstr))
    {
	port = atoi(portstr);
	free(portstr);
	DEBUGLOG("got port number %d\n", port);

	if (port <= 0 && port >= 65536)
	    port = 0;
    }

    ccs_disconnect(ccs_h);

    /* Block locking until we are logged in */
    pthread_mutex_init(&lock_start_mutex, NULL);
    pthread_mutex_lock(&lock_start_mutex);
    lock_start_flag = 1;

    node_hash = dm_hash_create(100);
    lock_hash = dm_hash_create(10);

    /* Get all nodes from CCS */
    if (get_all_cluster_nodes())
	return -1;

    /* Initialise GULM library */
    status = lg_initialize(&gulm_if, cluster_name, "clvmd");
    if (status)
    {
	DEBUGLOG("lg_initialize failed: %d\n", status);
	return status;
    }

    /* Connect to core - we are not "important" :-) */
    status = lg_core_login(gulm_if, 0);
    if (status)
    {
	DEBUGLOG("lg_core_login failed: %d\n", status);
	return status;
    }

    /* Initialise the inter-node comms */
    status = init_comms(port);
    if (status)
	return status;

    /* Add core FD to the list */
    status = add_internal_client(lg_core_selector(gulm_if), read_from_core_sock);
    if (status)
    {
	DEBUGLOG("can't allocate client space\n");
	return status;
    }

    /* Connect to the lock server */
    if (lg_lock_login(gulm_if, "CLVM"))
    {
	syslog(LOG_ERR, "Cannot login in to LOCK server\n");
	DEBUGLOG("Cannot login in to LOCK server\n");
	exit(88);
    }

    /* Add lockspace FD to the list */
    status = add_internal_client(lg_lock_selector(gulm_if), read_from_lock_sock);
    if (status)
    {
	DEBUGLOG("can't allocate client space\n");
	exit(status);
    }

    /* Request a list of nodes, we can't really do anything until
       this comes back */
    status = lg_core_nodelist(gulm_if);
    if (status)
    {
	DEBUGLOG("lg_core_nodelist failed: %d\n", status);
	return status;
    }

    /* So I can kill it without taking GULM down too */
    signal(SIGINT, badsig_handler);
    signal(SIGTERM, badsig_handler);

    return 0;
}

static void _cluster_closedown(void)
{
    DEBUGLOG("cluster_closedown\n");
    in_shutdown = 1;
    destroy_lvhash();
    lg_lock_logout(gulm_if);
    lg_core_logout(gulm_if);
    lg_release(gulm_if);
}

/* Expire locks for a named node, or us */
#define GIO_KEY_SIZE 46
static void drop_expired_locks(char *nodename)
{
    struct utsname nodeinfo;
    uint8_t mask[GIO_KEY_SIZE];

    DEBUGLOG("Dropping expired locks for %s\n", nodename?nodename:"(null)");
    memset(mask, 0xff, GIO_KEY_SIZE);

    if (!nodename)
    {
	uname(&nodeinfo);
	nodename = nodeinfo.nodename;
    }

    if (lg_lock_drop_exp(gulm_if, nodename, mask, GIO_KEY_SIZE))
    {
	DEBUGLOG("Error calling lg_lock_drop_exp()\n");
    }
}


static int read_from_core_sock(struct local_client *client, char *buf, int len, const char *csid,
			       struct local_client **new_client)
{
    int status;

    *new_client = NULL;
    status = lg_core_handle_messages(gulm_if, &core_callbacks, NULL);
    return status<0 ? status : 1;
}

static int read_from_lock_sock(struct local_client *client, char *buf, int len, const char *csid,
			       struct local_client **new_client)
{
    int status;

    *new_client = NULL;
    status = lg_lock_handle_messages(gulm_if, &lock_callbacks, NULL);
    return status<0 ? status : 1;
}


/* CORE callback routines */
static int core_login_reply(void *misc, uint64_t gen, uint32_t error, uint32_t rank, uint8_t corestate)
{
   DEBUGLOG("CORE Got a Login reply.  gen:%lld err:%d rank:%d corestate:%d\n",
         gen, error, rank, corestate);

   if (error)
       exit(error);

   /* Get the current core state (for quorum) */
   lg_core_corestate(gulm_if);

   return 0;
}

static void set_node_state(struct node_info *ninfo, char *csid, uint8_t nodestate)
{
    if (nodestate == lg_core_Logged_in)
    {
	/* Don't clobber NODE_CLVMD state */
	if (ninfo->state != NODE_CLVMD)
	{
	    if (ninfo->state == NODE_UNKNOWN ||
		ninfo->state == NODE_DOWN)
		num_nodes++;

	    ninfo->state = NODE_UP;
	}
    }
    else
    {
	if (nodestate == lg_core_Expired ||
	    nodestate == lg_core_Fenced ||
	    nodestate == lg_core_Logged_out)
	{
	    if (ninfo->state != NODE_DOWN)
		num_nodes--;
	    ninfo->state = NODE_DOWN;
	}
    }
    /* Gulm doesn't always send node DOWN events, so even if this a a node UP we must
     * assume (ahem) that it prevously went down at some time. So we close
     * the sockets here to make sure that we don't have any dead connections
     * to that node.
     */
    tcp_remove_client(csid);

    DEBUGLOG("set_node_state, '%s' state = %d num_nodes=%d\n",
	     ninfo->name, ninfo->state, num_nodes);
}

static struct node_info *add_or_set_node(char *name, struct in6_addr *ip, uint8_t state)
{
    struct node_info *ninfo;

    ninfo = dm_hash_lookup_binary(node_hash, (char *)ip, GULM_MAX_CSID_LEN);
    if (!ninfo)
    {
	/* If we can't find that node then re-read the config file in case it
	   was added after we were started */
	DEBUGLOG("Node %s not found, re-reading config file\n", name);
	get_all_cluster_nodes();

	/* Now try again */
	ninfo = dm_hash_lookup_binary(node_hash, (char *)ip, GULM_MAX_CSID_LEN);
	if (!ninfo)
	{
	    DEBUGLOG("Ignoring node %s, not part of the SAN cluster\n", name);
	    return NULL;
	}
    }

    set_node_state(ninfo, (char *)ip, state);

    return ninfo;
}

static void _get_our_csid(char *csid)
{
	get_our_gulm_csid(csid);
}

static int core_nodelist(void *misc, lglcb_t type, char *name, struct in6_addr *ip, uint8_t state)
{
    DEBUGLOG("CORE nodelist\n");

    if (type == lglcb_start)
    {
	DEBUGLOG("Got Nodelist, start\n");
    }
    else
    {
	if (type == lglcb_item)
	{
	    DEBUGLOG("Got nodelist, item: %s, %#x\n", name, state);

	    add_or_set_node(name, ip, state);
	}
	else
	{
	    if (type == lglcb_stop)
	    {
		char ourcsid[GULM_MAX_CSID_LEN];

		DEBUGLOG("Got Nodelist, stop\n");
		if (gulm_quorate)
		{
			clvmd_cluster_init_completed();
			init_state = INIT_DONE;
		}
		else
		{
			if (init_state == INIT_NOTDONE)
				init_state = INIT_WAITQUORATE;
		}

		/* Mark ourself as up */
		_get_our_csid(ourcsid);
		gulm_add_up_node(ourcsid);
	    }
	    else
	    {
		DEBUGLOG("Unknown lglcb_t %#x\n", type);
	    }
	}
    }

    return 0;
}

static int core_statechange(void *misc, uint8_t corestate, uint8_t quorate, struct in6_addr *masterip, char *mastername)
{
    DEBUGLOG("CORE Got statechange. quorate:%d, corestate:%x mastername:%s\n",
	     quorate, corestate, mastername);

    gulm_quorate = quorate;
    if (quorate && init_state == INIT_WAITQUORATE)
    {
	    clvmd_cluster_init_completed();
	    init_state = INIT_DONE;
    }
    return 0;
}

static int core_nodechange(void *misc, char *nodename, struct in6_addr *nodeip, uint8_t nodestate)
{
    struct node_info *ninfo;

    DEBUGLOG("CORE node change, name=%s, state = %d\n", nodename, nodestate);

    /* If we don't get nodeip here, try a lookup by name */
    if (!nodeip)
	_csid_from_name((char *)nodeip, nodename);
    if (!nodeip)
	return 0;

    ninfo = add_or_set_node(nodename, nodeip, nodestate);
    if (!ninfo)
	return 0;

    /* Check if we need to drop any expired locks */
    if (ninfo->state == NODE_DOWN)
    {
	drop_expired_locks(nodename);
    }

    return 0;
}
static int core_error(void *misc, uint32_t err)
{
    DEBUGLOG("CORE error: %d\n", err);
    // Not sure what happens here
    return 0;
}

/* LOCK callback routines */
static int lock_login_reply(void *misc, uint32_t error, uint8_t which)
{
    DEBUGLOG("LOCK Got a Login reply.  err:%d which:%d\n",
	     error, which);

    if (error)
	exit(error);

    /* Drop any expired locks for us that might be hanging around */
    drop_expired_locks(NULL);

    /* Enable locking operations in other threads */
    if (lock_start_flag)
    {
	lock_start_flag = 0;
	pthread_mutex_unlock(&lock_start_mutex);
    }

    return 0;
}

static int lock_lock_state(void *misc, uint8_t *key, uint16_t keylen,
			   uint64_t subid, uint64_t start, uint64_t stop,
			   uint8_t state, uint32_t flags, uint32_t error,
			   uint8_t *LVB, uint16_t LVBlen)
{
    struct lock_wait *lwait;

    DEBUGLOG("LOCK lock state: %s, error = %d\n", key, error);

    /* No waiting process to wake up when we are shutting down */
    if (in_shutdown)
	    return 0;

    lwait = dm_hash_lookup(lock_hash, key);
    if (!lwait)
    {
	DEBUGLOG("Can't find hash entry for resource %s\n", key);
	return 0;
    }
    lwait->status = error;
    pthread_mutex_lock(&lwait->mutex);
    pthread_cond_signal(&lwait->cond);
    pthread_mutex_unlock(&lwait->mutex);

    return 0;
}
static int lock_error(void *misc, uint32_t err)
{
    DEBUGLOG("LOCK error: %d\n", err);
    // Not sure what happens here
    return 0;
}


/* CORE callbacks */
static lg_core_callbacks_t core_callbacks = {
    .login_reply  = core_login_reply,
    .nodelist     = core_nodelist,
    .statechange  = core_statechange,
    .nodechange   = core_nodechange,
    .error        = core_error,
};

/* LOCK callbacks */
static lg_lockspace_callbacks_t lock_callbacks = {
    .login_reply   = lock_login_reply,
    .lock_state    = lock_lock_state,
    .error         = lock_error,
};

/* Allow tcp-comms to loop round the list of active nodes */
int get_next_node_csid(void **context, char *csid)
{
    struct node_info *ninfo = NULL;

    /* First node */
    if (!*context)
    {
	*context = dm_hash_get_first(node_hash);
    }
    else
    {
	*context = dm_hash_get_next(node_hash, *context);
    }
    if (*context)
	ninfo = dm_hash_get_data(node_hash, *context);

    /* Find a node that is UP */
    while (*context && ninfo->state == NODE_DOWN)
    {
	*context = dm_hash_get_next(node_hash, *context);
	if (*context)
	{
	    ninfo = dm_hash_get_data(node_hash, *context);
	}
    }

    if (!*context || ninfo->state == NODE_DOWN)
    {
	return 0;
    }

    memcpy(csid, dm_hash_get_key(node_hash, *context), GULM_MAX_CSID_LEN);
    return 1;
}

int gulm_name_from_csid(const char *csid, char *name)
{
    struct node_info *ninfo;

    ninfo = dm_hash_lookup_binary(node_hash, csid, GULM_MAX_CSID_LEN);
    if (!ninfo)
    {
        sprintf(name, "UNKNOWN %s", print_csid(csid));
	return -1;
    }

    strcpy(name, ninfo->name);
    return 0;
}


static int _csid_from_name(char *csid, const char *name)
{
    struct dm_hash_node *hn;
    struct node_info *ninfo;

    dm_hash_iterate(hn, node_hash)
    {
	ninfo = dm_hash_get_data(node_hash, hn);
	if (strcmp(ninfo->name, name) == 0)
	{
	    memcpy(csid, dm_hash_get_key(node_hash, hn), GULM_MAX_CSID_LEN);
	    return 0;
	}
    }
    return -1;
}

static int _get_num_nodes()
{
    DEBUGLOG("num_nodes = %d\n", num_nodes);
    return num_nodes;
}

/* Node is now known to be running a clvmd */
void gulm_add_up_node(const char *csid)
{
    struct node_info *ninfo;

    ninfo = dm_hash_lookup_binary(node_hash, csid, GULM_MAX_CSID_LEN);
    if (!ninfo) {
	    DEBUGLOG("gulm_add_up_node no node_hash entry for csid %s\n", print_csid(csid));
	return;
    }

    DEBUGLOG("gulm_add_up_node %s\n", ninfo->name);

    if (ninfo->state == NODE_DOWN)
	    num_nodes++;
    ninfo->state = NODE_CLVMD;

    return;

}
/* Node is now known to be NOT running a clvmd */
void add_down_node(char *csid)
{
    struct node_info *ninfo;

    ninfo = dm_hash_lookup_binary(node_hash, csid, GULM_MAX_CSID_LEN);
    if (!ninfo)
	return;

    /* Only set it to UP if it was previously known to be
       running clvmd - gulm may set it DOWN quite soon */
    if (ninfo->state == NODE_CLVMD)
	ninfo->state = NODE_UP;
    drop_expired_locks(ninfo->name);
    return;

}

/* Call a callback for each node, so the caller knows whether it's up or down */
static int _cluster_do_node_callback(struct local_client *master_client,
				     void (*callback)(struct local_client *, const char *csid, int node_up))
{
    struct dm_hash_node *hn;
    struct node_info *ninfo;
    int somedown = 0;

    dm_hash_iterate(hn, node_hash)
    {
	char csid[GULM_MAX_CSID_LEN];
	struct local_client *client;

	ninfo = dm_hash_get_data(node_hash, hn);
	memcpy(csid, dm_hash_get_key(node_hash, hn), GULM_MAX_CSID_LEN);

	DEBUGLOG("down_callback. node %s, state = %d\n", ninfo->name, ninfo->state);

	client = dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN);
	if (!client)
	{
	    /* If it's up but not connected, try to make contact */
	    if (ninfo->state == NODE_UP)
		    gulm_connect_csid(csid, &client);

	    client = dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN);

	}
	DEBUGLOG("down_callback2. node %s, state = %d\n", ninfo->name, ninfo->state);
	if (ninfo->state != NODE_DOWN)
		callback(master_client, csid, ninfo->state == NODE_CLVMD);

	if (ninfo->state != NODE_CLVMD)
		somedown = -1;
    }
    return somedown;
}

/* Convert gulm error codes to unix errno numbers */
static int gulm_to_errno(int gulm_ret)
{
    switch (gulm_ret)
    {
    case lg_err_TryFailed:
    case lg_err_AlreadyPend:
	    errno = EAGAIN;
	    break;

	/* More?? */
    default:
	    errno = EINVAL;
    }

    return gulm_ret ? -1 : 0;
}

/* Real locking */
static int _lock_resource(char *resource, int mode, int flags, int *lockid)
{
    int status;
    struct lock_wait lwait;

    /* Wait until the lock module is ready */
    if (lock_start_flag)
    {
	pthread_mutex_lock(&lock_start_mutex);
	pthread_mutex_unlock(&lock_start_mutex);
    }

    pthread_cond_init(&lwait.cond, NULL);
    pthread_mutex_init(&lwait.mutex, NULL);
    pthread_mutex_lock(&lwait.mutex);

    /* This needs to be converted from DLM/LVM2 value for GULM */
    if (flags & LKF_NOQUEUE) flags = lg_lock_flag_Try;

    dm_hash_insert(lock_hash, resource, &lwait);
    DEBUGLOG("lock_resource '%s', flags=%d, mode=%d\n", resource, flags, mode);

    status = lg_lock_state_req(gulm_if, resource, strlen(resource)+1,
			       0, 0, 0,
			       mode, flags, NULL, 0);
    if (status)
    {
	DEBUGLOG("lg_lock_state returned %d\n", status);
	return status;
    }

    /* Wait for it to complete */
    pthread_cond_wait(&lwait.cond, &lwait.mutex);
    pthread_mutex_unlock(&lwait.mutex);

    dm_hash_remove(lock_hash, resource);
    DEBUGLOG("lock-resource returning %d\n", lwait.status);

    return gulm_to_errno(lwait.status);
}


static int _unlock_resource(char *resource, int lockid)
{
    int status;
    struct lock_wait lwait;

    pthread_cond_init(&lwait.cond, NULL);
    pthread_mutex_init(&lwait.mutex, NULL);
    pthread_mutex_lock(&lwait.mutex);

    dm_hash_insert(lock_hash, resource, &lwait);

    DEBUGLOG("unlock_resource %s\n", resource);
    status = lg_lock_state_req(gulm_if, resource, strlen(resource)+1,
			       0, 0, 0,
			       lg_lock_state_Unlock, 0, NULL, 0);

    if (status)
    {
	DEBUGLOG("lg_lock_state(unlock) returned %d\n", status);
	return status;
    }

    /* When we are shutting down, don't wait for unlocks
       to be acknowledged, just do it. */
    if (in_shutdown)
	    return status;

    /* Wait for it to complete */

    pthread_cond_wait(&lwait.cond, &lwait.mutex);
    pthread_mutex_unlock(&lwait.mutex);

    dm_hash_remove(lock_hash, resource);

    return gulm_to_errno(lwait.status);
}


/* These two locking functions MUST be called in a seperate thread from
   the clvmd main loop because they expect to be woken up by it.

   These are abstractions around the real locking functions (above)
   as we need to emulate the DLM's EX/PW/CW interaction with GULM using
   two locks.
   To aid unlocking, we store the lock mode in the lockid (as GULM
   doesn't use this).
*/
static int _sync_lock(const char *resource, int mode, int flags, int *lockid)
{
    int status;
    char lock1[strlen(resource)+3];
    char lock2[strlen(resource)+3];

    snprintf(lock1, sizeof(lock1), "%s-1", resource);
    snprintf(lock2, sizeof(lock2), "%s-2", resource);

    switch (mode)
    {
    case LCK_EXCL:
	status = _lock_resource(lock1, lg_lock_state_Exclusive, flags, lockid);
	if (status)
	    goto out;

	/* If we can't get this lock too then bail out */
	status = _lock_resource(lock2, lg_lock_state_Exclusive, LCK_NONBLOCK, lockid);
        if (status == lg_err_TryFailed)
        {
           _unlock_resource(lock1, *lockid);
           status = -1;
           errno = EAGAIN;
        }
	break;

    case LCK_PREAD:
    case LCK_READ:
	status = _lock_resource(lock1, lg_lock_state_Shared, flags, lockid);
	if (status)
		goto out;
	status = _unlock_resource(lock2, *lockid);
	break;

    case LCK_WRITE:
	status = _lock_resource(lock2, lg_lock_state_Exclusive, flags, lockid);
	if (status)
		goto out;
	status = _unlock_resource(lock1, *lockid);
	break;

    default:
	status = -1;
	errno = EINVAL;
	break;
    }
 out:
    *lockid = mode;
    return status;
}

static int _sync_unlock(const char *resource, int lockid)
{
    int status = 0;
    char lock1[strlen(resource)+3];
    char lock2[strlen(resource)+3];

    snprintf(lock1, sizeof(lock1), "%s-1", resource);
    snprintf(lock2, sizeof(lock2), "%s-2", resource);

    /* The held lock mode is in the lock id */
    assert(lockid == LCK_EXCL ||
	   lockid == LCK_READ ||
	   lockid == LCK_PREAD ||
	   lockid == LCK_WRITE);

    status = _unlock_resource(lock1, lockid);
    if (!status)
	    status = _unlock_resource(lock2, lockid);

    return status;
}

static int _is_quorate()
{
	return gulm_quorate;
}

/* Get all the cluster node names & IPs from CCS and
   add them to our node list so we know who to talk to.
   Called when we start up and if we get sent SIGHUP.
*/
static int get_all_cluster_nodes()
{
    int ctree;
    char *nodename;
    int error;
    int i;

    /* Open the config file */
    ctree = ccs_force_connect(NULL, 1);
    if (ctree < 0)
    {
	log_error("Error connecting to CCS");
	return -1;
    }

    for (i=1;;i++)
    {
	char nodekey[256];
	char nodeip[GULM_MAX_CSID_LEN];
	int  clvmflag = 1;
	char *clvmflagstr;
	char key[256];

	sprintf(nodekey, "//cluster/clusternodes/clusternode[%d]/@name", i);
	error = ccs_get(ctree, nodekey, &nodename);
	if (error)
	    break;

	sprintf(key, "//cluster/clusternodes/clusternode[@name=\"%s\"]/clvm", nodename);
	if (!ccs_get(ctree, key, &clvmflagstr))
	{
	    clvmflag = atoi(clvmflagstr);
	    free(clvmflagstr);
	}

	DEBUGLOG("Got node %s from ccs(clvmflag = %d)\n", nodename, clvmflag);
	if ((get_ip_address(nodename, nodeip) == 0) && clvmflag)
	{
	    struct node_info *ninfo;

	    /* If it's not in the list, then add it */
	    ninfo = dm_hash_lookup_binary(node_hash, nodeip, GULM_MAX_CSID_LEN);
	    if (!ninfo)
	    {
		ninfo = malloc(sizeof(struct node_info));
		if (!ninfo)
		{
		    syslog(LOG_ERR, "Cannot alloc memory for node info\n");
		    ccs_disconnect(ctree);
		    return -1;
		}
		strcpy(ninfo->name, nodename);

		ninfo->state = NODE_DOWN;
		dm_hash_insert_binary(node_hash, nodeip, GULM_MAX_CSID_LEN, ninfo);
	    }
	}
	else
	{
		if (!clvmflag) {
			DEBUGLOG("node %s has clvm disabled\n", nodename);
		}
		else {
			DEBUGLOG("Cannot resolve host name %s\n", nodename);
			log_error("Cannot resolve host name %s\n", nodename);
		}
	}
	free(nodename);
    }

    /* Finished with config file */
    ccs_disconnect(ctree);

    return 0;
}

static int _get_main_cluster_fd(void)
{
	return get_main_gulm_cluster_fd();
}

static int _cluster_fd_callback(struct local_client *fd, char *buf, int len, const char *csid, struct local_client **new_client)
{
	return cluster_fd_gulm_callback(fd, buf, len, csid, new_client);
}

static int _cluster_send_message(const void *buf, int msglen, const char *csid, const char *errtext)
{
	return gulm_cluster_send_message((char *)buf, msglen, csid, errtext);
}

static int _get_cluster_name(char *buf, int buflen)
{
	strncpy(buf, cluster_name, buflen);
	return 0;
}

static struct cluster_ops _cluster_gulm_ops = {
	.cluster_init_completed   = NULL,
	.cluster_send_message     = _cluster_send_message,
	.name_from_csid           = gulm_name_from_csid,
	.csid_from_name           = _csid_from_name,
	.get_num_nodes            = _get_num_nodes,
	.cluster_fd_callback      = _cluster_fd_callback,
	.get_main_cluster_fd      = _get_main_cluster_fd,
	.cluster_do_node_callback = _cluster_do_node_callback,
	.is_quorate               = _is_quorate,
	.get_our_csid             = _get_our_csid,
	.add_up_node              = gulm_add_up_node,
	.reread_config            = _reread_config,
	.cluster_closedown        = _cluster_closedown,
	.get_cluster_name         = _get_cluster_name,
	.sync_lock                = _sync_lock,
	.sync_unlock              = _sync_unlock,
};

struct cluster_ops *init_gulm_cluster(void)
{
	if (!_init_cluster())
		return &_cluster_gulm_ops;
	else
		return NULL;
}
