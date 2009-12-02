/*	$NetBSD: tcp-comms.c,v 1.1.1.2 2009/12/02 00:27:06 haad Exp $	*/

/*
 *  Copyright (C) 2002-2003 Sistina Software, Inc. All rights reserved.
 *  Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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
 * This provides the inter-clvmd communications for a system without CMAN.
 * There is a listening TCP socket which accepts new connections in the
 * normal way.
 * It can also make outgoing connnections to the other clvmd nodes.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <netdb.h>
#include <assert.h>
#include <libdevmapper.h>

#include "clvm.h"
#include "clvmd-comms.h"
#include "clvmd.h"
#include "clvmd-gulm.h"

#define DEFAULT_TCP_PORT 21064

static int listen_fd = -1;
static int tcp_port;
struct dm_hash_table *sock_hash;

static int get_our_ip_address(char *addr, int *family);
static int read_from_tcpsock(struct local_client *fd, char *buf, int len, char *csid,
			     struct local_client **new_client);

/* Called by init_cluster() to open up the listening socket */
int init_comms(unsigned short port)
{
    struct sockaddr_in6 addr;

    sock_hash = dm_hash_create(100);
    tcp_port = port ? : DEFAULT_TCP_PORT;

    listen_fd = socket(AF_INET6, SOCK_STREAM, 0);

    if (listen_fd < 0)
    {
	return -1;
    }
    else
    {
	int one = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(int));
    }

    memset(&addr, 0, sizeof(addr)); // Bind to INADDR_ANY
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(tcp_port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
	DEBUGLOG("Can't bind to port: %s\n", strerror(errno));
	syslog(LOG_ERR, "Can't bind to port %d, is clvmd already running ?", tcp_port);
	close(listen_fd);
	return -1;
    }

    listen(listen_fd, 5);

    /* Set Close-on-exec */
    fcntl(listen_fd, F_SETFD, 1);

    return 0;
}

void tcp_remove_client(const char *c_csid)
{
    struct local_client *client;
    char csid[GULM_MAX_CSID_LEN];
    unsigned int i;
    memcpy(csid, c_csid, sizeof csid);
    DEBUGLOG("tcp_remove_client\n");

    /* Don't actually close the socket here - that's the
       job of clvmd.c whch will do the job when it notices the
       other end has gone. We just need to remove the client(s) from
       the hash table so we don't try to use it for sending any more */
    for (i = 0; i < 2; i++)
    {
	client = dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN);
	if (client)
	{
	    dm_hash_remove_binary(sock_hash, csid, GULM_MAX_CSID_LEN);
	    client->removeme = 1;
	    close(client->fd);
	}
	/* Look for a mangled one too, on the 2nd iteration. */
	csid[0] ^= 0x80;
    }
}

int alloc_client(int fd, const char *c_csid, struct local_client **new_client)
{
    struct local_client *client;
    char csid[GULM_MAX_CSID_LEN];
    memcpy(csid, c_csid, sizeof csid);

    DEBUGLOG("alloc_client %d csid = %s\n", fd, print_csid(csid));

    /* Create a local_client and return it */
    client = malloc(sizeof(struct local_client));
    if (!client)
    {
	DEBUGLOG("malloc failed\n");
	return -1;
    }

    memset(client, 0, sizeof(struct local_client));
    client->fd = fd;
    client->type = CLUSTER_DATA_SOCK;
    client->callback = read_from_tcpsock;
    if (new_client)
	*new_client = client;

    /* Add to our list of node sockets */
    if (dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN))
    {
	DEBUGLOG("alloc_client mangling CSID for second connection\n");
	/* This is a duplicate connection but we can't close it because
	   the other end may already have started sending.
	   So, we mangle the IP address and keep it, all sending will
	   go out of the main FD
	*/
	csid[0] ^= 0x80;
	client->bits.net.flags = 1; /* indicate mangled CSID */

        /* If it still exists then kill the connection as we should only
           ever have one incoming connection from each node */
        if (dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN))
        {
	    DEBUGLOG("Multiple incoming connections from node\n");
            syslog(LOG_ERR, " Bogus incoming connection from %d.%d.%d.%d\n", csid[0],csid[1],csid[2],csid[3]);

	    free(client);
            errno = ECONNREFUSED;
            return -1;
        }
    }
    dm_hash_insert_binary(sock_hash, csid, GULM_MAX_CSID_LEN, client);

    return 0;
}

int get_main_gulm_cluster_fd()
{
    return listen_fd;
}


/* Read on main comms (listen) socket, accept it */
int cluster_fd_gulm_callback(struct local_client *fd, char *buf, int len, const char *csid,
			struct local_client **new_client)
{
    int newfd;
    struct sockaddr_in6 addr;
    socklen_t addrlen = sizeof(addr);
    int status;
    char name[GULM_MAX_CLUSTER_MEMBER_NAME_LEN];

    DEBUGLOG("cluster_fd_callback\n");
    *new_client = NULL;
    newfd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);

    DEBUGLOG("cluster_fd_callback, newfd=%d (errno=%d)\n", newfd, errno);
    if (!newfd)
    {
	syslog(LOG_ERR, "error in accept: %m");
	errno = EAGAIN;
	return -1; /* Don't return an error or clvmd will close the listening FD */
    }

    /* Check that the client is a member of the cluster
       and reject if not.
    */
    if (gulm_name_from_csid((char *)&addr.sin6_addr, name) < 0)
    {
	syslog(LOG_ERR, "Got connect from non-cluster node %s\n",
	       print_csid((char *)&addr.sin6_addr));
	DEBUGLOG("Got connect from non-cluster node %s\n",
		 print_csid((char *)&addr.sin6_addr));
	close(newfd);

	errno = EAGAIN;
	return -1;
    }

    status = alloc_client(newfd, (char *)&addr.sin6_addr, new_client);
    if (status)
    {
	DEBUGLOG("cluster_fd_callback, alloc_client failed, status = %d\n", status);
	close(newfd);
	/* See above... */
	errno = EAGAIN;
	return -1;
    }
    DEBUGLOG("cluster_fd_callback, returning %d, %p\n", newfd, *new_client);
    return newfd;
}

/* Try to get at least 'len' bytes from the socket */
static int really_read(int fd, char *buf, int len)
{
	int got, offset;

	got = offset = 0;

	do {
		got = read(fd, buf+offset, len-offset);
		DEBUGLOG("really_read. got %d bytes\n", got);
		offset += got;
	} while (got > 0 && offset < len);

	if (got < 0)
		return got;
	else
		return offset;
}


static int read_from_tcpsock(struct local_client *client, char *buf, int len, char *csid,
			     struct local_client **new_client)
{
    struct sockaddr_in6 addr;
    socklen_t slen = sizeof(addr);
    struct clvm_header *header = (struct clvm_header *)buf;
    int status;
    uint32_t arglen;

    DEBUGLOG("read_from_tcpsock fd %d\n", client->fd);
    *new_client = NULL;

    /* Get "csid" */
    getpeername(client->fd, (struct sockaddr *)&addr, &slen);
    memcpy(csid, &addr.sin6_addr, GULM_MAX_CSID_LEN);

    /* Read just the header first, then get the rest if there is any.
     * Stream sockets, sigh.
     */
    status = really_read(client->fd, buf, sizeof(struct clvm_header));
    if (status > 0)
    {
	    int status2;

	    arglen = ntohl(header->arglen);

	    /* Get the rest */
	    if (arglen && arglen < GULM_MAX_CLUSTER_MESSAGE)
	    {
		    status2 = really_read(client->fd, buf+status, arglen);
		    if (status2 > 0)
			    status += status2;
		    else
			    status = status2;
	    }
    }

    DEBUGLOG("read_from_tcpsock, status = %d(errno = %d)\n", status, errno);

    /* Remove it from the hash table if there's an error, clvmd will
       remove the socket from its lists and free the client struct */
    if (status == 0 ||
	(status < 0 && errno != EAGAIN && errno != EINTR))
    {
	char remcsid[GULM_MAX_CSID_LEN];

	memcpy(remcsid, csid, GULM_MAX_CSID_LEN);
	close(client->fd);

	/* If the csid was mangled, then make sure we remove the right entry */
	if (client->bits.net.flags)
	    remcsid[0] ^= 0x80;
	dm_hash_remove_binary(sock_hash, remcsid, GULM_MAX_CSID_LEN);

	/* Tell cluster manager layer */
	add_down_node(remcsid);
    }
    else {
	    gulm_add_up_node(csid);
	    /* Send it back to clvmd */
	    process_message(client, buf, status, csid);
    }
    return status;
}

int gulm_connect_csid(const char *csid, struct local_client **newclient)
{
    int fd;
    struct sockaddr_in6 addr;
    int status;
    int one = 1;

    DEBUGLOG("Connecting socket\n");
    fd = socket(PF_INET6, SOCK_STREAM, 0);

    if (fd < 0)
    {
	syslog(LOG_ERR, "Unable to create new socket: %m");
	return -1;
    }

    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, csid, GULM_MAX_CSID_LEN);
    addr.sin6_port = htons(tcp_port);

    DEBUGLOG("Connecting socket %d\n", fd);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in6)) < 0)
    {
	/* "Connection refused" is "normal" because clvmd may not yet be running
	 * on that node.
	 */
	if (errno != ECONNREFUSED)
	{
	    syslog(LOG_ERR, "Unable to connect to remote node: %m");
	}
	DEBUGLOG("Unable to connect to remote node: %s\n", strerror(errno));
	close(fd);
	return -1;
    }

    /* Set Close-on-exec */
    fcntl(fd, F_SETFD, 1);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(int));

    status = alloc_client(fd, csid, newclient);
    if (status)
	close(fd);
    else
	add_client(*newclient);

    /* If we can connect to it, it must be running a clvmd */
    gulm_add_up_node(csid);
    return status;
}

/* Send a message to a known CSID */
static int tcp_send_message(void *buf, int msglen, const char *csid, const char *errtext)
{
    int status;
    struct local_client *client;
    char ourcsid[GULM_MAX_CSID_LEN];

    assert(csid);

    DEBUGLOG("tcp_send_message, csid = %s, msglen = %d\n", print_csid(csid), msglen);

    /* Don't connect to ourself */
    get_our_gulm_csid(ourcsid);
    if (memcmp(csid, ourcsid, GULM_MAX_CSID_LEN) == 0)
	return msglen;

    client = dm_hash_lookup_binary(sock_hash, csid, GULM_MAX_CSID_LEN);
    if (!client)
    {
	status = gulm_connect_csid(csid, &client);
	if (status)
	    return -1;
    }
    DEBUGLOG("tcp_send_message, fd = %d\n", client->fd);

    return write(client->fd, buf, msglen);
}


int gulm_cluster_send_message(void *buf, int msglen, const char *csid, const char *errtext)
{
    int status=0;

    DEBUGLOG("cluster send message, csid = %p, msglen = %d\n", csid, msglen);

    /* If csid is NULL then send to all known (not just connected) nodes */
    if (!csid)
    {
	void *context = NULL;
	char loop_csid[GULM_MAX_CSID_LEN];

	/* Loop round all gulm-known nodes */
	while (get_next_node_csid(&context, loop_csid))
	{
	    status = tcp_send_message(buf, msglen, loop_csid, errtext);
	    if (status == 0 ||
		(status < 0 && (errno == EAGAIN || errno == EINTR)))
		break;
	}
    }
    else
    {

	status = tcp_send_message(buf, msglen, csid, errtext);
    }
    return status;
}

/* To get our own IP address we get the locally bound address of the
   socket that's talking to GULM in the assumption(eek) that it will
   be on the "right" network in a multi-homed system */
static int get_our_ip_address(char *addr, int *family)
{
	struct utsname info;

	uname(&info);
	get_ip_address(info.nodename, addr);

	return 0;
}

/* Public version of above for those that don't care what protocol
   we're using */
void get_our_gulm_csid(char *csid)
{
    static char our_csid[GULM_MAX_CSID_LEN];
    static int got_csid = 0;

    if (!got_csid)
    {
	int family;

	memset(our_csid, 0, sizeof(our_csid));
	if (get_our_ip_address(our_csid, &family))
	{
	    got_csid = 1;
	}
    }
    memcpy(csid, our_csid, GULM_MAX_CSID_LEN);
}

static void map_v4_to_v6(struct in_addr *ip4, struct in6_addr *ip6)
{
   ip6->s6_addr32[0] = 0;
   ip6->s6_addr32[1] = 0;
   ip6->s6_addr32[2] = htonl(0xffff);
   ip6->s6_addr32[3] = ip4->s_addr;
}

/* Get someone else's IP address from DNS */
int get_ip_address(const char *node, char *addr)
{
    struct hostent *he;

    memset(addr, 0, GULM_MAX_CSID_LEN);

    // TODO: what do we do about multi-homed hosts ???
    // CCSs ip_interfaces solved this but some bugger removed it.

    /* Try IPv6 first. The man page for gethostbyname implies that
       it will lookup ip6 & ip4 names, but it seems not to */
    he = gethostbyname2(node, AF_INET6);
    if (he)
    {
	memcpy(addr, he->h_addr_list[0],
	       he->h_length);
    }
    else
    {
	he = gethostbyname2(node, AF_INET);
	if (!he)
	    return -1;
	map_v4_to_v6((struct in_addr *)he->h_addr_list[0], (struct in6_addr *)addr);
    }

    return 0;
}

char *print_csid(const char *csid)
{
    static char buf[128];
    int *icsid = (int *)csid;

    sprintf(buf, "[%x.%x.%x.%x]",
	    icsid[0],icsid[1],icsid[2],icsid[3]);

    return buf;
}
