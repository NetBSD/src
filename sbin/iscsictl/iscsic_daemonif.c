/*	$NetBSD: iscsic_daemonif.c,v 1.2 2011/10/30 18:40:06 christos Exp $	*/

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
#include "iscsic_globals.h"

/*
 * do_add_target:
 *    Handle both the add_target and add_send_target commands.
 *
 *    Parameter:
 *          argc, argv  Shifted arguments
 *          kind        Which list
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

STATIC int
do_add_target(int argc, char **argv, iscsid_list_kind_t kind)
{
	iscsid_add_target_req_t *targ;
	iscsid_set_target_authentication_req_t auth;
	iscsid_get_set_target_options_t opt;
	iscsid_response_t *rsp;
	iscsid_add_target_rsp_t *res;
	unsigned		i;
	int opts, auths, tlen;
	uint32_t tid;

	tlen = cl_get_target(&targ, argc, argv, kind != SEND_TARGETS_LIST);
	if (tlen == 0) {
		arg_missing("Target");
	}
	if (kind == SEND_TARGETS_LIST && !targ->num_portals) {
		arg_missing("Target Address");
	}
	opts = cl_get_target_opts(&opt, argc, argv);
	auths = cl_get_auth_opts(&auth, argc, argv);
	cl_get_symname(targ->sym_name, argc, argv);
	check_extra_args(argc, argv);

	targ->list_kind = kind;

	send_request(ISCSID_ADD_TARGET, tlen, targ);
	rsp = get_response(FALSE);

	if (rsp->status) {
		status_error(rsp->status);
	}
	res = (iscsid_add_target_rsp_t *)(void *)rsp->parameter;
	tid = res->target_id;

	if (kind == SEND_TARGETS_LIST) {
		printf("Added Send Target %d\n", res->target_id);
	} else {
		printf("Added Target %d", res->target_id);
		if (res->num_portals) {
			printf(", Portal ");
			for (i = 0; i < res->num_portals; i++)
				printf("%d ", res->portal_id[i]);
		}
		printf("\n");
	}

	free_response(rsp);

	if (opts) {
		opt.list_kind = kind;
		opt.target_id.id = tid;
		send_request(ISCSID_SET_TARGET_OPTIONS, sizeof(opt), &opt);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		}
		free_response(rsp);
	}

	if (auths) {
		auth.list_kind = kind;
		auth.target_id.id = tid;
		send_request(ISCSID_SET_TARGET_AUTHENTICATION, sizeof(auth), &auth);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		}
		free_response(rsp);
	}

	return 0;
}


/*
 * do_remove_target:
 *    Handle both the remove_target and remove_send_target commands.
 *
 *    Parameter:
 *          argc, argv  Shifted arguments
 *          kind        Which list
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

STATIC int
do_remove_target(int argc, char **argv, iscsid_list_kind_t kind)
{
	iscsid_list_id_t req;
	iscsid_search_list_req_t srch;
	iscsid_response_t *rsp;

	if (!cl_get_id('I', &req.id, argc, argv)) {
		if (!cl_get_string('n', (char *)srch.strval, argc, argv)) {
			arg_missing("Target ID or Name");
		}
		check_extra_args(argc, argv);

		srch.search_kind = FIND_TARGET_NAME;
		srch.list_kind = kind;

		send_request(ISCSID_SEARCH_LIST, sizeof(srch), &srch);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error_slist(rsp->status);
		}
		GET_SYM_ID(req.id.id, rsp->parameter);
		free_response(rsp);
	} else {
		check_extra_args(argc, argv);
	}
	req.list_kind = kind;
	send_request(ISCSID_REMOVE_TARGET, sizeof(req), &req);
	rsp = get_response(TRUE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);
	printf("OK\n");
	return 0;
}


/*
 * do_refresh:
 *    Handle the refresh_targets and refresh_isns commands.
 *
 *    Parameter:
 *          argc, argv  Shifted arguments
 *          kind        Which list
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

STATIC int
do_refresh(int argc, char **argv, iscsid_list_kind_t kind)
{
	iscsid_sym_id_t id;
	iscsid_response_t *rsp;
	iscsid_refresh_req_t req;

	req.kind = kind;

	if (cl_get_id('I', &id, argc, argv)) {
		check_extra_args(argc, argv);

		if (!id.id) {
			iscsid_search_list_req_t srch;

			srch.search_kind = FIND_NAME;
			srch.list_kind = kind;
			strlcpy((char *)srch.strval, (char *)id.name,
				sizeof(srch.strval));

			send_request(ISCSID_SEARCH_LIST, sizeof(srch), &srch);
			rsp = get_response(FALSE);
			if (rsp->status) {
				status_error_slist(rsp->status);
			}
			GET_SYM_ID(req.id[0], rsp->parameter);
			free_response(rsp);
		} else {
			req.id[0] = id.id;
		}
		req.num_ids = 1;
	} else {
		req.num_ids = 0;
		check_extra_args(argc, argv);
	}

	req.kind = kind;
	send_request(ISCSID_REFRESH_TARGETS, sizeof(req), &req);

	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	printf("OK\n");
	free_response(rsp);
	return 0;
}


/*
 * add_target:
 *    Handle the add_target command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
add_target(int argc, char **argv)
{
	return do_add_target(argc, argv, TARGET_LIST);
}


/*
 * remove_target:
 *    Handle the remove_target command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
remove_target(int argc, char **argv)
{
	return do_remove_target(argc, argv, TARGET_LIST);
}


/*
 * slp_find_targets:
 *    Handle the slp_find_targets command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 *
 *    ToDo: Implement.
 */

int
/*ARGSUSED*/
slp_find_targets(int argc, char **argv)
{
	printf("Not implemented\n");
	return 5;
}


/*
 * refresh_targets:
 *    Handle the refresh_targets command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
refresh_targets(int argc, char **argv)
{
	return do_refresh(argc, argv, SEND_TARGETS_LIST);
}


/*
 * add_portal:
 *    Handle the add_portal command.
 *
 *    Parameter:
 *          argc, argv  Shifted arguments
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
add_portal(int argc, char **argv)
{
	iscsid_add_portal_req_t port;
	iscsid_response_t *rsp;
	iscsid_add_portal_rsp_t *res;

	if (!cl_get_portal(&port, argc, argv)) {
		arg_missing("Portal Address");
	}
	if (!cl_get_id('I', &port.target_id, argc, argv)) {
		arg_missing("Target ID");
	}
	cl_get_symname(port.sym_name, argc, argv);
	check_extra_args(argc, argv);

	send_request(ISCSID_ADD_PORTAL, sizeof(port), &port);
	rsp = get_response(FALSE);

	if (rsp->status) {
		status_error(rsp->status);
	}
	res = (iscsid_add_portal_rsp_t *)(void *)rsp->parameter;

	printf("Added Portal %d to Target %d\n",
		res->portal_id.id, res->target_id.id);
	free_response(rsp);
	return 0;
}


/*
 * show_target:
 *    Get information about and display a target entry (including its portals).
 *
 *    Parameter:
 *       target ID
 *       list kind
 */

STATIC void
show_target(uint32_t id, iscsid_list_kind_t kind)
{
	iscsid_list_id_t req;
	iscsid_response_t *trsp, *prsp;
	iscsid_get_target_rsp_t *targ;
	iscsid_get_portal_rsp_t *port;
	unsigned i;

	/* get target info */
	req.list_kind = kind;
	req.id.id = id;
	req.id.name[0] = '\0';
	send_request(ISCSID_GET_TARGET_INFO, sizeof(req), &req);

	trsp = get_response(TRUE);
	if (trsp->status) {
		status_error(trsp->status);
	}
	targ = (iscsid_get_target_rsp_t *)(void *)trsp->parameter;

	/* display basic target info */
	printf("%6d", targ->target_id.id);
	if (targ->target_id.name[0]) {
		printf(" [%s]", targ->target_id.name);
	}
	printf(": %s", targ->TargetName);
	if (targ->TargetAlias[0]) {
		printf(" (%s)", targ->TargetAlias);
	}
	printf("\n");

	/* now get and display info on the target's portals */
	for (i = 0; i < targ->num_portals; i++) {
		req.id.id = targ->portal[i];
		send_request(ISCSID_GET_PORTAL_INFO, sizeof(req), &req);
		prsp = get_response(FALSE);
		if (prsp->status) {
			status_error(prsp->status);
		}
		port = (iscsid_get_portal_rsp_t *)(void *)prsp->parameter;

		printf("   %6d", port->portal_id.id);
		if (port->portal_id.name[0]) {
			printf(" [%s]", port->portal_id.name);
		}
		printf(": %s:%d", port->portal.address, port->portal.port);
		if (kind != SEND_TARGETS_LIST) {
			printf(",%d", port->portal.group_tag);
		}
		printf("\n");
		free_response(prsp);
	}
	free_response(trsp);
}


/*
 * do_list_targets:
 *    Handle both the list_targets and list_send_targets commands.
 *
 *    Parameter:
 *          argc, argv  Shifted arguments
 *          kind        Which list
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

STATIC int
do_list_targets(int argc, char **argv, iscsid_list_kind_t kind)
{
	iscsid_get_list_req_t lst;
	iscsid_response_t *rsp;
	iscsid_get_list_rsp_t *list;
	unsigned i;

	check_extra_args(argc, argv);

	/* get the list of targets */
	lst.list_kind = kind;
	send_request(ISCSID_GET_LIST, sizeof(lst), &lst);
	rsp = get_response(TRUE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	list = (iscsid_get_list_rsp_t *)(void *)rsp->parameter;

	/* display all targets */
	for (i = 0; i < list->num_entries; i++) {
		show_target(list->id[i], kind);
	}
	free_response(rsp);
	return 0;
}


/*
 * list_targets:
 *    Handle the list_targets command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
list_targets(int argc, char **argv)
{
	return do_list_targets(argc, argv, TARGET_LIST);
}


/*
 * add_send_target:
 *    Handle the add_target command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
add_send_target(int argc, char **argv)
{
	return do_add_target(argc, argv, SEND_TARGETS_LIST);
}


/*
 * remove_send_target:
 *    Handle the remove_target command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
remove_send_target(int argc, char **argv)
{
	return do_remove_target(argc, argv, SEND_TARGETS_LIST);
}


/*
 * list_send_targets:
 *    Handle the list_send_targets command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
list_send_targets(int argc, char **argv)
{
	return do_list_targets(argc, argv, SEND_TARGETS_LIST);
}


/*
 * add_isns_server:
 *    Handle the add_isns_server command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
add_isns_server(int argc, char **argv)
{
	iscsid_add_isns_server_req_t arg;
	iscsid_add_isns_server_rsp_t *res;
	iscsid_response_t *rsp;

	memset(&arg, 0x0, sizeof(arg));
	if (!(cl_get_isns(&arg, argc, argv))) {
		arg_missing("Server Address");
	}
	check_extra_args(argc, argv);

	send_request(ISCSID_ADD_ISNS_SERVER, sizeof(arg), &arg);
	rsp = get_response(FALSE);

	if (rsp->status) {
		status_error(rsp->status);
	}
	res = (iscsid_add_isns_server_rsp_t *)(void *)rsp->parameter;

	printf("Added iSNS Server ID %d\n", res->server_id);

	free_response(rsp);
	return 0;
}


/*
 * remove_isns_server:
 *    Handle the add_isns_server command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
remove_isns_server(int argc, char **argv)
{
	iscsid_sym_id_t req;
	iscsid_search_list_req_t srch;
	iscsid_response_t *rsp;

	if (!cl_get_id('I', &req, argc, argv)) {
		if (!cl_get_string('a', (char *)srch.strval, argc, argv)) {
			arg_missing("Server Address");
		}
		check_extra_args(argc, argv);
		srch.search_kind = FIND_ADDRESS;
		srch.list_kind = ISNS_LIST;

		send_request(ISCSID_SEARCH_LIST, sizeof(srch), &srch);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error_slist(rsp->status);
		}
		GET_SYM_ID(req.id, rsp->parameter);
		free_response(rsp);
	} else {
		check_extra_args(argc, argv);
	}
	send_request(ISCSID_REMOVE_ISNS_SERVER, sizeof(req), &req);
	rsp = get_response(TRUE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);

	printf("OK\n");
	return 0;
}


/*
 * find_isns_servers:
 *    Handle the find_isns_servers command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
/*ARGSUSED*/
find_isns_servers(int argc, char **argv)
{
	printf("Not implemented\n");
	return 5;
}


/*
 * list_isns_servers:
 *    Handle the list_isns_servers command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
list_isns_servers(int argc, char **argv)
{
	iscsid_get_list_req_t lst;
	iscsid_response_t *rsp, *prsp;
	iscsid_get_list_rsp_t *list;
	iscsid_get_isns_server_rsp_t *isns;
	iscsid_sym_id_t req;
	unsigned i;

	memset(&req, 0x0, sizeof(req));
	check_extra_args(argc, argv);

	/* get the list of servers */
	lst.list_kind = ISNS_LIST;
	send_request(ISCSID_GET_LIST, sizeof(lst), &lst);
	rsp = get_response(TRUE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	list = (iscsid_get_list_rsp_t *)(void *)rsp->parameter;

	/* display all servers */
	for (i = 0; i < list->num_entries; i++) {
		req.id = list->id[i];
		send_request(ISCSID_GET_ISNS_SERVER, sizeof(req), &req);
		prsp = get_response(FALSE);
		if (prsp->status)
			status_error(prsp->status);

		isns = (iscsid_get_isns_server_rsp_t *)(void *)prsp->parameter;
		printf("%6d: %s\n", list->id[i], isns->address);
		free_response(prsp);
	}
	free_response(rsp);
	return 0;
}


/*
 * refresh_isns:
 *    Handle the refresh_isns command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
refresh_isns(int argc, char **argv)
{
	return do_refresh(argc, argv, ISNS_LIST);
}


/*
 * add_initiator:
 *    Handle the add_initiator command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
add_initiator(int argc, char **argv)
{
	iscsid_add_initiator_req_t req;
	iscsid_add_initiator_rsp_t *res;
	iscsid_response_t *rsp;

	memset(&req, 0x0, sizeof(req));
	if (!cl_get_string('a', (char *)req.address, argc, argv)) {
		arg_missing("Interface Address");
	}
	cl_get_symname(req.name, argc, argv);
	check_extra_args(argc, argv);

	send_request(ISCSID_ADD_INITIATOR_PORTAL, sizeof(req), &req);
	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	res = (iscsid_add_initiator_rsp_t *)(void *)rsp->parameter;
	printf("Added Initiator Portal %d\n", res->portal_id);

	free_response(rsp);
	return 0;
}


/*
 * remove_initiator:
 *    Handle the remove_initiator command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
remove_initiator(int argc, char **argv)
{
	iscsid_sym_id_t req;
	iscsid_response_t *rsp;

	memset(&req, 0x0, sizeof(req));
	if (!cl_get_id('I', &req, argc, argv)) {
		arg_missing("Initiator Portal ID");
	}
	check_extra_args(argc, argv);

	send_request(ISCSID_REMOVE_INITIATOR_PORTAL, sizeof(req), &req);
	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);

	printf("OK\n");
	return 0;
}

/*
 * list_initiators:
 *    Handle the list_initiators command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
list_initiators(int argc, char **argv)
{
	iscsid_get_list_req_t lst;
	iscsid_response_t *rsp, *prsp;
	iscsid_get_list_rsp_t *list;
	iscsid_get_initiator_rsp_t *init;
	iscsid_sym_id_t req;
	unsigned i;

	memset(&req, 0x0, sizeof(req));
	check_extra_args(argc, argv);

	/* get the list of servers */
	lst.list_kind = INITIATOR_LIST;
	send_request(ISCSID_GET_LIST, sizeof(lst), &lst);
	rsp = get_response(TRUE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	list = (iscsid_get_list_rsp_t *)(void *)rsp->parameter;

	/* display all servers */
	for (i = 0; i < list->num_entries; i++) {
		req.id = list->id[i];
		send_request(ISCSID_GET_INITIATOR_PORTAL, sizeof(req), &req);
		prsp = get_response(FALSE);
		if (prsp->status) {
			status_error(prsp->status);
		}
		init = (iscsid_get_initiator_rsp_t *)(void *)prsp->parameter;
		printf("%6d", init->portal_id.id);
		if (init->portal_id.name[0]) {
			printf("[%s]", init->portal_id.name);
		}
		printf(": %s\n", init->address);
		free_response(prsp);
	}
	free_response(rsp);
	return 0;
}


/*
 * login_or_add:
 *    Common routine for login and add_connection. Handles the actual command,
 *    the only pre-initialized component is the session ID in the login request
 *    for add_connection.
 *
 *    Parameters: the login request, plus argc & argv
*/

STATIC void
login_or_add(iscsid_login_req_t * loginp, int argc, char **argv)
{
	iscsid_add_target_req_t *targ = NULL;
	iscsid_add_target_rsp_t	*tgtrsp;
	iscsid_set_target_authentication_req_t auth;
	iscsid_get_set_target_options_t opt;
	iscsid_search_list_req_t srch;
	iscsid_response_t *rsp;
	int opts, auths, tlen;

	/* get the remaining parameters */
	opts = cl_get_target_opts(&opt, argc, argv);
	auths = cl_get_auth_opts(&auth, argc, argv);
	cl_get_symname(loginp->sym_name, argc, argv);
	cl_get_id ('i', &loginp->initiator_id, argc, argv);

	/* do we have a portal ID? */
	if (!cl_get_id('P', &loginp->portal_id, argc, argv)) {
		/* No portal ID - then we must have a target name */
		if (!(tlen = cl_get_target(&targ, argc, argv, TRUE))) {
			if (NO_ID(&loginp->session_id)) {
				arg_missing("Target");
			}
		}

		check_extra_args(argc, argv);

		if (targ != NULL) {
			/* was a complete target with portal specified? */
			if (targ->num_portals) {
				/* then add it like in an add_target command */
				targ->list_kind = TARGET_LIST;
				send_request(ISCSID_ADD_TARGET, tlen, targ);
				rsp = get_response(FALSE);
				if (rsp->status) {
					status_error(rsp->status);
				}
				tgtrsp = (iscsid_add_target_rsp_t *)(void *)(rsp->parameter);
				(void) memcpy(&loginp->portal_id.id, &tgtrsp->target_id,
					sizeof(loginp->portal_id.id));
			} else {
				/* else find the target by its TargetName */
				srch.search_kind = FIND_TARGET_NAME;
				srch.list_kind = TARGET_LIST;
				strlcpy((char *)srch.strval, (char *)targ->TargetName,
					sizeof(srch.strval));

				send_request(ISCSID_SEARCH_LIST, sizeof(srch), &srch);
				rsp = get_response(FALSE);
				if (rsp->status) {
					status_error_slist(rsp->status);
				}
				GET_SYM_ID(loginp->portal_id.id, rsp->parameter);
			}
			free_response(rsp);
		}
	} else
		check_extra_args(argc, argv);

	/* Options specified? Then set them */
	if (opts) {
		opt.target_id = loginp->portal_id;
		opt.list_kind = TARGET_LIST;
		send_request(ISCSID_SET_TARGET_OPTIONS, sizeof(opt), &opt);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		}
		free_response(rsp);
	}
	/* Authentication options specified? Then set them */
	if (auths) {
		auth.target_id = loginp->portal_id;
		auth.list_kind = TARGET_LIST;
		send_request(ISCSID_SET_TARGET_AUTHENTICATION, sizeof(auth), &auth);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		}
		free_response(rsp);
	}
}


/*
 * login:
 *    Handle login command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
*/

int
login(int argc, char **argv)
{
	iscsid_login_req_t loginp;
	iscsid_login_rsp_t *res;
	iscsid_response_t *rsp;

	memset(&loginp, 0x0, sizeof(loginp));
	loginp.login_type = (cl_get_opt('m', argc, argv))
						? ISCSI_LOGINTYPE_NOMAP : ISCSI_LOGINTYPE_MAP;

	login_or_add(&loginp, argc, argv);

	send_request(ISCSID_LOGIN, sizeof(loginp), &loginp);

	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	res = (iscsid_login_rsp_t *)(void *)rsp->parameter;
	printf("Created Session %d, Connection %d\n",
		res->session_id.id, res->connection_id.id);

	free_response(rsp);
	return 0;
}


/*
 * logout:
 *    Handle logout command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
*/

int
logout(int argc, char **argv)
{
	iscsid_sym_id_t req;
	iscsid_response_t *rsp;

	if (!cl_get_id('I', &req, argc, argv)) {
		arg_missing("Session");
	}
	check_extra_args(argc, argv);

	send_request(ISCSID_LOGOUT, sizeof(req), &req);
	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);

	printf("OK\n");
	return 0;
}


/*
 * add_connection:
 *    Handle add_connection command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
*/

int
add_connection(int argc, char **argv)
{
	iscsid_login_req_t loginp;
	iscsid_login_rsp_t	*loginrsp;
	iscsid_response_t *rsp;

	memset(&loginp, 0x0, sizeof(loginp));
	loginp.login_type = ISCSI_LOGINTYPE_MAP;
	cl_get_id('I', &loginp.session_id, argc, argv);

	login_or_add(&loginp, argc, argv);

	send_request(ISCSID_ADD_CONNECTION, sizeof(loginp), &loginp);

	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	loginrsp = (iscsid_login_rsp_t *)(void *)(rsp->parameter);
	printf("Added Connection %d\n", loginrsp->connection_id.id);

	free_response(rsp);
	return 0;
}


/*
 * remove_connection:
 *    Handle remove connection command
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
*/

int
remove_connection(int argc, char **argv)
{
	iscsid_remove_connection_req_t req;
	iscsid_response_t *rsp;

	if (!cl_get_id('I', &req.session_id, argc, argv)) {
		arg_missing("Session");
	}
	if (!cl_get_id('C', &req.connection_id, argc, argv)) {
		arg_missing("Connection");
	}
	check_extra_args(argc, argv);

	send_request(ISCSID_REMOVE_CONNECTION, sizeof(req), &req);
	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);

	printf("OK\n");
	return 0;
}


/*
 * list_sessions:
 *    Handle list_sessions command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 *
 * ToDo: initiator portals, connection recurse
*/

int
list_sessions(int argc, char **argv)
{
	iscsid_response_t *lrsp, *rsp;
	iscsid_get_session_list_rsp_t *list;
	iscsid_session_list_entry_t *ent;
	iscsid_get_connection_info_req_t creq;
	iscsid_get_connection_info_rsp_t *info;
	iscsid_sym_id_t clreq;
	iscsid_get_connection_list_rsp_t *clinfo;
	iscsid_connection_list_entry_t *clent;
	unsigned i;
	unsigned j;
	int lconn;

	memset(&creq, 0x0, sizeof(creq));
	memset(&clreq, 0x0, sizeof(clreq));
	lconn = cl_get_opt('c', argc, argv);

	check_extra_args(argc, argv);

	send_request(ISCSID_GET_SESSION_LIST, 0, NULL);
	lrsp = get_response(TRUE);
	if (lrsp->status) {
		status_error(lrsp->status);
	}
	list = (iscsid_get_session_list_rsp_t *)(void *)lrsp->parameter;

	for (i = 0, ent = list->session; i < list->num_entries; i++, ent++) {
		creq.session_id.id = ent->session_id.id;
		send_request(ISCSID_GET_CONNECTION_INFO, sizeof(creq), &creq);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		}
		info = (iscsid_get_connection_info_rsp_t *)(void *)rsp->parameter;

		printf("Session %d", info->session_id.id);
		if (info->session_id.name[0]) {
			printf("[%s]", info->session_id.name);
		}
		printf(": Target %s", info->TargetName);
		if (info->target_portal_id.name[0]) {
			printf(" [%s]", info->target_portal_id.name);
		}
		printf("\n");

		if (lconn) {
			clreq.id = info->session_id.id;
			send_request(ISCSID_GET_CONNECTION_LIST, sizeof(clreq), &clreq);
			rsp = get_response(FALSE);
			if (rsp->status) {
				status_error(rsp->status);
			}
			clinfo = (iscsid_get_connection_list_rsp_t *)(void *)rsp->parameter;

			for (j = 0, clent = clinfo->connection;
				 j < clinfo->num_connections; j++, clent++) {
				printf(" Connection %d", clent->connection_id.id);
				if (clent->connection_id.name[0]) {
					printf("[%s]", clent->connection_id.name);
				}
				printf(" Address %s:%d,%d\n", clent->target_portal.address,
					clent->target_portal.port,
					clent->target_portal.group_tag);
			}
		}
	}

	return 0;
}


/*
 * set_node_name:
 *    Handle the set_node_name command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
set_node_name(int argc, char **argv)
{
	iscsid_set_node_name_req_t req;
	iscsid_response_t *rsp;
	uint64_t isid;

	memset(&req, 0x0, sizeof(req));
	if (!cl_get_string('n', (char *)req.InitiatorName, argc, argv)) {
		arg_missing("Initiator Name");
	}
	cl_get_string('A', (char *)req.InitiatorAlias, argc, argv);
	isid = cl_get_longlong('i', argc, argv);
	hton6(req.ISID, isid);

	check_extra_args(argc, argv);

	send_request(ISCSID_SET_NODE_NAME, sizeof(req), &req);

	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	free_response(rsp);

	printf("OK\n");
	return 0;
}

/*
 * get_version:
 *    Handle the version command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
/*ARGSUSED*/
get_version(int argc, char **argv)
{
	iscsid_response_t *rsp;
	iscsid_get_version_rsp_t *ver;

	send_request(ISCSID_GET_VERSION, 0, NULL);
	rsp = get_response(FALSE);
	if (rsp->status) {
		status_error(rsp->status);
	}
	ver = (iscsid_get_version_rsp_t *)(void *)rsp->parameter;
	printf("%s\n%s\n%s\n", VERSION_STRING, ver->version_string,
		   ver->driver_version_string);

	return 0;
}


#ifdef ISCSI_DEBUG
/*
 * kill_daemon:
 *    Handle kill_daemon command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    Always 0.
*/

int
kill_daemon(int argc, char **argv)
{
	check_extra_args(argc, argv);

	send_request(ISCSID_DAEMON_TERMINATE, 0, NULL);
	return 0;
}
#endif
