/*	$NetBSD: server.c,v 1.9 2010/10/18 20:43:23 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.sbin/bluetooth/sdpd/server.c,v 1.2 2005/12/06 17:56:36 emax Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: server.c,v 1.9 2010/10/18 20:43:23 plunky Exp $");

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/ucred.h>
#include <sys/un.h>

#include <assert.h>
#include <bluetooth.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <sdp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdpd.h"

static bool	server_open_control	(server_t *, char const *);
static bool	server_open_l2cap	(server_t *);
static void	server_accept_client	(server_t *, int);
static bool	server_process_request	(server_t *, int);
static void	server_close_fd		(server_t *, int);
static bool	server_auth_check	(server_t *, void *);

/* number of groups we allocate space for in cmsg */
#define MAX_GROUPS	20

/*
 * Initialize server
 */
bool
server_init(server_t *srv, char const *control, char const *sgroup)
{

	assert(srv != NULL);
	assert(control != NULL);

	memset(srv, 0, sizeof(srv));
	FD_ZERO(&srv->fdset);
	srv->sgroup = sgroup;

	srv->fdmax = -1;
	srv->fdidx = calloc(FD_SETSIZE, sizeof(fd_idx_t));
	if (srv->fdidx == NULL) {
		log_crit("Failed to allocate fd index");
		goto fail;
	}

	srv->ctllen = CMSG_SPACE(SOCKCREDSIZE(MAX_GROUPS));
	srv->ctlbuf = malloc(srv->ctllen);
	if (srv->ctlbuf == NULL) {
		log_crit("Malloc cmsg buffer (len=%d) failed.", srv->ctllen);
		goto fail;
	}

	srv->imtu = SDP_LOCAL_MTU - sizeof(sdp_pdu_t);
	srv->ibuf = malloc(srv->imtu);
	if (srv->ibuf == NULL) {
		log_crit("Malloc input buffer (imtu=%d) failed.", srv->imtu);
		goto fail;
	}

	srv->omtu = L2CAP_MTU_DEFAULT - sizeof(sdp_pdu_t);
	srv->obuf = malloc(srv->omtu);
	if (srv->obuf == NULL) {
		log_crit("Malloc output buffer (omtu=%d) failed.", srv->omtu);
		goto fail;
	}

	if (db_init(srv)
	    && server_open_control(srv, control)
	    && server_open_l2cap(srv))
		return true;

fail:
	server_shutdown(srv);
	return false;
}

/*
 * Open local control socket
 */
static bool
server_open_control(server_t *srv, char const *control)
{
	struct sockaddr_un	un;
	int			opt, fd;

	if (unlink(control) == -1 && errno != ENOENT) {
		log_crit("Could not unlink(%s). %s (%d)",
		    control, strerror(errno), errno);

		return false;
	}

	fd = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (fd == -1) {
		log_crit("Could not create control socket. %s (%d)",
		    strerror(errno), errno);

		return false;
	}

	opt = 1;
	if (setsockopt(fd, 0, LOCAL_CREDS, &opt, sizeof(opt)) == -1)
		log_crit("Warning: No credential checks on control socket");

	memset(&un, 0, sizeof(un));
	un.sun_len = sizeof(un);
	un.sun_family = AF_LOCAL;
	strlcpy(un.sun_path, control, sizeof(un.sun_path));

	if (bind(fd, (struct sockaddr *) &un, sizeof(un)) == -1) {
		log_crit("Could not bind control socket. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
	}

	if (chmod(control, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) == -1) {
		log_crit("Could not set permissions on control socket. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
	}

	if (listen(fd, 5) == -1) {
		log_crit("Could not listen on control socket. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
	}

	/* Add control descriptor to index */
	if (fd > srv->fdmax)
		srv->fdmax = fd;

	FD_SET(fd, &srv->fdset);
	srv->fdidx[fd].valid = true;
	srv->fdidx[fd].server = true;
	srv->fdidx[fd].control = true;
	srv->fdidx[fd].priv = false;
	return true;
}

/*
 * Open L2CAP server socket
 */
static bool
server_open_l2cap(server_t *srv)
{
	struct sockaddr_bt	sa;
	int			fd;

	fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (fd == -1) {
		log_crit("Could not create L2CAP socket. %s (%d)",
		    strerror(errno), errno);

		return false;
	}

        if (setsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_IMTU,
	    &srv->imtu, sizeof(srv->imtu)) == -1) {
		log_crit("Could not set L2CAP Incoming MTU. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
        }

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	sa.bt_psm = L2CAP_PSM_SDP;
	bdaddr_copy(&sa.bt_bdaddr, BDADDR_ANY);

	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
		log_crit("Could not bind L2CAP socket. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
	}

	if (listen(fd, 5) == -1) {
		log_crit("Could not listen on L2CAP socket. %s (%d)",
		    strerror(errno), errno);

		close(fd);
		return false;
	}

	/* Add L2CAP descriptor to index */
	if (fd > srv->fdmax)
		srv->fdmax = fd;

	FD_SET(fd, &srv->fdset);
	srv->fdidx[fd].valid = true;
	srv->fdidx[fd].server = true;
	srv->fdidx[fd].control = false;
	srv->fdidx[fd].priv = false;
	return true;
}

/*
 * Shutdown server
 */
void
server_shutdown(server_t *srv)
{
	record_t *r;
	int	fd;

	assert(srv != NULL);

	while ((r = LIST_FIRST(&srv->rlist)) != NULL) {
		LIST_REMOVE(r, next);
		free(r);
	}

	for (fd = 0; fd < srv->fdmax + 1; fd ++) {
		if (srv->fdidx[fd].valid)
			server_close_fd(srv, fd);
	}

	free(srv->fdidx);
	free(srv->ctlbuf);
	free(srv->ibuf);
	free(srv->obuf);

	memset(srv, 0, sizeof(*srv));
}

/*
 * Do one server iteration
 */
bool
server_do(server_t *srv)
{
	fd_set	fdset;
	int	n, fd;

	assert(srv != NULL);

	memcpy(&fdset, &srv->fdset, sizeof(fdset));
	n = select(srv->fdmax + 1, &fdset, NULL, NULL, NULL);
	if (n == -1) {
		if (errno == EINTR)
			return true;

		log_err("Could not select(%d, %p). %s (%d)",
		    srv->fdmax + 1, &fdset, strerror(errno), errno);

		return false;
	}

	for (fd = 0; fd < srv->fdmax + 1 && n > 0; fd++) {
		if (!FD_ISSET(fd, &fdset))
			continue;

		assert(srv->fdidx[fd].valid);

		if (srv->fdidx[fd].server)
			server_accept_client(srv, fd);
		else if (!server_process_request(srv, fd))
			server_close_fd(srv, fd);

		n--;
	}

	return true;

}

/*
 * Accept new client connection and register it with index
 */
static void
server_accept_client(server_t *srv, int fd)
{
	struct sockaddr_bt	sa;
	socklen_t		len;
	int			cfd;
	uint16_t		omtu;

	do {
		cfd = accept(fd, NULL, NULL);
	} while (cfd == -1 && errno == EINTR);

	if (cfd == -1) {
		log_err("Could not accept connection on %s socket. %s (%d)",
		    srv->fdidx[fd].control ? "control" : "L2CAP",
		    strerror(errno), errno);

		return;
	}

	if (cfd >= FD_SETSIZE) {
		log_crit("File descriptor too large");
		close(cfd);
		return;
	}

	assert(!FD_ISSET(cfd, &srv->fdset));
	assert(!srv->fdidx[cfd].valid);

	memset(&sa, 0, sizeof(sa));
	omtu = srv->omtu;

	if (!srv->fdidx[fd].control) {
		len = sizeof(sa);
		if (getsockname(cfd, (struct sockaddr *)&sa, &len) == -1)
			log_warning("getsockname failed, using BDADDR_ANY");

		len = sizeof(omtu);
	        if (getsockopt(cfd, BTPROTO_L2CAP, SO_L2CAP_OMTU, &omtu, &len) == -1)
			log_warning("Could not get L2CAP OMTU, using %d", omtu);
		else
			omtu -= sizeof(sdp_pdu_t);
	}

	/* Add client descriptor to the index */
	if (cfd > srv->fdmax)
		srv->fdmax = cfd;

	FD_SET(cfd, &srv->fdset);
	srv->fdidx[cfd].valid = true;
	srv->fdidx[cfd].server = false;
	srv->fdidx[cfd].control = srv->fdidx[fd].control;
	srv->fdidx[cfd].priv = false;
	srv->fdidx[cfd].omtu = (omtu > srv->omtu) ? srv->omtu : omtu;
	srv->fdidx[cfd].offset = 0;
	bdaddr_copy(&srv->fdidx[cfd].bdaddr, &sa.bt_bdaddr);

	log_debug("new %s client on fd#%d",
	    srv->fdidx[cfd].control ? "control" : "L2CAP", cfd);
}

/*
 * Process request from the client
 */
static bool
server_process_request(server_t *srv, int fd)
{
	struct msghdr	msg;
	struct iovec	iov[2];
	struct cmsghdr	*cmsg;
	ssize_t		len;
	uint16_t	error;

	assert(FD_ISSET(fd, &srv->fdset));
	assert(srv->fdidx[fd].valid);
	assert(!srv->fdidx[fd].server);

	iov[0].iov_base = &srv->pdu;
	iov[0].iov_len = sizeof(srv->pdu);
	iov[1].iov_base = srv->ibuf;
	iov[1].iov_len = srv->imtu;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = __arraycount(iov);
	msg.msg_control = srv->ctlbuf;
	msg.msg_controllen = srv->ctllen;
	msg.msg_flags = 0;

	do {
		len = recvmsg(fd, &msg, 0);
	} while (len == -1 && errno == EINTR);

	if (len == -1) {
		log_err("Could not receive SDP request on %s socket. %s (%d)",
		    srv->fdidx[fd].control ? "control" : "L2CAP",
		    strerror(errno), errno);

		return false;
	}

	if (len == 0) {
		log_info("Client on %s socket has disconnected",
		    srv->fdidx[fd].control ? "control" : "L2CAP");

		return false;
	}

	if (msg.msg_flags & MSG_TRUNC)
		log_info("Truncated message on %s socket",
		    srv->fdidx[fd].control ? "control" : "L2CAP");

	if ((cmsg = CMSG_FIRSTHDR(&msg)) != NULL
	    && cmsg->cmsg_level == SOL_SOCKET
	    && cmsg->cmsg_type == SCM_CREDS
	    && cmsg->cmsg_len >= CMSG_LEN(SOCKCREDSIZE(0)))
		srv->fdidx[fd].priv = server_auth_check(srv, CMSG_DATA(cmsg));

	srv->pdu.len = be16toh(srv->pdu.len);

	if ((uint32_t)len < sizeof(srv->pdu)
	    || (uint32_t)len != sizeof(srv->pdu) + srv->pdu.len) {
		error = SDP_ERROR_CODE_INVALID_PDU_SIZE;
	} else {
		switch (srv->pdu.pid) {
		case SDP_PDU_SERVICE_SEARCH_REQUEST:
			error = service_search_request(srv, fd);
			break;

		case SDP_PDU_SERVICE_ATTRIBUTE_REQUEST:
			error = service_attribute_request(srv, fd);
			break;

		case SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_REQUEST:
			error = service_search_attribute_request(srv, fd);
			break;

#ifdef SDP_COMPAT
		case SDP_PDU_SERVICE_REGISTER_REQUEST:
			error = compat_register_request(srv, fd);
			break;

		case SDP_PDU_SERVICE_CHANGE_REQUEST:
			error = compat_change_request(srv, fd);
			break;
#endif

		case SDP_PDU_RECORD_INSERT_REQUEST:
			error = record_insert_request(srv, fd);
			break;

		case SDP_PDU_RECORD_UPDATE_REQUEST:
			error = record_update_request(srv, fd);
			break;

		case SDP_PDU_RECORD_REMOVE_REQUEST:
			error = record_remove_request(srv, fd);
			break;

		default:
			error = SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;
			break;
		}
	}

	if (error != 0) {
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
		srv->pdu.len = sizeof(error);
		be16enc(srv->obuf, error);
		log_debug("sending ErrorResponse (error=0x%04x)", error);
	}

	iov[0].iov_base = &srv->pdu;
	iov[0].iov_len = sizeof(srv->pdu);
	iov[1].iov_base = srv->obuf;
	iov[1].iov_len = srv->pdu.len;

	srv->pdu.len = htobe16(srv->pdu.len);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = __arraycount(iov);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	do {
		len = sendmsg(fd, &msg, 0);
	} while (len == -1 && errno == EINTR);

	if (len == -1) {
		log_err("Could not send SDP response on %s socket. %s (%d)",
		    srv->fdidx[fd].control ? "control" : "L2CAP",
		    strerror(errno), errno);

		return false;
	}

	return true;
}

/*
 * Close descriptor and remove it from index
 */
static void
server_close_fd(server_t *srv, int fd)
{

	assert(FD_ISSET(fd, &srv->fdset));
	assert(srv->fdidx[fd].valid);

	db_unselect(srv, fd);	/* release selected records */
	db_release(srv, fd);	/* expire owned records */

	close(fd);
	FD_CLR(fd, &srv->fdset);
	srv->fdidx[fd].valid = false;

	log_debug("client on fd#%d closed", fd);

	if (fd == srv->fdmax) {
		while (fd > 0 && !srv->fdidx[fd].valid)
			fd--;

		srv->fdmax = fd;
	}
}

/*
 * check credentials, return true when permitted to modify service records
 */
static bool
server_auth_check(server_t *srv, void *data)
{
	struct sockcred *cred = data;
	struct group *grp;
	int n;

	if (cred == NULL)
		return false;

	if (cred->sc_uid == 0 || cred->sc_euid == 0)
		return true;

	if (srv->sgroup == NULL)
		return false;

	grp = getgrnam(srv->sgroup);
	if (grp == NULL) {
		log_err("No gid for group '%s'", srv->sgroup);
		srv->sgroup = NULL;
		return false;
	}

	if (cred->sc_gid == grp->gr_gid || cred->sc_egid == grp->gr_gid)
		return true;

	if (cred->sc_ngroups > MAX_GROUPS) {
		log_info("Credentials truncated, lost %d groups",
		    MAX_GROUPS - cred->sc_ngroups);

		cred->sc_ngroups = MAX_GROUPS;
	}

	for (n = 0 ; n < cred->sc_ngroups ; n++) {
		if (cred->sc_groups[n] == grp->gr_gid)
			return true;
	}

	return false;
}
