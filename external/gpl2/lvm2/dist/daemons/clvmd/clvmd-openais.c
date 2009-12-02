/*	$NetBSD: clvmd-openais.c,v 1.1.1.2 2009/12/02 00:27:03 haad Exp $	*/

/*
 * Copyright (C) 2007-2009 Red Hat, Inc. All rights reserved.
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
 * This provides the interface between clvmd and OpenAIS as the cluster
 * and lock manager.
 */

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <configure.h>
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

#include <openais/saAis.h>
#include <openais/saLck.h>

#include <corosync/corotypes.h>
#include <corosync/cpg.h>

#include "locking.h"
#include "lvm-logging.h"
#include "clvm.h"
#include "clvmd-comms.h"
#include "lvm-functions.h"
#include "clvmd.h"

/* Timeout value for several openais calls */
#define TIMEOUT 10

static void openais_cpg_deliver_callback (cpg_handle_t handle,
				  const struct cpg_name *groupName,
				  uint32_t nodeid,
				  uint32_t pid,
				  void *msg,
				  size_t msg_len);
static void openais_cpg_confchg_callback(cpg_handle_t handle,
				 const struct cpg_name *groupName,
				 const struct cpg_address *member_list, size_t member_list_entries,
				 const struct cpg_address *left_list, size_t left_list_entries,
				 const struct cpg_address *joined_list, size_t joined_list_entries);

static void _cluster_closedown(void);

/* Hash list of nodes in the cluster */
static struct dm_hash_table *node_hash;

/* For associating lock IDs & resource handles */
static struct dm_hash_table *lock_hash;

/* Number of active nodes */
static int num_nodes;
static unsigned int our_nodeid;

static struct local_client *cluster_client;

/* OpenAIS handles */
static cpg_handle_t cpg_handle;
static SaLckHandleT lck_handle;

static struct cpg_name cpg_group_name;

/* Openais callback structs */
cpg_callbacks_t openais_cpg_callbacks = {
	.cpg_deliver_fn =            openais_cpg_deliver_callback,
	.cpg_confchg_fn =            openais_cpg_confchg_callback,
};

struct node_info
{
	enum {NODE_UNKNOWN, NODE_DOWN, NODE_UP, NODE_CLVMD} state;
	int nodeid;
};

struct lock_info
{
	SaLckResourceHandleT res_handle;
	SaLckLockIdT         lock_id;
	SaNameT              lock_name;
};

/* Set errno to something approximating the right value and return 0 or -1 */
static int ais_to_errno(SaAisErrorT err)
{
	switch(err)
	{
	case SA_AIS_OK:
		return 0;
        case SA_AIS_ERR_LIBRARY:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_VERSION:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_INIT:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_TIMEOUT:
		errno = ETIME;
		break;
        case SA_AIS_ERR_TRY_AGAIN:
		errno = EAGAIN;
		break;
        case SA_AIS_ERR_INVALID_PARAM:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_NO_MEMORY:
		errno = ENOMEM;
		break;
        case SA_AIS_ERR_BAD_HANDLE:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_BUSY:
		errno = EBUSY;
		break;
        case SA_AIS_ERR_ACCESS:
		errno = EPERM;
		break;
        case SA_AIS_ERR_NOT_EXIST:
		errno = ENOENT;
		break;
        case SA_AIS_ERR_NAME_TOO_LONG:
		errno = ENAMETOOLONG;
		break;
        case SA_AIS_ERR_EXIST:
		errno = EEXIST;
		break;
        case SA_AIS_ERR_NO_SPACE:
		errno = ENOSPC;
		break;
        case SA_AIS_ERR_INTERRUPT:
		errno = EINTR;
		break;
	case SA_AIS_ERR_NAME_NOT_FOUND:
		errno = ENOENT;
		break;
        case SA_AIS_ERR_NO_RESOURCES:
		errno = ENOMEM;
		break;
        case SA_AIS_ERR_NOT_SUPPORTED:
		errno = EOPNOTSUPP;
		break;
        case SA_AIS_ERR_BAD_OPERATION:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_FAILED_OPERATION:
		errno = EIO;
		break;
        case SA_AIS_ERR_MESSAGE_ERROR:
		errno = EIO;
		break;
        case SA_AIS_ERR_QUEUE_FULL:
		errno = EXFULL;
		break;
        case SA_AIS_ERR_QUEUE_NOT_AVAILABLE:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_BAD_FLAGS:
		errno = EINVAL;
		break;
        case SA_AIS_ERR_TOO_BIG:
		errno = E2BIG;
		break;
        case SA_AIS_ERR_NO_SECTIONS:
		errno = ENOMEM;
		break;
	default:
		errno = EINVAL;
		break;
	}
	return -1;
}

static char *print_openais_csid(const char *csid)
{
	static char buf[128];
	int id;

	memcpy(&id, csid, sizeof(int));
	sprintf(buf, "%d", id);
	return buf;
}

static int add_internal_client(int fd, fd_callback_t callback)
{
	struct local_client *client;

	DEBUGLOG("Add_internal_client, fd = %d\n", fd);

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

static void openais_cpg_deliver_callback (cpg_handle_t handle,
				  const struct cpg_name *groupName,
				  uint32_t nodeid,
				  uint32_t pid,
				  void *msg,
				  size_t msg_len)
{
	int target_nodeid;

	memcpy(&target_nodeid, msg, OPENAIS_CSID_LEN);

	DEBUGLOG("%u got message from nodeid %d for %d. len %d\n",
		 our_nodeid, nodeid, target_nodeid, msg_len-4);

	if (nodeid != our_nodeid)
		if (target_nodeid == our_nodeid || target_nodeid == 0)
			process_message(cluster_client, (char *)msg+OPENAIS_CSID_LEN,
					msg_len-OPENAIS_CSID_LEN, (char*)&nodeid);
}

static void openais_cpg_confchg_callback(cpg_handle_t handle,
				 const struct cpg_name *groupName,
				 const struct cpg_address *member_list, size_t member_list_entries,
				 const struct cpg_address *left_list, size_t left_list_entries,
				 const struct cpg_address *joined_list, size_t joined_list_entries)
{
	int i;
	struct node_info *ninfo;

	DEBUGLOG("confchg callback. %d joined, %d left, %d members\n",
		 joined_list_entries, left_list_entries, member_list_entries);

	for (i=0; i<joined_list_entries; i++) {
		ninfo = dm_hash_lookup_binary(node_hash,
					      (char *)&joined_list[i].nodeid,
					      OPENAIS_CSID_LEN);
		if (!ninfo) {
			ninfo = malloc(sizeof(struct node_info));
			if (!ninfo) {
				break;
			}
			else {
				ninfo->nodeid = joined_list[i].nodeid;
				dm_hash_insert_binary(node_hash,
						      (char *)&ninfo->nodeid,
						      OPENAIS_CSID_LEN, ninfo);
			}
		}
		ninfo->state = NODE_CLVMD;
	}

	for (i=0; i<left_list_entries; i++) {
		ninfo = dm_hash_lookup_binary(node_hash,
					      (char *)&left_list[i].nodeid,
					      OPENAIS_CSID_LEN);
		if (ninfo)
			ninfo->state = NODE_DOWN;
	}

	for (i=0; i<member_list_entries; i++) {
		if (member_list[i].nodeid == 0) continue;
		ninfo = dm_hash_lookup_binary(node_hash,
				(char *)&member_list[i].nodeid,
				OPENAIS_CSID_LEN);
		if (!ninfo) {
			ninfo = malloc(sizeof(struct node_info));
			if (!ninfo) {
				break;
			}
			else {
				ninfo->nodeid = member_list[i].nodeid;
				dm_hash_insert_binary(node_hash,
						(char *)&ninfo->nodeid,
						OPENAIS_CSID_LEN, ninfo);
			}
		}
		ninfo->state = NODE_CLVMD;
	}

	num_nodes = member_list_entries;
}

static int lck_dispatch(struct local_client *client, char *buf, int len,
			const char *csid, struct local_client **new_client)
{
	*new_client = NULL;
	saLckDispatch(lck_handle, SA_DISPATCH_ONE);
	return 1;
}

static int _init_cluster(void)
{
	SaAisErrorT err;
	SaVersionT  ver = { 'B', 1, 1 };
	int select_fd;

	node_hash = dm_hash_create(100);
	lock_hash = dm_hash_create(10);

	err = cpg_initialize(&cpg_handle,
			     &openais_cpg_callbacks);
	if (err != SA_AIS_OK) {
		syslog(LOG_ERR, "Cannot initialise OpenAIS CPG service: %d",
		       err);
		DEBUGLOG("Cannot initialise OpenAIS CPG service: %d", err);
		return ais_to_errno(err);
	}

	err = saLckInitialize(&lck_handle,
					NULL,
			      &ver);
	if (err != SA_AIS_OK) {
		cpg_initialize(&cpg_handle, &openais_cpg_callbacks);
		syslog(LOG_ERR, "Cannot initialise OpenAIS lock service: %d",
		       err);
		DEBUGLOG("Cannot initialise OpenAIS lock service: %d\n\n", err);
		return ais_to_errno(err);
	}

	/* Connect to the clvmd group */
	strcpy((char *)cpg_group_name.value, "clvmd");
	cpg_group_name.length = strlen((char *)cpg_group_name.value);
	err = cpg_join(cpg_handle, &cpg_group_name);
	if (err != SA_AIS_OK) {
		cpg_finalize(cpg_handle);
		saLckFinalize(lck_handle);
		syslog(LOG_ERR, "Cannot join clvmd process group");
		DEBUGLOG("Cannot join clvmd process group: %d\n", err);
		return ais_to_errno(err);
	}

	err = cpg_local_get(cpg_handle,
			    &our_nodeid);
	if (err != SA_AIS_OK) {
		cpg_finalize(cpg_handle);
		saLckFinalize(lck_handle);
		syslog(LOG_ERR, "Cannot get local node id\n");
		return ais_to_errno(err);
	}
	DEBUGLOG("Our local node id is %d\n", our_nodeid);

	saLckSelectionObjectGet(lck_handle, (SaSelectionObjectT *)&select_fd);
	add_internal_client(select_fd, lck_dispatch);

	DEBUGLOG("Connected to OpenAIS\n");

	return 0;
}

static void _cluster_closedown(void)
{
	DEBUGLOG("cluster_closedown\n");
	destroy_lvhash();

	saLckFinalize(lck_handle);
	cpg_finalize(cpg_handle);
}

static void _get_our_csid(char *csid)
{
	memcpy(csid, &our_nodeid, sizeof(int));
}

/* OpenAIS doesn't really have nmode names so we
   just use the node ID in hex instead */
static int _csid_from_name(char *csid, const char *name)
{
	int nodeid;
	struct node_info *ninfo;

	if (sscanf(name, "%x", &nodeid) == 1) {
		ninfo = dm_hash_lookup_binary(node_hash, csid, OPENAIS_CSID_LEN);
		if (ninfo)
			return nodeid;
	}
	return -1;
}

static int _name_from_csid(const char *csid, char *name)
{
	struct node_info *ninfo;

	ninfo = dm_hash_lookup_binary(node_hash, csid, OPENAIS_CSID_LEN);
	if (!ninfo)
	{
		sprintf(name, "UNKNOWN %s", print_openais_csid(csid));
		return -1;
	}

	sprintf(name, "%x", ninfo->nodeid);
	return 0;
}

static int _get_num_nodes()
{
	DEBUGLOG("num_nodes = %d\n", num_nodes);
	return num_nodes;
}

/* Node is now known to be running a clvmd */
static void _add_up_node(const char *csid)
{
	struct node_info *ninfo;

	ninfo = dm_hash_lookup_binary(node_hash, csid, OPENAIS_CSID_LEN);
	if (!ninfo) {
		DEBUGLOG("openais_add_up_node no node_hash entry for csid %s\n",
			 print_openais_csid(csid));
		return;
	}

	DEBUGLOG("openais_add_up_node %d\n", ninfo->nodeid);

	ninfo->state = NODE_CLVMD;

	return;
}

/* Call a callback for each node, so the caller knows whether it's up or down */
static int _cluster_do_node_callback(struct local_client *master_client,
				     void (*callback)(struct local_client *,
						      const char *csid, int node_up))
{
	struct dm_hash_node *hn;
	struct node_info *ninfo;
	int somedown = 0;

	dm_hash_iterate(hn, node_hash)
	{
		char csid[OPENAIS_CSID_LEN];

		ninfo = dm_hash_get_data(node_hash, hn);
		memcpy(csid, dm_hash_get_key(node_hash, hn), OPENAIS_CSID_LEN);

		DEBUGLOG("down_callback. node %d, state = %d\n", ninfo->nodeid,
			 ninfo->state);

		if (ninfo->state != NODE_DOWN)
			callback(master_client, csid, ninfo->state == NODE_CLVMD);
		if (ninfo->state != NODE_CLVMD)
			somedown = -1;
	}
	return somedown;
}

/* Real locking */
static int _lock_resource(char *resource, int mode, int flags, int *lockid)
{
	struct lock_info *linfo;
	SaLckResourceHandleT res_handle;
	SaAisErrorT err;
	SaLckLockIdT lock_id;
	SaLckLockStatusT lockStatus;

	/* This needs to be converted from DLM/LVM2 value for OpenAIS LCK */
	if (flags & LCK_NONBLOCK) flags = SA_LCK_LOCK_NO_QUEUE;

	linfo = malloc(sizeof(struct lock_info));
	if (!linfo)
		return -1;

	DEBUGLOG("lock_resource '%s', flags=%d, mode=%d\n", resource, flags, mode);

	linfo->lock_name.length = strlen(resource)+1;
	strcpy((char *)linfo->lock_name.value, resource);

	err = saLckResourceOpen(lck_handle, &linfo->lock_name,
				SA_LCK_RESOURCE_CREATE, TIMEOUT, &res_handle);
	if (err != SA_AIS_OK)
	{
		DEBUGLOG("ResourceOpen returned %d\n", err);
		free(linfo);
		return ais_to_errno(err);
	}

	err = saLckResourceLock(
			res_handle,
			&lock_id,
			mode,
			flags,
			0,
			SA_TIME_END,
			&lockStatus);
	if (err != SA_AIS_OK && lockStatus != SA_LCK_LOCK_GRANTED)
	{
		free(linfo);
		saLckResourceClose(res_handle);
		return ais_to_errno(err);
	}
			
	/* Wait for it to complete */

	DEBUGLOG("lock_resource returning %d, lock_id=%llx\n", err,
		 lock_id);

	linfo->lock_id = lock_id;
	linfo->res_handle = res_handle;

	dm_hash_insert(lock_hash, resource, linfo);

	return ais_to_errno(err);
}


static int _unlock_resource(char *resource, int lockid)
{
	SaAisErrorT err;
	struct lock_info *linfo;

	DEBUGLOG("unlock_resource %s\n", resource);
	linfo = dm_hash_lookup(lock_hash, resource);
	if (!linfo)
		return 0;

	DEBUGLOG("unlock_resource: lockid: %llx\n", linfo->lock_id);
	err = saLckResourceUnlock(linfo->lock_id, SA_TIME_END);
	if (err != SA_AIS_OK)
	{
		DEBUGLOG("Unlock returned %d\n", err);
		return ais_to_errno(err);
	}

	/* Release the resource */
	dm_hash_remove(lock_hash, resource);
	saLckResourceClose(linfo->res_handle);
	free(linfo);

	return ais_to_errno(err);
}

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
		status = _lock_resource(lock1, SA_LCK_EX_LOCK_MODE, flags, lockid);
		if (status)
			goto out;

		/* If we can't get this lock too then bail out */
		status = _lock_resource(lock2, SA_LCK_EX_LOCK_MODE, LCK_NONBLOCK,
					lockid);
		if (status == SA_LCK_LOCK_NOT_QUEUED)
		{
			_unlock_resource(lock1, *lockid);
			status = -1;
			errno = EAGAIN;
		}
		break;

	case LCK_PREAD:
	case LCK_READ:
		status = _lock_resource(lock1, SA_LCK_PR_LOCK_MODE, flags, lockid);
		if (status)
			goto out;
		_unlock_resource(lock2, *lockid);
		break;

	case LCK_WRITE:
		status = _lock_resource(lock2, SA_LCK_EX_LOCK_MODE, flags, lockid);
		if (status)
			goto out;
		_unlock_resource(lock1, *lockid);
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

	_unlock_resource(lock1, lockid);
	_unlock_resource(lock2, lockid);

	return status;
}

/* We are always quorate ! */
static int _is_quorate()
{
	return 1;
}

static int _get_main_cluster_fd(void)
{
	int select_fd;

	cpg_fd_get(cpg_handle, &select_fd);
	return select_fd;
}

static int _cluster_fd_callback(struct local_client *fd, char *buf, int len,
				const char *csid,
				struct local_client **new_client)
{
	cluster_client = fd;
	*new_client = NULL;
	cpg_dispatch(cpg_handle, SA_DISPATCH_ONE);
	return 1;
}

static int _cluster_send_message(const void *buf, int msglen, const char *csid,
				 const char *errtext)
{
	struct iovec iov[2];
	SaAisErrorT err;
	int target_node;

	if (csid)
		memcpy(&target_node, csid, OPENAIS_CSID_LEN);
	else
		target_node = 0;

	iov[0].iov_base = &target_node;
	iov[0].iov_len = sizeof(int);
	iov[1].iov_base = (char *)buf;
	iov[1].iov_len = msglen;

	err = cpg_mcast_joined(cpg_handle, CPG_TYPE_AGREED, iov, 2);
	return ais_to_errno(err);
}

/* We don't have a cluster name to report here */
static int _get_cluster_name(char *buf, int buflen)
{
	strncpy(buf, "OpenAIS", buflen);
	return 0;
}

static struct cluster_ops _cluster_openais_ops = {
	.cluster_init_completed   = NULL,
	.cluster_send_message     = _cluster_send_message,
	.name_from_csid           = _name_from_csid,
	.csid_from_name           = _csid_from_name,
	.get_num_nodes            = _get_num_nodes,
	.cluster_fd_callback      = _cluster_fd_callback,
	.get_main_cluster_fd      = _get_main_cluster_fd,
	.cluster_do_node_callback = _cluster_do_node_callback,
	.is_quorate               = _is_quorate,
	.get_our_csid             = _get_our_csid,
	.add_up_node              = _add_up_node,
	.reread_config            = NULL,
	.cluster_closedown        = _cluster_closedown,
	.get_cluster_name         = _get_cluster_name,
	.sync_lock                = _sync_lock,
	.sync_unlock              = _sync_unlock,
};

struct cluster_ops *init_openais_cluster(void)
{
	if (!_init_cluster())
		return &_cluster_openais_ops;
	else
		return NULL;
}
