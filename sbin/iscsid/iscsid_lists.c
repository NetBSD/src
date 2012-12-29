/*	$NetBSD: iscsid_lists.c,v 1.8 2012/12/29 08:28:20 mlelstv Exp $	*/

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

/* counter for initiator ID */
static uint32_t initiator_id = 0;

/* -------------------------------------------------------------------------- */

#if 0

/*
 * verify_session:
 *    Verify that a specific session still exists, delete it if not.
 *
 * Parameter:  The session pointer.
 */

static void
verify_session(session_t * sess)
{
	generic_entry_t *curr, *next;
	int nosess = 0;

	for (curr = sess->connections.tqh_first; curr != NULL && !nosess; curr = next) {
		next = curr->link.tqe_next;
		nosess = verify_connection((connection_t *) curr) == ISCSI_STATUS_INVALID_SESSION_ID;
	}

	if (!nosess && sess->num_connections)
		return;

	TAILQ_REMOVE(&list[SESSION_LIST].list, &sess->entry, link);
	list[SESSION_LIST].num_entries--;

	while ((curr = TAILQ_FIRST(&sess->connections)) != NULL) {
		TAILQ_REMOVE(&sess->connections, curr, link);
		free(curr);
	}
	free(sess);
}


/*
 * verify_sessions:
 *    Verify that all sessions in the list still exist.
 */

void
verify_sessions(void)
{
	generic_entry_t *curr, *next;

	for (curr = list[SESSION_LIST].list.tqh_first; curr != NULL; curr = next) {
		next = curr->link.tqe_next;
		verify_session((session_t *) curr);
	}
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * find_id:
 *    Find a list element by ID.
 *
 *    Parameter:  the list head and the ID to search for
 *
 *    Returns:    The pointer to the element (or NULL if not found)
 */

generic_entry_t *
find_id(generic_list_t * head, uint32_t id)
{
	generic_entry_t *curr;

	if (!id)
		return NULL;

	TAILQ_FOREACH(curr, head, link)
		if (curr->sid.id == id)
			break;

	return curr;
}

/*
 * find_name:
 *    Find a list entry by name.
 *
 *    Parameter:  the list head and the symbolic name to search for
 *
 *    Returns:    The pointer to the entry (or NULL if not found)
 */

generic_entry_t *
find_name(generic_list_t * head, uint8_t * name)
{
	generic_entry_t *curr;

	if (!*name)
		return NULL;

	TAILQ_FOREACH(curr, head, link)
		if (strcmp((char *)curr->sid.name, (char *)name) == 0)
			break;

	return curr;
}


/*
 * find_sym_id:
 *    Find a list entry by name or numeric id.
 *
 *    Parameter:  the list head and the symbolic id to search for
 *
 *    Returns:    The pointer to the entry (or NULL if not found)
 */

generic_entry_t *
find_sym_id(generic_list_t * head, iscsid_sym_id_t * sid)
{

	if (sid->id != 0)
		return find_id(head, sid->id);

	return (sid->name[0]) ? find_name(head, sid->name) : NULL;
}


/*
 * get_id:
 *    Get the numeric ID for a symbolic ID
 *
 *    Parameter:  the list head and the symbolic id
 *
 *    Returns:    The numeric ID (0 if not found)
 */

uint32_t
get_id(generic_list_t * head, iscsid_sym_id_t * sid)
{
	generic_entry_t *ent;

	if (sid->id != 0)
		return sid->id;

	ent = find_name(head, sid->name);
	return (ent != NULL) ? ent->sid.id : 0;
}


/*
 * find_target_name:
 *    Find a target by TargetName.
 *
 *    Parameter:  the target name
 *
 *    Returns:    The pointer to the target (or NULL if not found)
 */

target_t *
find_target(iscsid_list_kind_t lst, iscsid_sym_id_t * sid)
{
	target_t *targ;

	if ((targ = (target_t *)(void *)find_sym_id (&list [lst].list, sid)) != NULL)
		return targ;
	if (lst == TARGET_LIST) {
		portal_t *portal;

		if ((portal = (void *)find_portal (sid)) != NULL)
			return portal->target;
	}
	return NULL;
}


/*
 * find_target_name:
 *    Find a target by TargetName.
 *
 *    Parameter:  the target name
 *
 *    Returns:    The pointer to the target (or NULL if not found)
 */

target_t *
find_TargetName(iscsid_list_kind_t lst, uint8_t * name)
{
	generic_entry_t *curr;
	target_t *t = NULL;

	if (lst == PORTAL_LIST)
		lst = TARGET_LIST;

	TAILQ_FOREACH(curr, &list[lst].list, link) {
		t = (void *)curr;
		if (strcmp((char *)t->TargetName, (char *)name) == 0)
			break;
	}

	/* return curr instead of t because curr==NULL if name not found */
	DEB(10, ("Find_TargetName returns %p\n", curr));
	return (target_t *)curr;
}


/*
 * find_portal_by_addr:
 *    Find a Portal by Address.
 *
 *    Parameter:  the associated target, and the address
 *
 *    Returns:    The pointer to the portal (or NULL if not found)
 */

portal_t *
find_portal_by_addr(target_t * target, iscsi_portal_address_t * addr)
{
	generic_entry_t *curr;
	portal_t *p = NULL;

	TAILQ_FOREACH(curr, &list[PORTAL_LIST].list, link) {
		p = (void *)curr;
		DEB(10, ("Find_portal_by_addr - addr %s port %d target %p\n",
				 p->addr.address,
				 p->addr.port,
				 p->target));

		if (strcmp((char *)p->addr.address, (char *)addr->address) == 0 &&
			(!addr->port || p->addr.port == addr->port) &&
			p->target == target)
			break;
	}

	/* return curr instead of p because curr==NULL if not found */
	DEB(10, ("Find_portal_by_addr returns %p\n", curr));
	return (portal_t *)curr;
}


/*
 * find_send_target_by_addr:
 *    Find a Send Target by Address.
 *
 *    Parameter:  the address
 *
 *    Returns:    The pointer to the portal (or NULL if not found)
 */

send_target_t *
find_send_target_by_addr(iscsi_portal_address_t * addr)
{
	generic_entry_t *curr;
	send_target_t *t = NULL;

	TAILQ_FOREACH(curr, &list[SEND_TARGETS_LIST].list, link) {
		t = (void *)curr;
		if (strcmp((char *)t->addr.address, (char *)addr->address) == 0 &&
			(!addr->port || t->addr.port == addr->port))
			break;
	}

	/* return curr instead of p because curr==NULL if not found */
	DEB(10, ("Find_send_target_by_addr returns %p\n", curr));
	return (send_target_t *)curr;
}


/*
 * get_list:
 *    Handle GET_LIST request: Return the list of IDs contained in the list.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_list(iscsid_get_list_req_t * par, iscsid_response_t ** prsp, int *prsp_temp)
{
	iscsid_get_list_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	int num;
	uint32_t *idp;
	generic_list_t *plist;
	generic_entry_t *curr;

	DEB(10, ("get_list, kind %d\n", par->list_kind));

	if (par->list_kind == SESSION_LIST)
		LOCK_SESSIONS;
	else if (par->list_kind >= NUM_DAEMON_LISTS) {
		rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
		return;
	}

	plist = &list[par->list_kind].list;
	num = list[par->list_kind].num_entries;

	if (!num) {
		if (par->list_kind == SESSION_LIST)
			UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_LIST_EMPTY;
		return;
	}

	rsp = make_rsp(sizeof(iscsid_get_list_rsp_t) +
					(num - 1) * sizeof(uint32_t), prsp, prsp_temp);
	if (rsp == NULL) {
		if (par->list_kind == SESSION_LIST)
			UNLOCK_SESSIONS;
		return;
	}
	/* copy the ID of all list entries */
	res = (iscsid_get_list_rsp_t *)(void *)rsp->parameter;
	res->num_entries = num;
	idp = res->id;

	TAILQ_FOREACH(curr, plist, link)
		* idp++ = curr->sid.id;

	if (par->list_kind == SESSION_LIST)
		UNLOCK_SESSIONS;
}


/*
 * search_list:
 *    Handle SEARCH_LIST request: Search the given list for the string or
 *    address.
 *    Note: Not all combinations of list and search type make sense.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
search_list(iscsid_search_list_req_t * par, iscsid_response_t ** prsp,
			int *prsp_temp)
{
	iscsid_response_t *rsp = *prsp;
	generic_entry_t *elem = NULL;

	DEB(10, ("search_list, list_kind %d, search_kind %d\n",
			 par->list_kind, par->search_kind));

	if (par->list_kind == SESSION_LIST)
		LOCK_SESSIONS;
	else if (par->list_kind >= NUM_DAEMON_LISTS) {
		rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
		return;
	}

	if (!list[par->list_kind].num_entries) {
		if (par->list_kind == SESSION_LIST)
			UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_NOT_FOUND;
		return;
	}

	switch (par->search_kind) {
	case FIND_ID:
		elem = find_id(&list[par->list_kind].list, par->intval);
		break;

	case FIND_NAME:
		elem = find_name(&list[par->list_kind].list, par->strval);
		break;

	case FIND_TARGET_NAME:
		switch (par->list_kind) {
		case TARGET_LIST:
		case PORTAL_LIST:
		case SEND_TARGETS_LIST:
			elem = (void *)find_TargetName(par->list_kind,
														par->strval);
			break;

		case SESSION_LIST:
			TAILQ_FOREACH(elem, &list[SESSION_LIST].list, link)
				if (strcmp((char *)((session_t *)(void *)elem)->target.TargetName,
							(char *)par->strval) == 0)
					break;
			break;

		default:
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		break;

	case FIND_ADDRESS:
		switch (par->list_kind) {
		case PORTAL_LIST:
			TAILQ_FOREACH(elem, &list[PORTAL_LIST].list, link) {
				portal_t *p = (void *)elem;
				if (strcmp((char *)p->addr.address, (char *)par->strval) == 0 &&
					(!par->intval ||
					 p->addr.port == par->intval))
					break;
			}
			break;

		case SEND_TARGETS_LIST:
			TAILQ_FOREACH(elem, &list[SEND_TARGETS_LIST].list, link) {
				send_target_t *t = (void *)elem;
				if (strcmp((char *)t->addr.address,
							(char *)par->strval) == 0 &&
					(!par->intval ||
					 t->addr.port == par->intval))
					break;
			}
			break;

		case ISNS_LIST:
			TAILQ_FOREACH(elem, &list[ISNS_LIST].list, link) {
				isns_t *i = (void *)elem;
				if (strcmp((char *)i->address, (char *)par->strval) == 0 &&
					(!par->intval || i->port == par->intval))
					break;
			}
			break;

		default:
			rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
			break;
		}
		break;

	default:
		rsp->status = ISCSID_STATUS_INVALID_PARAMETER;
		return;
	}

	if (elem == NULL) {
		if (par->list_kind == SESSION_LIST)
			UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_NOT_FOUND;
		return;
	}

	rsp = make_rsp(sizeof(iscsid_sym_id_t), prsp, prsp_temp);
	if (rsp == NULL) {
		if (par->list_kind == SESSION_LIST)
			UNLOCK_SESSIONS;
		return;
	}

	(void) memcpy(rsp->parameter, &elem->sid, sizeof(elem->sid));
	if (par->list_kind == SESSION_LIST)
		UNLOCK_SESSIONS;
}


/*
 * get_session_list:
 *    Handle GET_SESSION_LIST request: Return a list of sessions complete
 *    with basic session info.
 *
 *    Parameter:
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_session_list(iscsid_response_t ** prsp, int *prsp_temp)
{
	iscsid_get_session_list_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	iscsid_session_list_entry_t *ent;
	generic_list_t *plist;
	generic_entry_t *curr;
	session_t *sess;
	connection_t *conn;
	int num;

	DEB(10, ("get_session_list\n"));

	LOCK_SESSIONS;
	plist = &list[SESSION_LIST].list;
	num = list[SESSION_LIST].num_entries;

	if (!num) {
		UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_LIST_EMPTY;
		return;
	}

	rsp = make_rsp(sizeof(iscsid_get_session_list_rsp_t) +
				   (num - 1) * sizeof(iscsid_session_list_entry_t),
					prsp, prsp_temp);
	if (rsp == NULL) {
		UNLOCK_SESSIONS;
		return;
	}
	/* copy the ID of all list entries */
	res = (iscsid_get_session_list_rsp_t *)(void *)rsp->parameter;
	res->num_entries = num;
	ent = res->session;

	TAILQ_FOREACH(curr, plist, link) {
		sess = (session_t *)(void *)curr;
		conn = (connection_t *)(void *)TAILQ_FIRST(&sess->connections);

		ent->session_id = sess->entry.sid;
		ent->num_connections = sess->num_connections;
		if (conn) {
			ent->first_connection_id = conn->entry.sid.id;
			ent->portal_id = conn->portal.sid.id;
			ent->initiator_id = conn->initiator_id;
		} else {
			ent->first_connection_id = 0;
			ent->portal_id = 0;
			ent->initiator_id = 0;
		}
		ent++;
	}
	UNLOCK_SESSIONS;
}

/*
 * get_connection_list:
 *    Handle GET_CONNECTION_LIST request: Return a list of connections
 *    for a session.
 *
 *    Parameter:
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_connection_list(iscsid_sym_id_t *req, iscsid_response_t **prsp,
					int *prsp_temp)
{
	iscsid_get_connection_list_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	iscsid_connection_list_entry_t *ent;
	generic_entry_t *curr;
	session_t *sess;
	connection_t *conn;
	int num;

	DEB(10, ("get_connection_list\n"));

	LOCK_SESSIONS;
	if ((sess = find_session(req)) == NULL) {
		UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_INVALID_SESSION_ID;
		return;
	}

	num = sess->num_connections;
	rsp = make_rsp(sizeof(iscsid_get_connection_list_rsp_t) +
				   (num - 1) * sizeof(iscsid_connection_list_entry_t),
					prsp, prsp_temp);
	if (rsp == NULL) {
		UNLOCK_SESSIONS;
		return;
	}
	/* copy the ID of all list entries */
	res = (iscsid_get_connection_list_rsp_t *)(void *)rsp->parameter;
	res->num_connections = num;
	ent = res->connection;

	TAILQ_FOREACH(curr, &sess->connections, link) {
		conn = (connection_t *)(void *)curr;
		ent->connection_id = conn->entry.sid;
		ent->target_portal_id = conn->portal.sid;
		ent->target_portal = conn->portal.addr;
		ent++;
	}
	UNLOCK_SESSIONS;
}


/*
 * get_connection_info:
 *    Handle GET_CONNECTION_INFO request: Return information about a connection
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_connection_info(iscsid_get_connection_info_req_t * req,
					iscsid_response_t ** prsp, int *prsp_temp)
{
	iscsid_get_connection_info_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	session_t *sess;
	connection_t *conn;
	initiator_t *init = NULL;

	DEB(10, ("get_connection_info, session %d, connection %d\n",
			 req->session_id.id, req->connection_id.id));

	LOCK_SESSIONS;
	if ((sess = find_session(&req->session_id)) == NULL) {
		UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_INVALID_SESSION_ID;
		return;
	}
	if (!req->connection_id.id && !req->connection_id.name[0]) {
		conn = (connection_t *)(void *)TAILQ_FIRST(&sess->connections);
	} else if ((conn = find_connection(sess, &req->connection_id)) == NULL) {
		UNLOCK_SESSIONS;
		rsp->status = ISCSID_STATUS_INVALID_CONNECTION_ID;
		return;
	}

	rsp = make_rsp(sizeof(iscsid_get_connection_info_rsp_t), prsp, prsp_temp);
	if (rsp == NULL) {
		UNLOCK_SESSIONS;
		return;
	}

	if (conn && conn->initiator_id)
		init = find_initiator_id(conn->initiator_id);

	res = (iscsid_get_connection_info_rsp_t *)(void *)rsp->parameter;

	res->session_id = sess->entry.sid;
	if (conn) {
		res->connection_id = conn->entry.sid;
		res->target_portal_id = conn->portal.sid;
		res->target_portal = conn->portal.addr;
		strlcpy((char *)res->TargetName, (char *)conn->target.TargetName,
			sizeof(res->TargetName));
		strlcpy((char *)res->TargetAlias, (char *)conn->target.TargetAlias,
			sizeof(res->TargetAlias));
	} else {
		res->connection_id.id = 0;
		res->connection_id.name[0] = '\0';
		res->target_portal_id.id = 0;
		res->target_portal_id.name[0] = '\0';
		memset(&res->target_portal, 0, sizeof(res->target_portal));
		memset(&res->TargetName, 0, sizeof(res->TargetName));
		memset(&res->TargetAlias, 0, sizeof(res->TargetAlias));
	}
	if (init != NULL) {
		res->initiator_id = init->entry.sid;
		strlcpy((char *)res->initiator_address, (char *)init->address,
			sizeof(res->initiator_address));
	}
	UNLOCK_SESSIONS;
}

/* ------------------------------------------------------------------------- */

/*
 * find_initator_by_addr:
 *    Find an Initiator Portal by Address.
 *
 *    Parameter:  the address
 *
 *    Returns:    The pointer to the portal (or NULL if not found)
 */

static initiator_t *
find_initiator_by_addr(uint8_t * addr)
{
	generic_entry_t *curr;
	initiator_t *i = NULL;

	TAILQ_FOREACH(curr, &list[INITIATOR_LIST].list, link) {
		i = (void *)curr;
		if (strcmp((char *)i->address, (char *)addr) == 0)
			break;
	}

	/* return curr instead of i because if not found, curr==NULL */
	DEB(9, ("Find_initiator_by_addr returns %p\n", curr));
	return (initiator_t *)curr;
}


/*
 * add_initiator_portal:
 *    Add an initiator portal.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
add_initiator_portal(iscsid_add_initiator_req_t *par, iscsid_response_t **prsp,
					 int *prsp_temp)
{
	iscsid_add_initiator_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	initiator_t *init;

	DEB(9, ("AddInitiatorPortal '%s' (name '%s')\n", par->address, par->name));

	if (find_initiator_by_addr(par->address) != NULL) {
		rsp->status = ISCSID_STATUS_DUPLICATE_ENTRY;
		return;
	}

	if (find_initiator_name(par->name) != NULL) {
		rsp->status = ISCSID_STATUS_DUPLICATE_NAME;
		return;
	}

	if ((init = calloc(1, sizeof(*init))) == NULL) {
		rsp->status = ISCSID_STATUS_NO_RESOURCES;
		return;
	}

	DEB(9, ("AddInitiatorPortal initiator_id = %d\n", initiator_id));

	for (initiator_id++;
		 !initiator_id || find_initiator_id(initiator_id) != NULL;)
		initiator_id++;

	init->entry.sid.id = initiator_id;
	strlcpy((char *)init->entry.sid.name, (char *)par->name, sizeof(init->entry.sid.name));
	strlcpy((char *)init->address, (char *)par->address, sizeof(init->address));

	rsp = make_rsp(sizeof(iscsid_add_initiator_rsp_t), prsp, prsp_temp);
	if (rsp == NULL)
		return;

	LOCK_SESSIONS;
	TAILQ_INSERT_TAIL(&list[INITIATOR_LIST].list, &init->entry, link);
	list[INITIATOR_LIST].num_entries++;
	UNLOCK_SESSIONS;

	res = (iscsid_add_initiator_rsp_t *)(void *)rsp->parameter;
	res->portal_id = init->entry.sid.id;
}


/*
 * remove_initiator_portal:
 *    Handle REMOVE_INITIATOR request: Removes an initiator entry.
 *
 *    Parameter:
 *          par         The request parameter containing the ID.
 *
 *    Returns:     status
 */

uint32_t
remove_initiator_portal(iscsid_sym_id_t * par)
{
	initiator_t *init;

	if ((init = find_initiator(par)) == NULL)
		return ISCSID_STATUS_INVALID_INITIATOR_ID;

	LOCK_SESSIONS;
	list[INITIATOR_LIST].num_entries--;

	TAILQ_REMOVE(&list[INITIATOR_LIST].list, &init->entry, link);
	UNLOCK_SESSIONS;

	free(init);

	return ISCSID_STATUS_SUCCESS;
}



/*
 * get_initiator_portal:
 *    Handle GET_INITIATOR_PORTAL request: Return information about the given
 *    initiator portal.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_initiator_portal(iscsid_sym_id_t *par, iscsid_response_t **prsp,
					 int *prsp_temp)
{
	iscsid_get_initiator_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	initiator_t *init;

	DEB(10, ("get_initiator_portal, id %d (%s)\n", par->id, par->name));

	if ((init = find_initiator(par)) == NULL) {
		rsp->status = ISCSID_STATUS_INVALID_INITIATOR_ID;
		return;
	}

	rsp = make_rsp(sizeof(iscsid_get_initiator_rsp_t), prsp, prsp_temp);
	if (rsp == NULL)
		return;

	res = (iscsid_get_initiator_rsp_t *)(void *)rsp->parameter;
	res->portal_id = init->entry.sid;
	strlcpy((char *)res->address, (char *)init->address, sizeof(res->address));
}


/*
 * select_initiator:
 *    Select the initiator portal to use.
 *    Selects the portal with the least number of active connections.
 *
 *    Returns:
 *       Pointer to the portal, NULL if no portals are defined.
 *
 *    NOTE: Called with session list locked, so don't lock again.
 */

initiator_t *
select_initiator(void)
{
	generic_entry_t *curr;
	initiator_t *imin = NULL;
	uint32_t ccnt = 64 * 1024;	/* probably not more than 64k connections... */

	if (!list[INITIATOR_LIST].num_entries)
		return NULL;

	TAILQ_FOREACH(curr, &list[INITIATOR_LIST].list, link) {
		initiator_t *i = (void *)curr;
		if ((i->active_connections < ccnt)) {
			ccnt = i->active_connections;
			imin = i;
		}
	}
	return imin;
}

/* ------------------------------------------------------------------------- */

/*
 * event_kill_session:
 *    Handle SESSION_TERMINATED event: Remove session and all associated
 *    connections.
 *
 *    Parameter:
 *          sid         Session ID
 */

void
event_kill_session(uint32_t sid)
{
	session_t *sess;
	connection_t *conn;
	portal_t *portal;
	initiator_t *init;

	LOCK_SESSIONS;

	sess = find_session_id(sid);

	if (sess == NULL) {
		UNLOCK_SESSIONS;
		return;
	}

	TAILQ_REMOVE(&list[SESSION_LIST].list, &sess->entry, link);
	list[SESSION_LIST].num_entries--;

	UNLOCK_SESSIONS;

	while ((conn = (connection_t *)(void *)TAILQ_FIRST(&sess->connections)) != NULL) {
		TAILQ_REMOVE(&sess->connections, &conn->entry, link);

		portal = find_portal_id(conn->portal.sid.id);
		if (portal != NULL)
			portal->active_connections--;

		init = find_initiator_id(conn->initiator_id);
		if (init != NULL)
			init->active_connections--;

		free(conn);
	}
	free(sess);
}


/*
 * event_kill_connection:
 *    Handle CONNECTION_TERMINATED event: Remove connection from session.
 *
 *    Parameter:
 *          sid         Session ID
 *          cid         Connection ID
 */

void
event_kill_connection(uint32_t sid, uint32_t cid)
{
	session_t *sess;
	connection_t *conn;
	portal_t *portal;
	initiator_t *init;

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

	TAILQ_REMOVE(&sess->connections, &conn->entry, link);
	sess->num_connections--;

	init = find_initiator_id(conn->initiator_id);
	if (init != NULL)
		init->active_connections--;

	UNLOCK_SESSIONS;

	portal = find_portal_id(conn->portal.sid.id);
	if (portal != NULL)
		portal->active_connections--;

	free(conn);
}
