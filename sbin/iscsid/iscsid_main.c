/*	$NetBSD: iscsid_main.c,v 1.7.2.1 2013/02/25 00:28:08 tls Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "iscsid_globals.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>

#define DEVICE    "/dev/iscsi0"

/* -------------------------------------------------------------------------- */

list_head_t list[NUM_DAEMON_LISTS];	/* the lists this daemon keeps */

pthread_mutex_t sesslist_lock;	/* session list lock */
pthread_t event_thread;			/* event thread handle */

int driver = -1;				/* the driver's file desc */
int client_sock;				/* the client communication socket */

#ifndef ISCSI_DEBUG
#define ISCSI_DEBUG 0
#endif
int debug_level = ISCSI_DEBUG;	/* How much info to display */
int nothreads;

/*
   To avoid memory fragmentation (and speed things up a bit), we use the
   static bufs unless the request or response exceeds the buffer size
   (which it normally shouldn't, assuming we don't have thousands
   of list entries).
*/
static uint8_t req_buf[REQ_BUFFER_SIZE];	/* default buffer for requests */
static uint8_t rsp_buf[RSP_BUFFER_SIZE];	/* default buffer for responses */

/* -------------------------------------------------------------------------- */

static void __dead
usage(void) 
{
	fprintf(stderr, "Usage: %s [-d <lvl>] [-n]\n", getprogname());
	exit(EXIT_FAILURE);
}


/*
 * create_node_name:
 *    Create and set default node name.
 *
 *    Returns 0 on success, else an error code.
 */

static int
create_node_name(void)
{
	iscsi_set_node_name_parameters_t snp;
	uint32_t hid = 0;
	size_t siz;
	int mib[2];
	unsigned char *s;

	(void) memset(&snp, 0x0, sizeof(snp));
	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTID;
	siz = sizeof(hid);
	sysctl(mib, 2, &hid, &siz, NULL, 0);
	mib[1] = KERN_HOSTNAME;
	siz = ISCSI_STRING_LENGTH - 45;
	sysctl(mib, 2, snp.InitiatorAlias, &siz, NULL, 0);

	DEB(1, ("Host Name: <%s>, Host ID: %u\n", snp.InitiatorAlias, hid));
	if (!snp.InitiatorAlias[0]) {
		printf("Warning: iSCSI Node Name not set (No Host Name)!\n");
		return ISCSID_STATUS_NO_INITIATOR_NAME;
	}
	for (s = snp.InitiatorAlias; *s; s++)
		if (!isalnum((unsigned char) *s) && *s != '-' && *s != '.' && *s != ':')
			*s = '-';
	snprintf((char *)snp.InitiatorName, sizeof(snp.InitiatorName),
		"iqn.1994-04.org.netbsd:iscsi.%s:%u", snp.InitiatorAlias, hid);

	ioctl(driver, ISCSI_SET_NODE_NAME, &snp);
	return snp.status;
}


/*
 * init_daemon:
 *    Open driver, create communication socket.
 *
 *    Returns:    <0 on error
 */

static int
init_daemon(void)
{
	int sock, i;
	struct sockaddr_un name;
	iscsid_request_t req;

	if ((driver = open(DEVICE, O_RDONLY)) < 0) {
		perror("opening " DEVICE);
#ifndef ISCSI_DEBUG		/* DEBUG ONLY: Allow daemon to operate w/o driver */
		return -1;
#endif
	}

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("opening datagram socket");
		return -1;
	}

	name.sun_family = AF_UNIX;
	strlcpy(name.sun_path, ISCSID_SOCK_NAME, sizeof(name.sun_path));

	req.request = ISCSID_DAEMON_TEST;
	req.parameter_length = 0;

	i = sendto(sock, &req, sizeof(req), 0, (struct sockaddr *)(void *)&name,
				(socklen_t)sizeof(struct sockaddr_un));
	if (i == sizeof(req)) {
		printf("Daemon already loaded!\n");
		close(sock);
		return -1;
	}

	unlink(ISCSID_SOCK_NAME);
	if (bind(sock, (struct sockaddr *)(void *)&name, (socklen_t)sizeof(struct sockaddr_un))) {
		perror("binding name to socket");
		return -1;
	}

	for (i = 0; i < NUM_DAEMON_LISTS; i++) {
		TAILQ_INIT(&list[i].list);
		list[i].num_entries = 0;
	}

	if (!nothreads && (i = pthread_mutex_init(&sesslist_lock, NULL)) != 0) {
		printf("Mutex init failed (%d)\n", i);
		close(sock);
		return -1;
	}

	if (!register_event_handler()) {
		printf("Couldn't register event handler\n");
		close(sock);
		unlink(ISCSID_SOCK_NAME);
		if (!nothreads)
			pthread_mutex_destroy(&sesslist_lock);
		return -1;
	}

	create_node_name();

	return sock;
}


/*
 * make_rsp:
 *    Allocate a response buffer if the static buffer is insufficient, set
 *    the response parameter length.
 *
 *    Parameter:
 *          len         Response parameter size (not counting header)
 *          prsp        Pointer to address of response buffer
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 *
 *    Returns:    Pointer to response buffer, NULL if allocation failed.
 */

iscsid_response_t *
make_rsp(size_t len, iscsid_response_t ** prsp, int *prsp_temp)
{
	iscsid_response_t *rsp;

	if ((len + sizeof(iscsid_response_t)) > RSP_BUFFER_SIZE) {
		if ((rsp = calloc(1, len)) == NULL) {
			(*prsp)->status = ISCSID_STATUS_NO_RESOURCES;
			return NULL;
		}
		*prsp_temp = TRUE;
		*prsp = rsp;
	} else
		rsp = *prsp;

	memset (rsp, 0, len + sizeof(iscsid_response_t));
	rsp->parameter_length = (uint32_t)len;
	return rsp;
}


/*
 * process_message:
 *    minimal parameter check and dispatch for the daemon functions.
 *
 *    Parameter:
 *          req         The request
 *          prsp        Pointer to address of response buffer
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

static void
process_message(iscsid_request_t *req, iscsid_response_t **prsp, int *prsp_temp)
{
	iscsid_response_t *rsp;
	void *p = req->parameter;

	*prsp_temp = FALSE;
	*prsp = rsp = (iscsid_response_t *)(void *)rsp_buf;
	rsp->parameter_length = 0;
	rsp->status = ISCSID_STATUS_SUCCESS;

	switch (req->request) {
	case ISCSID_ADD_TARGET:
		if (req->parameter_length < sizeof(iscsid_add_target_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		add_target((iscsid_add_target_req_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_ADD_PORTAL:
		if (req->parameter_length != sizeof(iscsid_add_portal_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		add_portal((iscsid_add_portal_req_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_SET_TARGET_OPTIONS:
		if (req->parameter_length != sizeof(iscsid_get_set_target_options_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = set_target_options((iscsid_get_set_target_options_t *)p);
		break;

	case ISCSID_GET_TARGET_OPTIONS:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = ISCSID_STATUS_NOTIMPL;
		break;

	case ISCSID_SET_TARGET_AUTHENTICATION:
		if (req->parameter_length !=
			sizeof(iscsid_set_target_authentication_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = set_target_auth((iscsid_set_target_authentication_req_t *)p);
		break;

	case ISCSID_SLP_FIND_TARGETS:
		rsp->status = ISCSID_STATUS_NOTIMPL;
		break;

	case ISCSID_REFRESH_TARGETS:
		if (req->parameter_length < sizeof(iscsid_refresh_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = refresh_targets((iscsid_refresh_req_t *)p);
		break;

	case ISCSID_REMOVE_TARGET:
		if (req->parameter_length != sizeof(iscsid_list_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = remove_target((iscsid_list_id_t *)p);
		break;

	case ISCSID_SEARCH_LIST:
		if (req->parameter_length != sizeof(iscsid_search_list_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		search_list((iscsid_search_list_req_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_GET_LIST:
		if (req->parameter_length != sizeof(iscsid_get_list_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_list((iscsid_get_list_req_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_GET_TARGET_INFO:
		if (req->parameter_length != sizeof(iscsid_list_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_target_info((iscsid_list_id_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_GET_PORTAL_INFO:
		if (req->parameter_length != sizeof(iscsid_list_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_portal_info((iscsid_list_id_t *)p, prsp, prsp_temp);
		break;

#ifndef ISCSI_MINIMAL
	case ISCSID_ADD_ISNS_SERVER:
		if (req->parameter_length != sizeof(iscsid_add_isns_server_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		add_isns_server((iscsid_add_isns_server_req_t *)p,
						prsp, prsp_temp);
		break;

	case ISCSID_GET_ISNS_SERVER:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_isns_server((iscsid_sym_id_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_SLP_FIND_ISNS_SERVERS:
		rsp->status = ISCSID_STATUS_NOTIMPL;
		break;

	case ISCSID_REMOVE_ISNS_SERVER:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = remove_isns_server((iscsid_sym_id_t *)p);
		break;
#endif

	case ISCSID_ADD_INITIATOR_PORTAL:
		if (req->parameter_length != sizeof(iscsid_add_initiator_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		add_initiator_portal((iscsid_add_initiator_req_t *)p,
							prsp, prsp_temp);
		break;

	case ISCSID_GET_INITIATOR_PORTAL:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_initiator_portal((iscsid_sym_id_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_REMOVE_INITIATOR_PORTAL:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = remove_initiator_portal((iscsid_sym_id_t *)p);
		break;

	case ISCSID_LOGIN:
		if (req->parameter_length != sizeof(iscsid_login_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		login((iscsid_login_req_t *)p, rsp);
		break;

	case ISCSID_ADD_CONNECTION:
		if (req->parameter_length != sizeof(iscsid_login_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		add_connection((iscsid_login_req_t *)p, rsp);
		break;

	case ISCSID_LOGOUT:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = logout((iscsid_sym_id_t *)p);
		break;

	case ISCSID_REMOVE_CONNECTION:
		if (req->parameter_length != sizeof(iscsid_remove_connection_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = remove_connection((iscsid_remove_connection_req_t *)p);
		break;

	case ISCSID_GET_SESSION_LIST:
		get_session_list(prsp, prsp_temp);
		break;

	case ISCSID_GET_CONNECTION_LIST:
		if (req->parameter_length != sizeof(iscsid_sym_id_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_connection_list((iscsid_sym_id_t *)p, prsp, prsp_temp);
		break;

	case ISCSID_GET_CONNECTION_INFO:
		if (req->parameter_length != sizeof(iscsid_get_connection_info_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		get_connection_info((iscsid_get_connection_info_req_t *)p,
							prsp, prsp_temp);
		break;

	case ISCSID_SET_NODE_NAME:
		if (req->parameter_length != sizeof(iscsid_set_node_name_req_t)) {
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		rsp->status = set_node_name((iscsid_set_node_name_req_t *)p);
		break;

	case ISCSID_GET_VERSION:
		get_version(prsp, prsp_temp);
		break;

	default:
		rsp->status = ISCSID_STATUS_INVALID_REQUEST;
		break;
	}
}


/*
 * exit_daemon:
 *    Deregister the event handler, deregister isns servers, then exit program.
 */

void
exit_daemon(void)
{
	LOCK_SESSIONS;
	deregister_event_handler();

#ifndef ISCSI_MINIMAL
	dereg_all_isns_servers();
#endif

	printf("iSCSI Daemon Exits\n");
	exit(0);
}


/*
 * main:
 *    init, go daemon, then loop reading requests, processing them,
 *    and sending responses.
 *    Stops on receiving a terminate message (no response to that one is sent),
 *    or when an error occurs reading or writing the socket.
 *
 *    Parameter:  argc, argv currently ignored.
 */

int
/*ARGSUSED*/
main(int argc, char **argv)
{
	int req_temp, rsp_temp, c;
	ssize_t ret;
	size_t len;
	struct sockaddr_un from;
	socklen_t fromlen;
	iscsid_request_t *req;
	iscsid_response_t *rsp;
	struct timeval seltout = { 2, 0 };	/* 2 second poll interval */
	char *p;

	while ((c = getopt(argc, argv, "d:n")) != -1)
		switch (c) {
		case 'n':
			nothreads++;
			break;
		case 'd':
			debug_level=(int)strtol(optarg, &p, 10);
			if (*p)
				errx(EXIT_FAILURE, "illegal debug level -- %s",
				    optarg);
			break;
		default:
			usage();
		}

	client_sock = init_daemon();
	if (client_sock < 0)
		exit(1);

	printf("iSCSI Daemon loaded\n");

	if (!debug_level)
		daemon(0, 1);

	if (nothreads)
		setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &seltout,
		    sizeof(seltout));
	else {
		ret = pthread_create(&event_thread, NULL, event_handler, NULL);
		if (ret) {
			printf("Thread creation failed (%zd)\n", ret);
			close(client_sock);
			unlink(ISCSID_SOCK_NAME);
			deregister_event_handler();
			pthread_mutex_destroy(&sesslist_lock);
			return -1;
		}
	}

    /* ---------------------------------------------------------------------- */

	for (;;) {
		/* First, get size of request */
		req = (iscsid_request_t *)(void *)req_buf;
		fromlen = sizeof(from);
		len = sizeof(iscsid_request_t);

		if (nothreads) {
			do {
				ret = recvfrom(client_sock, req, len, MSG_PEEK |
				MSG_WAITALL, (struct sockaddr *)(void *)&from,
			    	&fromlen);
				if (ret == -1)
					event_handler(NULL);
			} while (ret == -1 && errno == EAGAIN);
		} else {
			do {
				ret = recvfrom(client_sock, req, len, MSG_PEEK |
				    MSG_WAITALL, (struct sockaddr *) &from,
				    &fromlen);
				if (ret == -1)
					event_handler(NULL);
			} while (ret == -1 && errno == EAGAIN);
		}

		if ((size_t)ret != len) {
			perror("Receiving from socket");
			break;
		}
		DEB(98, ("Request %d, parlen %d\n",
				req->request, req->parameter_length));

		len += req->parameter_length;

		/* now that we know the size, get the buffer for it */
		req_temp = (len > REQ_BUFFER_SIZE);

		if (req_temp) {
			req = malloc(len);
			if (!req) {
				printf("Can't alloc %zu bytes\n", len);
				break;
			}
		}
		/* read the complete request */
		fromlen = sizeof(from);
		ret = recvfrom(client_sock, req, len, MSG_WAITALL,
						(struct sockaddr *)(void *)&from, &fromlen);
		if ((size_t)ret != len) {
			DEBOUT(("Error receiving from socket!\n"));
			if (req_temp)
				free(req);
			continue;
		}
		/* terminate? then go die. */
		if (req->request == ISCSID_DAEMON_TERMINATE)
			break;

		/* No reply required to test message */
		if (req->request == ISCSID_DAEMON_TEST) {
			if (req_temp)
				free(req);
			continue;
		}
		/* no return path? then we can't send a reply, */
		/* so don't process the command */
		if (!from.sun_path[0]) {
			if (req_temp)
				free(req);
			DEBOUT(("No Return Address!\n"));
			continue;
		}
		/* process the request */
		process_message(req, &rsp, &rsp_temp);
		if (rsp == NULL) {
			if (req_temp)
				free(req);
			DEBOUT(("Invalid message!\n"));
			continue;
		}

		DEB(98, ("Sending reply: status %d, len %d\n",
				rsp->status, rsp->parameter_length));

		/* send the response */
		len = sizeof(iscsid_response_t) + rsp->parameter_length;
		ret = sendto(client_sock, rsp, len, 0,
					(struct sockaddr *)(void *)&from, fromlen);
		if (len != (size_t)ret) {
			DEBOUT(("Error sending reply!\n"));
		}
		/* free temp buffers if we needed them */
		if (req_temp)
			free(req);
		if (rsp_temp)
			free(rsp);
	}

	exit_daemon();

	/* we never get here */
	return 0;
}
