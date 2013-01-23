/*	$NetBSD: iscsid_driverif.c,v 1.2.2.3 2013/01/23 00:05:31 yamt Exp $	*/

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

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


/* Global node name (Initiator Name and Alias) */
iscsid_set_node_name_req_t node_name;

/* -------------------------------------------------------------------------- */

/*
 * set_node_name:
 *    Handle set_node_name request. Copy names into our own buffers and
 *    set the driver's info as well.
 *
 *    Parameter:
 *          par         The request parameter
 *
 *    Returns: Status.
 */

uint32_t
set_node_name(iscsid_set_node_name_req_t * par)
{
	iscsi_set_node_name_parameters_t snp;

	(void) memset(&snp, 0x0, sizeof(snp));
	if (!par->InitiatorName[0])
		return ISCSID_STATUS_NO_INITIATOR_NAME;

	if (strlen((char *)par->InitiatorName) > ISCSI_STRING_LENGTH
		|| strlen((char *)par->InitiatorAlias) > ISCSI_STRING_LENGTH)
		return ISCSID_STATUS_PARAMETER_INVALID;

	if (!par->InitiatorAlias[0])
		gethostname((char *)node_name.InitiatorAlias, sizeof(node_name.InitiatorAlias));

	node_name = *par;

#ifdef ISCSI_DEBUG				/* DEBUG ONLY: Allow op without driver present */
	if (driver < 0)
		return ISCSID_STATUS_SUCCESS;
#endif

	strlcpy((char *)snp.InitiatorName, (char *)par->InitiatorName,
		sizeof(snp.InitiatorName));
	strlcpy((char *)snp.InitiatorAlias, (char *)par->InitiatorAlias,
		sizeof(snp.InitiatorAlias));
	memcpy(snp.ISID, par->ISID, 6);

	DEB(10, ("Setting Node Name: %s (%s)\n",
			 snp.InitiatorName, snp.InitiatorAlias));
	(void)ioctl(driver, ISCSI_SET_NODE_NAME, &snp);
	return snp.status;
}


/*
 * bind_socket:
 *    Bind socket to initiator portal.
 *
 *    Parameter:
 *       sock     The socket
 *       addr     The initiator portal address
 *
 *    Returns:
 *       TRUE on success, FALSE on error.
 */

static int
bind_socket(int sock, uint8_t * addr)
{
	struct sockaddr_in serverAddress;
	struct hostent *host;

	DEB(8, ("Binding to <%s>\n", addr));
	(void) memset(&serverAddress, 0x0, sizeof(serverAddress));
	host = gethostbyname((char *)addr);
	if (host == NULL)
		return FALSE;
	if (host->h_length > (int)sizeof(serverAddress.sin_addr))
		return FALSE;
	serverAddress.sin_family = host->h_addrtype;
	serverAddress.sin_port = 0;
	serverAddress.sin_len = host->h_length;
	memcpy(&serverAddress.sin_addr, host->h_addr_list[0], host->h_length);

	return bind(sock, (struct sockaddr *)(void *)&serverAddress,
				(socklen_t)sizeof(serverAddress)) >= 0;
}


/* -------------------------------------------------------------------------- */

/*
 * find_free_portal:
 *    Find the Portal with the least number of connections.
 *
 *    Parameter:  the portal group
 *
 *    Returns:    The pointer to the first free portal (or NULL if none found)
 */

static portal_t *
find_free_portal(portal_group_t * group)
{
	portal_t *curr, *m;
	uint32_t n;

	if ((curr = TAILQ_FIRST(&group->portals)) == NULL)
		return NULL;

	m = curr;
	n = curr->active_connections;

	while ((curr = TAILQ_NEXT(curr, group_list)) != NULL)
		if (curr->active_connections < n) {
			m = curr;
			n = curr->active_connections;
		}

	return m;
}


/*
 * make_connection:
 *    Common routine for login and add_connection. Creates the connection
 *    structure, connects the socket, and executes the login.
 *
 *    Parameter:
 *          sess        The associated session. NULL for a send_targets request.
 *          req         The request parameters. NULL for send_targets.
 *          res         The response buffer. For SendTargets, only the status
 *                      is set. For a "real" login, the login response
 *                      is filled in.
 *          stid        Send target request only, else NULL. Pointer to uint32:
 *                         On Input, contains send target ID
 *                         On Output, receives session ID
 *
 *    Returns:    The connection structure on successful login, else NULL.
 *
 *    NOTE: Session list must be locked on entry.
 */

static connection_t *
make_connection(session_t * sess, iscsid_login_req_t * req,
				iscsid_response_t * res, uint32_t * stid)
{
	connection_t *conn;
	iscsi_login_parameters_t loginp;
	int sock;
	int ret;
	int yes = 1;
	target_t *target;
	portal_t *portal = NULL;
	iscsi_portal_address_t *addr;
	struct sockaddr_in serverAddress;
	struct hostent *host;
	initiator_t *init;

	DEB(9, ("Make Connection sess=%p, req=%p, res=%p, stid=%p\n",
			 sess, req, res, stid));
	(void) memset(&loginp, 0x0, sizeof(loginp));
	(void) memset(&serverAddress, 0x0, sizeof(serverAddress));

	/* find the target portal */
	if (stid != NULL) {
		send_target_t *starget;

		if ((starget = find_send_target_id(*stid)) == NULL) {
			res->status = ISCSID_STATUS_INVALID_TARGET_ID;
			return NULL;
		}
		addr = &starget->addr;
		target = (target_t *)(void *)starget;
	} else {
		if (NO_ID(&req->portal_id)
			|| (portal = find_portal(&req->portal_id)) == NULL) {
			portal_group_t *group;

			/* if no ID was specified, use target from existing session */
			if (NO_ID(&req->portal_id)) {
				if (!sess->num_connections ||
					((target = find_target_id(TARGET_LIST,
					sess->target.sid.id)) == NULL)) {
					res->status = ISCSID_STATUS_INVALID_PORTAL_ID;
					return NULL;
				}
			}
			/* if a target was given instead, use it */
			else if ((target =
							  find_target(TARGET_LIST, &req->portal_id)) == NULL) {
				res->status = ISCSID_STATUS_INVALID_PORTAL_ID;
				return NULL;
			}
			/* now get from target to portal - if this is the first connection, */
			/* just use the first portal group. */
			if (!sess->num_connections) {
				group = TAILQ_FIRST(&target->group_list);
			}
			/* if it's a second connection, use an available portal in the same */
			/* portal group */
			else {
				conn = (connection_t *)(void *)
				    TAILQ_FIRST(&sess->connections);

				if (conn == NULL ||
					(portal = find_portal_id(conn->portal.sid.id)) == NULL) {
					res->status = ISCSID_STATUS_INVALID_PORTAL_ID;
					return NULL;
				}
				group = portal->group;
			}

			if ((portal = find_free_portal(group)) == NULL) {
				res->status = ISCSID_STATUS_INVALID_PORTAL_ID;
				return NULL;
			}
			DEB(1, ("find_free_portal returns pid=%d\n", portal->entry.sid.id));
		} else
			target = portal->target;

		addr = &portal->addr;

		/* symbolic name for connection? check for duplicates */
		if (req->sym_name[0]) {
			void *p;

			if (sess->num_connections)
				p = find_connection_name(sess, req->sym_name);
			else
				p = find_session_name(req->sym_name);
			if (p) {
				res->status = ISCSID_STATUS_DUPLICATE_NAME;
				return NULL;
			}
		}
	}

	if (req != NULL && !NO_ID(&req->initiator_id)) {
		if ((init = find_initiator(&req->initiator_id)) == NULL) {
			res->status = ISCSID_STATUS_INVALID_INITIATOR_ID;
			return NULL;
		}
	} else
		init = select_initiator();

	/* translate target address */
	DEB(8, ("Connecting to <%s>, port %d\n", addr->address, addr->port));

	host = gethostbyname((char *)addr->address);
	if (host == NULL) {
		switch (h_errno) {
		case HOST_NOT_FOUND:
			res->status = ISCSID_STATUS_HOST_NOT_FOUND;
			break;
		case TRY_AGAIN:
			res->status = ISCSID_STATUS_HOST_TRY_AGAIN;
			break;
		default:
			res->status = ISCSID_STATUS_HOST_ERROR;
			break;
		}
		return NULL;
	}
	if (host->h_length > (int)sizeof(serverAddress.sin_addr)) {
		res->status = ISCSID_STATUS_HOST_ERROR;
		return NULL;
	}
	DEB(8, ("Gethostbyname OK, addrtype %d, len %d, addr %x\n",
			host->h_addrtype, host->h_length, *((int *) host->h_addr_list[0])));
	serverAddress.sin_family = host->h_addrtype;
	serverAddress.sin_port = htons((addr->port)
		? addr->port : ISCSI_DEFAULT_PORT);
	serverAddress.sin_len = host->h_length;
	memcpy(&serverAddress.sin_addr, host->h_addr_list[0], host->h_length);

	/* alloc the connection structure */
	conn = calloc(1, sizeof(*conn));
	if (conn == NULL) {
		res->status = ISCSID_STATUS_NO_RESOURCES;
		return NULL;
	}
	/* create and connect the socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		free(conn);
		res->status = ISCSID_STATUS_SOCKET_ERROR;
		return NULL;
	}

	if (init) {
		if (!bind_socket(sock, init->address)) {
			close(sock);
			free(conn);
			res->status = ISCSID_STATUS_INITIATOR_BIND_ERROR;
			return NULL;
		}
	}

	DEB(8, ("Connecting socket\n"));
	if (connect(sock, (struct sockaddr *)(void *)&serverAddress,
		(socklen_t)sizeof(serverAddress)) < 0) {
		close(sock);
		free(conn);
		res->status = ISCSID_STATUS_CONNECT_ERROR;
		DEB(1, ("Connecting to socket failed (error %d), returning %d\n",
				errno, res->status));
		return NULL;
	}
	/* speed up socket processing */
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, (socklen_t)sizeof(yes));

	/* setup login parameter structure */
	loginp.socket = sock;
	if (target->TargetName[0]) {
		loginp.is_present.TargetName = 1;
		loginp.TargetName = target->TargetName;
	}
	if (target->options.is_present.MaxConnections) {
		loginp.is_present.MaxConnections = 1;
		loginp.MaxConnections = target->options.MaxConnections;
	}
	if (target->options.is_present.DataDigest) {
		loginp.is_present.DataDigest = 1;
		loginp.DataDigest = target->options.DataDigest;
	}
	if (target->options.is_present.HeaderDigest) {
		loginp.is_present.HeaderDigest = 1;
		loginp.HeaderDigest = target->options.HeaderDigest;
	}
	if (target->options.is_present.DefaultTime2Retain) {
		loginp.is_present.DefaultTime2Retain = 1;
		loginp.DefaultTime2Retain = target->options.DefaultTime2Retain;
	}
	if (target->options.is_present.DefaultTime2Wait) {
		loginp.is_present.DefaultTime2Wait = 1;
		loginp.DefaultTime2Wait = target->options.DefaultTime2Wait;
	}
	if (target->options.is_present.ErrorRecoveryLevel) {
		loginp.is_present.ErrorRecoveryLevel = 1;
		loginp.ErrorRecoveryLevel = target->options.ErrorRecoveryLevel;
	}
	if (target->options.is_present.MaxRecvDataSegmentLength) {
		loginp.is_present.MaxRecvDataSegmentLength = 1;
		loginp.MaxRecvDataSegmentLength =
			target->options.MaxRecvDataSegmentLength;
	}
	if (target->auth.auth_info.auth_number) {
		loginp.is_present.auth_info = 1;
		loginp.auth_info = target->auth.auth_info;
		if (target->auth.password[0]) {
			loginp.is_present.password = 1;
			loginp.password = target->auth.password;
		}
		if (target->auth.target_password[0]) {
			loginp.is_present.target_password = 1;
			loginp.target_password = target->auth.target_password;
		}
		if (target->auth.user_name[0]) {
			loginp.is_present.user_name = 1;
			loginp.user_name = target->auth.user_name;
		}
	}
	loginp.is_present.TargetAlias = 1;
	loginp.TargetAlias = target->TargetAlias;

	if (portal != NULL) {
		/* override general target options with portal options (if specified) */
		if (portal->options.is_present.DataDigest) {
			loginp.is_present.DataDigest = 1;
			loginp.DataDigest = portal->options.DataDigest;
		}
		if (portal->options.is_present.HeaderDigest) {
			loginp.is_present.HeaderDigest = 1;
			loginp.HeaderDigest = portal->options.HeaderDigest;
		}
		if (portal->options.is_present.MaxRecvDataSegmentLength) {
			loginp.is_present.MaxRecvDataSegmentLength = 1;
			loginp.MaxRecvDataSegmentLength =
				portal->options.MaxRecvDataSegmentLength;
		}
	}

	if (req != NULL) {
		loginp.session_id = get_id(&list[SESSION_LIST].list, &req->session_id);
		loginp.login_type = req->login_type;
	} else
		loginp.login_type = ISCSI_LOGINTYPE_DISCOVERY;

	DEB(5, ("Calling Login...\n"));

	ret = ioctl(driver, (sess != NULL && sess->num_connections)
				? ISCSI_ADD_CONNECTION : ISCSI_LOGIN, &loginp);

	res->status = loginp.status;

	if (ret)
		close(sock);

	if (ret || loginp.status) {
		free(conn);
		if (!res->status)
			res->status = ISCSID_STATUS_GENERAL_ERROR;
		return NULL;
	}
	/* connection established! link connection into session and return IDs */

	conn->loginp = loginp;
	conn->entry.sid.id = loginp.connection_id;
	if (req != NULL) {
		strlcpy((char *)conn->entry.sid.name, (char *)req->sym_name,
			sizeof(conn->entry.sid.name));
	}

	/*
	   Copy important target information
	 */
	conn->target.sid = target->entry.sid;
	strlcpy((char *)conn->target.TargetName, (char *)target->TargetName,
		sizeof(conn->target.TargetName));
	strlcpy((char *)conn->target.TargetAlias, (char *)target->TargetAlias,
		sizeof(conn->target.TargetAlias));
	conn->target.options = target->options;
	conn->target.auth = target->auth;
	conn->portal.addr = *addr;

	conn->session = sess;

	if (stid == NULL) {
		iscsid_login_rsp_t *rsp = (iscsid_login_rsp_t *)(void *)
		    res->parameter;

		sess->entry.sid.id = loginp.session_id;
		TAILQ_INSERT_TAIL(&sess->connections, &conn->entry, link);
		sess->num_connections++;

		res->parameter_length = sizeof(*rsp);
		rsp->connection_id = conn->entry.sid;
		rsp->session_id = sess->entry.sid;

		if (init != NULL) {
			conn->initiator_id = init->entry.sid.id;
			init->active_connections++;
		}
	} else
		*stid = loginp.session_id;

	/*
	   Copy important portal information
	 */
	if (portal != NULL) {
		conn->portal.sid = portal->entry.sid;
		portal->active_connections++;
	}

	return conn;
}


/*
 * event_recover_connection:
 *    Handle RECOVER_CONNECTION event: Attempt to re-establish connection.
 *
 *    Parameter:
 *          sid         Session ID
 *          cid         Connection ID
 */

static void
event_recover_connection(uint32_t sid, uint32_t cid)
{
	int sock, ret;
	int yes = 1;
	session_t *sess;
	connection_t *conn;
	portal_t *portal;
	initiator_t *init;
	iscsi_portal_address_t *addr;
	struct sockaddr_in serverAddress;
	struct hostent *host;

	DEB(1, ("Event_Recover_Connection sid=%d, cid=%d\n", sid, cid));
	(void) memset(&serverAddress, 0x0, sizeof(serverAddress));

	LOCK_SESSIONS;

	sess = find_session_id(sid);
	if (sess == NULL) {
		UNLOCK_SESSIONS;
		return;
	}

	conn = find_connection_id(sess, cid);
	if (conn == NULL) {
		UNLOCK_SESSIONS;
		return;
	}

	UNLOCK_SESSIONS;

	conn->loginp.status = ISCSI_STATUS_CONNECTION_FAILED;

	/* If we can't find the portal to connect to, abort. */

	if ((portal = find_portal_id(conn->portal.sid.id)) == NULL)
		return;

	init = find_initiator_id(conn->initiator_id);
	addr = &portal->addr;
	conn->portal.addr = *addr;

	/* translate target address */
	DEB(1, ("Event_Recover_Connection Connecting to <%s>, port %d\n",
			addr->address, addr->port));

	if ((host = gethostbyname((char *)addr->address)) == NULL) {
		DEB(1, ("GetHostByName failed (error %d)\n", h_errno));
		return;
	}
	if (host->h_length > (int)sizeof(serverAddress.sin_addr)) {
		DEB(1, ("Host address length invalid (%d)\n", host->h_length));
		return;
	}

	serverAddress.sin_family = host->h_addrtype;
	serverAddress.sin_port = htons((addr->port)
		? addr->port : ISCSI_DEFAULT_PORT);
	serverAddress.sin_len = host->h_length;
	memcpy(&serverAddress.sin_addr, host->h_addr_list[0], host->h_length);

	/* create and connect the socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		DEB(1, ("Creating socket failed (error %d)\n", errno));
		return;
	}

	DEB(1, ("recover_connection: Socket = %d\n", sock));

	if (init) {
		if (!bind_socket(sock, init->address)) {
			DEB(1, ("Binding to interface failed (error %d)\n", errno));
			close(sock);
			return;
		}
	}

	if (connect(sock, (struct sockaddr *)(void *)&serverAddress,
		(socklen_t)sizeof(serverAddress)) < 0) {
		DEB(1, ("Connecting to socket failed (error %d)\n", errno));
		close(sock);
		return;
	}
	/* speed up socket processing */
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, (socklen_t)sizeof(yes));

	conn->loginp.socket = sock;
	conn->loginp.status = 0;
	ret = ioctl(driver, ISCSI_RESTORE_CONNECTION, &conn->loginp);

	if (ret)
		close(sock);
}


/*
 * login:
 *    Handle LOGIN request: Log into given portal. Create session, then
 *    let make_connection do the rest.
 *
 *    Parameter:
 *          req         The request parameters
 *          res         The response buffer
 */

void
login(iscsid_login_req_t * req, iscsid_response_t * res)
{
	session_t *sess;
	connection_t *conn;

	sess = calloc(1, sizeof(*sess));
	if (sess == NULL) {
		res->status = ISCSID_STATUS_NO_RESOURCES;
		return;
	}
	TAILQ_INIT(&sess->connections);
	strlcpy((char *)sess->entry.sid.name, (char *)req->sym_name,
			sizeof(sess->entry.sid.name));

	LOCK_SESSIONS;
	conn = make_connection(sess, req, res, 0);

	if (conn == NULL)
		free(sess);
	else {
		sess->target = conn->target;
		TAILQ_INSERT_TAIL(&list[SESSION_LIST].list, &sess->entry, link);
		list[SESSION_LIST].num_entries++;
	}
	UNLOCK_SESSIONS;
}


/*
 * add_connection:
 *    Handle ADD_CONNECTION request: Log secondary connection into given portal.
 *    Find the session, then let make_connection do the rest.
 *
 *    Parameter:
 *          req         The request parameters
 *          res         The response buffer
 */

void
add_connection(iscsid_login_req_t * req, iscsid_response_t * res)
{
	session_t *sess;

	LOCK_SESSIONS;
	sess = find_session(&req->session_id);
	if (sess == NULL) {
		UNLOCK_SESSIONS;
		res->status = ISCSID_STATUS_INVALID_SESSION_ID;
		return;
	}

	make_connection(sess, req, res, 0);
	UNLOCK_SESSIONS;
}


/*
 * logout:
 *    Handle LOGOUT request: Log out the given session.
 *
 *    Parameter:
 *          req         The request parameters
 *
 *    Returns: Response status
 */

uint32_t
logout(iscsid_sym_id_t * req)
{
	iscsi_logout_parameters_t logoutp;
	session_t *sess;
	int ret;

	(void) memset(&logoutp, 0x0, sizeof(logoutp));
	LOCK_SESSIONS;
	sess = find_session(req);
	if (sess == NULL) {
		UNLOCK_SESSIONS;
		return ISCSID_STATUS_INVALID_SESSION_ID;
	}

	logoutp.session_id = sess->entry.sid.id;
	UNLOCK_SESSIONS;

	ret = ioctl(driver, ISCSI_LOGOUT, &logoutp);
	DEB(9, ("Logout returns %d, status = %d\n", ret, logoutp.status));

	return logoutp.status;
}


/*
 * remove_connection:
 *    Handle REMOVE_CONNECTION request: Log out the given connection.
 *
 *    Parameter:
 *          req         The request parameters
 *
 *    Returns: Response status
 */

uint32_t
remove_connection(iscsid_remove_connection_req_t * req)
{
	iscsi_remove_parameters_t removep;
	session_t *sess;
	connection_t *conn;
	int ret;

	LOCK_SESSIONS;
	sess = find_session(&req->session_id);
	if (sess == NULL) {
		UNLOCK_SESSIONS;
		return ISCSID_STATUS_INVALID_SESSION_ID;
	}
	conn = find_connection(sess, &req->connection_id);
	if (conn == NULL) {
		UNLOCK_SESSIONS;
		return ISCSID_STATUS_INVALID_CONNECTION_ID;
	}

	removep.session_id = sess->entry.sid.id;
	removep.connection_id = conn->entry.sid.id;
	UNLOCK_SESSIONS;

	ret = ioctl(driver, ISCSI_REMOVE_CONNECTION, &removep);
	DEB(9, ("Remove Connection returns %d, status=%d\n", ret, removep.status));

	return removep.status;
}

/*
 * send_targets:
 *    Handle SEND_TARGETS request:
 *       First login with type = discovery.
 *       Then send the SendTargets iSCSI request to the target, which will
 *       return a list of target portals.
 *       Then logout.
 *
 *    Parameter:
 *          stid              The send target ID
 *          response_buffer   Pointer to pointer to buffer containing response
 *                            The response contains the list of the target
 *							  portals. The caller frees the buffer after it
 *							  is done with it.
 *          response_size     Pointer to variable which upon return will hold
 *							  the size of the response buffer.
 *
 *    Returns: Response status
 */

uint32_t
send_targets(uint32_t stid, uint8_t **response_buffer, uint32_t *response_size)
{
	iscsi_send_targets_parameters_t sendt;
	iscsi_logout_parameters_t logoutp;
	int ret;
	connection_t *conn;
	iscsid_response_t res;
	uint32_t rc = ISCSID_STATUS_SUCCESS;

	(void) memset(&sendt, 0x0, sizeof(sendt));
	(void) memset(&logoutp, 0x0, sizeof(logoutp));
	(void) memset(&res, 0x0, sizeof(res));
	conn = make_connection(NULL, NULL, &res, &stid);
	DEB(9, ("Make connection returns, status = %d\n", res.status));

	if (conn == NULL)
		return res.status;

	sendt.session_id = stid;
	sendt.response_buffer = NULL;
	sendt.response_size = 0;
	sendt.response_used = sendt.response_total = 0;
	strlcpy((char *)sendt.key, "All", sizeof(sendt.key));

	/*Call once to get the size of the buffer necessary */
	ret = ioctl(driver, ISCSI_SEND_TARGETS, &sendt);

	if (!ret && !sendt.status) {
		/* Allocate buffer required and call again to retrieve data */
		/* We allocate one extra byte so we can place a terminating 0 */
		/* at the end of the buffer. */

		sendt.response_size = sendt.response_total;
		sendt.response_buffer = calloc(1, sendt.response_size + 1);
		if (sendt.response_buffer == NULL)
			rc = ISCSID_STATUS_NO_RESOURCES;
		else {
			ret = ioctl(driver, ISCSI_SEND_TARGETS, &sendt);
			((uint8_t *)sendt.response_buffer)[sendt.response_size] = 0;

			if (ret || sendt.status) {
				free(sendt.response_buffer);
				sendt.response_buffer = NULL;
				sendt.response_used = 0;
				if ((rc = sendt.status) == 0)
					rc = ISCSID_STATUS_GENERAL_ERROR;
			}
		}
	} else if ((rc = sendt.status) == 0)
		rc = ISCSID_STATUS_GENERAL_ERROR;

	*response_buffer = sendt.response_buffer;
	*response_size = sendt.response_used;

	logoutp.session_id = stid;
	ret = ioctl(driver, ISCSI_LOGOUT, &logoutp);
	/* ignore logout status */

	free(conn);

	return rc;
}



/*
 * get_version:
 *    Handle GET_VERSION request.
 *
 *    Returns: Filled get_version_rsp structure.
 */

void
get_version(iscsid_response_t ** prsp, int *prsp_temp)
{
	iscsid_response_t *rsp = *prsp;
	iscsid_get_version_rsp_t *ver;
	iscsi_get_version_parameters_t drv_ver;

	rsp = make_rsp(sizeof(iscsid_get_version_rsp_t), prsp, prsp_temp);
	if (rsp == NULL)
		return;
	ver = (iscsid_get_version_rsp_t *)(void *)rsp->parameter;

	ver->interface_version = INTERFACE_VERSION;
	ver->major = VERSION_MAJOR;
	ver->minor = VERSION_MINOR;
	strlcpy ((char *)ver->version_string, VERSION_STRING, sizeof(ver->version_string));

#ifdef ISCSI_DEBUG				/* DEBUG ONLY: Allow op without driver present */
	if (driver < 0)
		return;
#endif
	ioctl(driver, ISCSI_GET_VERSION, &drv_ver);
	ver->driver_interface_version = drv_ver.interface_version;
	ver->driver_major = drv_ver.major;
	ver->driver_minor = drv_ver.minor;
	strlcpy ((char *)ver->driver_version_string, (char *)drv_ver.version_string,
			 sizeof (ver->driver_version_string));
}


/* -------------------------------------------------------------------------- */

iscsi_register_event_parameters_t event_reg;	/* registered event ID */


/*
 * register_event_handler:
 *    Call driver to register the event handler.
 *
 *    Returns:
 *       TRUE on success.
 */

boolean_t
register_event_handler(void)
{
	ioctl(driver, ISCSI_REGISTER_EVENT, &event_reg);
	return event_reg.event_id != 0;
}


/*
 * deregister_event_handler:
 *    Call driver to deregister the event handler. If the event handler thread
 *    is waiting for an event, this will wake it up and cause it to exit.
 */

void
deregister_event_handler(void)
{
	if (event_reg.event_id) {
		ioctl(driver, ISCSI_DEREGISTER_EVENT, &event_reg);
		event_reg.event_id = 0;
	}
}


/*
 * event_handler:
 *    Event handler thread. Wait for the driver to generate an event and
 *    process it appropriately. Exits when the driver terminates or the
 *    handler is deregistered because the daemon is terminating.
 *
 *    Parameter:
 *          par         Not used.
 */

void *
/*ARGSUSED*/
event_handler(void *par)
{
	iscsi_wait_event_parameters_t evtp;
	int rc;

	DEB(99, ("Event handler starts\n"));
	(void) memset(&evtp, 0x0, sizeof(evtp));

	evtp.event_id = event_reg.event_id;

	do {
		if (nothreads)
			rc = ioctl(driver, ISCSI_POLL_EVENT, &evtp);
		else
			rc = ioctl(driver, ISCSI_WAIT_EVENT, &evtp);

		if (rc != 0) {
			perror("ioctl");
			break;
		}

		DEB(1, ("Got Event: kind %d, status %d, sid %d, cid %d, reason %d\n",
				evtp.event_kind, evtp.status, evtp.session_id,
				evtp.connection_id, evtp.reason));

		if (evtp.status)
			break;

		switch (evtp.event_kind) {
		case ISCSI_SESSION_TERMINATED:
			event_kill_session(evtp.session_id);
			break;

		case ISCSI_CONNECTION_TERMINATED:
			event_kill_connection(evtp.session_id, evtp.connection_id);
			break;

		case ISCSI_RECOVER_CONNECTION:
			event_recover_connection(evtp.session_id, evtp.connection_id);
			break;

		default:
			break;
		}
	} while (evtp.event_kind != ISCSI_DRIVER_TERMINATING);

	if (nothreads && evtp.event_kind == ISCSI_DRIVER_TERMINATING)
		exit_daemon();

	return NULL;
}

#if 0
/*
 * verify_connection:
 *    Verify that a specific connection still exists, delete it if not.
 *
 * Parameter:  The connection pointer.
 *
 * Returns:    The status returned by the driver.
 */

uint32_t
verify_connection(connection_t * conn)
{
	iscsi_conn_status_parameters_t req;
	session_t *sess = conn->session;

	req.connection_id = conn->entry.sid.id;
	req.session_id = sess->entry.sid.id;

	ioctl(driver, ISCSI_CONNECTION_STATUS, &req);

	if (req.status) {
		TAILQ_REMOVE(&sess->connections, &conn->entry, link);
		sess->num_connections--;
		free(conn);
	}
	DEB(9, ("Verify connection returns status %d\n", req.status));
	return req.status;
}

#endif
