/*	$NetBSD: iscsid_targets.c,v 1.5 2012/12/29 08:28:20 mlelstv Exp $	*/

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

#include <ctype.h>

/* counter for portal and target ID */
static uint32_t portarget_id = 0;

/* counter for send_targets ID */
static uint32_t send_target_id = 0;


/*
 * create_portal:
 *    Create a portal entry and link it into the appropriate lists.
 *    May also create the associated portal group entry if it does not exist.
 *    Will return the existing entry if the address matches a defined portal.
 *
 *    Parameter:
 *       target   the pointer to the target
 *       addr     the portal address (includes tag)
 *		 dtype    portal discovery type
 *		 did	  discovery ID
 *
 *    Returns:    pointer to created portal
 */

static portal_t *
create_portal(target_t *target, iscsi_portal_address_t *addr,
			  iscsi_portal_types_t dtype, uint32_t did)
{
	portal_group_t *curr;
	portal_t *portal;
	u_short tag = addr->group_tag;

	DEB(9, ("Create Portal addr %s port %d group %d\n",
			addr->address, addr->port, addr->group_tag));

	if ((portal = find_portal_by_addr(target, addr)) != NULL)
		return portal;

	portal = calloc(1, sizeof(*portal));
	if (!portal) {
		DEBOUT(("Out of memory in create_portal!\n"));
		return NULL;
	}
	portal->addr = *addr;
	portal->target = target;
	portal->portaltype = dtype;
	portal->discoveryid = did;
	if (!portal->addr.port) {
		portal->addr.port = ISCSI_DEFAULT_PORT;
	}
	for (portarget_id++; !portarget_id ||
	     find_portal_id(portarget_id) != NULL ||
	     find_target_id(TARGET_LIST, portarget_id) != NULL;) {
		portarget_id++;
	}
	portal->entry.sid.id = portarget_id;

	TAILQ_FOREACH(curr, &target->group_list, groups)
		if (curr->tag == tag)
			break;

	if (!curr) {
		curr = calloc(1, sizeof(*curr));
		if (!curr) {
			free(portal);
			DEBOUT(("Out of memory in create_portal!\n"));
			return NULL;
		}
		curr->tag = tag;
		TAILQ_INIT(&curr->portals);
		TAILQ_INSERT_TAIL(&target->group_list, curr, groups);
		target->num_groups++;
	}

	portal->group = curr;

	TAILQ_INSERT_TAIL(&curr->portals, portal, group_list);
	curr->num_portals++;
	target->num_portals++;

	TAILQ_INSERT_TAIL(&list[PORTAL_LIST].list, &portal->entry, link);
	list[PORTAL_LIST].num_entries++;

	DEB(9, ("create_portal returns %p\n", portal));
	return portal;
}


/*
 * delete_portal:
 *    Delete a portal entry after unlinking it from its lists.
 *    May also delete the associated portal group entry if the group is empty.
 *
 *    Parameter:
 *       portal   		the pointer to the portal
 *		 delete_empty   delete empty target if true
 */

void
delete_portal(portal_t * portal, boolean_t delete_empty)
{
	portal_group_t *curr = portal->group;
	target_t *target = portal->target;

	TAILQ_REMOVE(&curr->portals, portal, group_list);
	TAILQ_REMOVE(&list[PORTAL_LIST].list, &portal->entry, link);
	curr->num_portals--;
	target->num_portals--;
	list[PORTAL_LIST].num_entries--;

	if (!curr->num_portals) {
		TAILQ_REMOVE(&target->group_list, curr, groups);
		free(curr);
		target->num_groups--;
	}
	free(portal);

	/* Optionally delete target if no portals left */
	if (delete_empty && !target->num_portals) {
		TAILQ_REMOVE(&list[TARGET_LIST].list, &target->entry, link);
		list[TARGET_LIST].num_entries--;
		free(target);
	}
}


/*
 * create_target:
 *    Create a target structure and initialize it.
 *
 *    Parameter:
 *          name        The target name
 *
 *    Returns:    Pointer to target structure, NULL if allocation failed.
 */

static target_t *
create_target(uint8_t * name)
{
	target_t *target;

	DEB(9, ("Create Target %s\n", name));

	if ((target = calloc(1, sizeof(*target))) == NULL) {
		DEBOUT(("Out of memory in create_target!\n"));
		return NULL;
	}

	TAILQ_INIT(&target->group_list);

	for (portarget_id++;
	     !portarget_id ||
	     find_portal_id(portarget_id) != NULL ||
	     find_target_id(TARGET_LIST, portarget_id) != NULL;
	     portarget_id++) {
	}

	target->entry.sid.id = portarget_id;
	strlcpy((char *)target->TargetName, (char *)name, sizeof(target->TargetName));

	return target;
}


/*
 * delete_target:
 *    Delete a target entry after unlinking it from its lists.
 *    Also deletes all portals associated with the target.
 *
 *    Parameter:
 *       target   the pointer to the target
 */

static void
delete_target(target_t * target)
{
	portal_group_t *cgroup;
	portal_t *curr = NULL;

	/* First delete all portals in all portal groups. */
	/* (this will also delete the groups) */
	while (target->num_groups) {
		cgroup = TAILQ_FIRST(&target->group_list);
		while (cgroup && cgroup->num_portals) {
			curr = TAILQ_FIRST(&cgroup->portals);
			if (curr)
				delete_portal(curr, FALSE);
		}
	}

	/*Now delete the target itself */
	TAILQ_REMOVE(&list[TARGET_LIST].list, &target->entry, link);
	list[TARGET_LIST].num_entries--;
	free(target);
}


/*
 * create_send_target:
 *    Create a send_target structure and initialize it.
 *
 *    Parameter:
 *          name        The target name
 *          addr        The portal address
 *
 *    Returns:    Pointer to structure, NULL if allocation failed.
 */

static target_t *
create_send_target(uint8_t * name, iscsi_portal_address_t * addr)
{
	send_target_t *target;

	DEB(9, ("Create Send Target %s\n", name));

	if ((target = calloc(1, sizeof(*target))) == NULL)
		return NULL;

	for (send_target_id++;
		 !send_target_id
		 || find_target_id(SEND_TARGETS_LIST, send_target_id) != NULL;)
		send_target_id++;

	target->entry.sid.id = send_target_id;
	strlcpy((char *)target->TargetName, (char *)name, sizeof(target->TargetName));
	target->addr = *addr;
	target->num_groups = 1;
	target->num_portals = 1;

	return (target_t *)(void *)target;
}

/*
 * delete_send_target:
 *    Delete a send_target entry after unlinking it from its lists.
 *
 *    Parameter:
 *       send_target   the pointer to the send_target
 */

static void
delete_send_target(send_target_t * send_target)
{
	generic_entry_t *curr;
	uint32_t id;

	id = send_target->entry.sid.id;

	TAILQ_FOREACH(curr, &list[PORTAL_LIST].list, link) {
		portal_t *p = (void *)curr;
		if (p->portaltype == PORTAL_TYPE_SENDTARGET &&
			p->discoveryid == id)
			p->discoveryid = 0; /* mark deleted */
	}

	TAILQ_REMOVE(&list[SEND_TARGETS_LIST].list, &send_target->entry, link);
	list[SEND_TARGETS_LIST].num_entries--;
	free(send_target);
}



/*
 * add_target:
 *    Handle ADD_TARGET request: Create a target or send_target and its
 *    associated portals.
 *    This routine allows the same target to be defined more than once,
 *    adding any missing data (for example additional portals).
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */


void
add_target(iscsid_add_target_req_t *par, iscsid_response_t **prsp,
			int *prsp_temp)
{
	iscsid_add_target_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	target_t *target, *tn;
	portal_t *portal;
	int i, num;

	DEB(9, ("Add Target, name %s, num_portals %d\n",
			par->TargetName, par->num_portals));

	if (par->list_kind == SEND_TARGETS_LIST && par->num_portals != 1) {
		rsp->status = ISCSID_STATUS_PARAMETER_INVALID;
		return;
	}
	/* check to see if the target already exists */
	if ((par->TargetName[0] &&
	     (target = find_TargetName(par->list_kind, par->TargetName)) != NULL) ||
	    (par->list_kind == SEND_TARGETS_LIST &&
	     (target = (target_t *)(void *)
			find_send_target_by_addr(&par->portal[0])) != NULL)) {
		num = target->num_portals;

		/* symbolic name? */
		if (par->sym_name[0]) {
			/* already named? rename if OK */
			tn = find_target_symname(par->list_kind, par->sym_name);
			if (tn && tn != target) {
				rsp->status = ISCSID_STATUS_DUPLICATE_NAME;
				return;
			}
			strlcpy((char *)target->entry.sid.name, (char *)par->sym_name, sizeof(target->entry.sid.name));
		}
	} else {
		if (par->sym_name[0] &&
			(find_target_symname(par->list_kind, par->sym_name) ||
			 find_portal_name(par->sym_name))) {
			rsp->status = ISCSID_STATUS_DUPLICATE_NAME;
			return;
		}

		if (par->list_kind == SEND_TARGETS_LIST)
			target = create_send_target(par->TargetName, &par->portal[0]);
		else
			target = create_target(par->TargetName);

		if (target == NULL) {
			rsp->status = ISCSID_STATUS_NO_RESOURCES;
			return;
		}
		num = 0;
		strlcpy((char *)target->entry.sid.name, (char *)par->sym_name,
			sizeof(target->entry.sid.name));
	}

	rsp = make_rsp(sizeof(*res) + (par->num_portals * sizeof(uint32_t)),
			prsp, prsp_temp);
	if (rsp == NULL)
		return;

	res = (iscsid_add_target_rsp_t *)(void *)rsp->parameter;
	res->target_id = target->entry.sid.id;

	/* link into target list */
	if (!num) {
		TAILQ_INSERT_TAIL(&list[par->list_kind].list, &target->entry,
			link);
		list[par->list_kind].num_entries++;
	}

	/*
	   Add the given portals. Note that create_portal also checks for
	   duplicate entries, and returns the pointer to the existing entry
	   if the request is a duplicate.
	 */

	if (par->list_kind == SEND_TARGETS_LIST) {
		res->portal_id[0] = target->entry.sid.id;
		res->num_portals = 1;
	} else {
		for (i = 0; i < (int)par->num_portals; i++) {
			portal = create_portal(target, &par->portal[i],
					PORTAL_TYPE_STATIC,
					target->entry.sid.id);
			if (portal == NULL) {
				rsp->status = ISCSID_STATUS_NO_RESOURCES;
				break;
			}
			res->portal_id[i] = portal->entry.sid.id;
		}
		res->num_portals = i;
	}

	DEB(9, ("AddTarget returns\n"));
}


/*
 * add_discovered_target:
 *    Check whether the given target and portal already exist.
 *    If not, add them.
 *
 *    Parameter:
 *          TargetName
 *          portal
 *          dtype = type of portal added: PORTAL_TYPE_SENDTARGET or
 *					PORTAL_TYPE_ISNS
 *          did = ID of SendTargets or iSNS for which portal was discovered
 *
 *    Returns: Pointer to created target, NULL on error (out of memory)
 *    Always sets portaltype to dtype even if portal already exists
 *      (used for refreshing to mark portals that we find)
 */

target_t *
add_discovered_target(uint8_t * TargetName, iscsi_portal_address_t * addr,
				iscsi_portal_types_t dtype, uint32_t did)
{
	target_t *target;
	portal_t *portal;

	DEB(9, ("Add Discovered Target, name %s, addr %s\n",
			TargetName, addr->address));

	if ((target = find_TargetName(TARGET_LIST, TargetName)) == NULL) {
		if ((target = create_target(TargetName)) == NULL) {
			return NULL;
		}
		portal = create_portal(target, addr, dtype, did);
		if (portal == NULL) {
			free(target);
			return NULL;
		}
		TAILQ_INSERT_TAIL(&list[TARGET_LIST].list, &target->entry, link);
		list[TARGET_LIST].num_entries++;
	} else if ((portal = create_portal(target, addr, dtype, did)) == NULL) {
		return NULL;
	}
	portal->portaltype = dtype;

	return target;
}


/*
 * set_target_options:
 *    Handle SET_TARGET_OPTIONS request: Copy the given options into the
 *    target structure.
 *
 *    Parameter:
 *          par         The request parameters.
 *
 *    Returns:     status
 */

uint32_t
set_target_options(iscsid_get_set_target_options_t * par)
{
	target_t *target;

	if ((target = find_target(par->list_kind, &par->target_id)) == NULL)
		return ISCSID_STATUS_INVALID_TARGET_ID;

	target->options = *par;

	return ISCSID_STATUS_SUCCESS;
}


/*
 * set_target_auth:
 *    Handle SET_TARGET_AUTHENTICATION request: Copy the given options into the
 *    target structure.
 *
 *    Parameter:
 *          par         The request parameters.
 *
 *    Returns:     status
 */

uint32_t
set_target_auth(iscsid_set_target_authentication_req_t * par)
{
	target_t *target;

	if ((target = find_target(par->list_kind, &par->target_id)) == NULL) {
		return ISCSID_STATUS_INVALID_TARGET_ID;
	}
	target->auth = *par;

	return ISCSID_STATUS_SUCCESS;
}


/*
 * get_target_info:
 *    Handle GET_TARGET_INFO request: Return information about the given
 *    target and its portals. If a portal ID is given, returns only the
 *    target info and the ID of this portal.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_target_info(iscsid_list_id_t *par, iscsid_response_t **prsp, int *prsp_temp)
{
	iscsid_get_target_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	uint32_t *idp;
	target_t *target;
	portal_group_t *cgroup;
	portal_t *curr = NULL;
	int num = 1;

	DEB(10, ("get_target_info, id %d\n", par->id.id));

	if ((target = find_target(par->list_kind, &par->id)) == NULL) {
		if (par->list_kind == SEND_TARGETS_LIST ||
		    (curr = find_portal(&par->id)) == NULL) {
			rsp->status = ISCSID_STATUS_INVALID_TARGET_ID;
			return;
		}
		target = curr->target;
	} else if (par->list_kind != SEND_TARGETS_LIST) {
		num = target->num_portals;
	}
	rsp = make_rsp(sizeof(*res) + (num - 1) * sizeof(uint32_t),
			prsp, prsp_temp);
	if (rsp == NULL)
		return;

	res = (iscsid_get_target_rsp_t *)(void *)rsp->parameter;
	res->target_id = target->entry.sid;
	strlcpy((char *)res->TargetName, (char *)target->TargetName,
			sizeof(res->TargetName));
	strlcpy((char *)res->TargetAlias, (char *)target->TargetAlias,
			sizeof(res->TargetAlias));

	res->num_portals = num;
	idp = res->portal;

	if (curr) {
		*idp = curr->entry.sid.id;
	} else if (par->list_kind != SEND_TARGETS_LIST) {
		TAILQ_FOREACH(cgroup, &target->group_list, groups)
			TAILQ_FOREACH(curr, &cgroup->portals, group_list)
			* idp++ = curr->entry.sid.id;
	} else
		*idp = target->entry.sid.id;
}


/*
 * add_portal:
 *    Handle ADD_PORTAL request: Add a portal to an existing target.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */


void
add_portal(iscsid_add_portal_req_t *par, iscsid_response_t **prsp,
			int *prsp_temp)
{
	iscsid_add_portal_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	target_t *target;
	portal_t *portal;

	DEB(9, ("Add portal: target %d (%s), symname %s, addr %s\n",
			par->target_id.id, par->target_id.name,
			par->sym_name, par->portal.address));

	if ((target = find_target(TARGET_LIST, &par->target_id)) == NULL) {
		rsp->status = ISCSID_STATUS_INVALID_TARGET_ID;
		return;
	}

	if (par->sym_name[0] &&
		(find_target_symname(TARGET_LIST, par->sym_name) ||
		 find_portal_name(par->sym_name))) {
		rsp->status = ISCSID_STATUS_DUPLICATE_NAME;
		return;
	}

	portal = create_portal(target, &par->portal, PORTAL_TYPE_STATIC,
							target->entry.sid.id);
	if (portal == NULL) {
		rsp->status = ISCSID_STATUS_NO_RESOURCES;
		return;
	}

	if (par->sym_name[0]) {
		strlcpy((char *)portal->entry.sid.name, (char *)par->sym_name,
			sizeof(portal->entry.sid.name));
	}
	portal->options = par->options;

	rsp = make_rsp(sizeof(*res), prsp, prsp_temp);
	if (rsp == NULL)
		return;
#if 0 /*XXX: christos res is uninitialized here?? */
	res->target_id = target->entry.sid;
	res->portal_id = portal->entry.sid;
#endif

	DEB(9, ("AddPortal success (id %d)\n", portal->entry.sid.id));
}


/*
 * get_portal_info:
 *    Handle GET_PORTAL_INFO request: Return information about the given
 *    portal.
 *
 *    Parameter:
 *          par         The request parameters.
 *          prsp        Pointer to address of response buffer.
 *          prsp_temp   Will be set to TRUE if buffer was allocated, FALSE
 *                      for static buffer.
 */

void
get_portal_info(iscsid_list_id_t *par, iscsid_response_t **prsp, int *prsp_temp)
{
	iscsid_get_portal_rsp_t *res;
	iscsid_response_t *rsp = *prsp;
	portal_t *portal = NULL;
	send_target_t *starg = NULL;
	int err;

	DEB(10, ("get_portal_info, id %d\n", par->id.id));

	if (par->list_kind == SEND_TARGETS_LIST)
		err = ((starg = (send_target_t *)(void *)find_target(par->list_kind,
							&par->id)) == NULL);
	else
		err = ((portal = find_portal(&par->id)) == NULL);

	if (err) {
		rsp->status = ISCSID_STATUS_INVALID_PORTAL_ID;
		return;
	}

	rsp = make_rsp(sizeof(*res), prsp, prsp_temp);
	if (rsp == NULL)
		return;

	res = (iscsid_get_portal_rsp_t *)(void *)rsp->parameter;
	if (par->list_kind == SEND_TARGETS_LIST) {
		res->target_id = starg->entry.sid;
		res->portal_id = starg->entry.sid;
		res->portal = starg->addr;
	} else {
		res->target_id = portal->target->entry.sid;
		res->portal_id = portal->entry.sid;
		res->portal = portal->addr;
	}
}

/*
 * remove_target:
 *    Handle REMOVE_TARGET request: Removes a target, target portal,
 *		or Send-Targets portal from its respective list.
 *      Removing a target will remove all associated portals.
 *
 *    Parameter:
 *          par         The request parameters = iscsid_list_id_t
 *						containing the target ID.
 *
 *    Returns:     status
 */

uint32_t
remove_target(iscsid_list_id_t * par)
{
	target_t *target = NULL;
	portal_t *portal = NULL;
	send_target_t *starg = NULL;
	int err;

	DEB(9, ("remove_target, id %d\n", par->id.id));

	if (par->list_kind == SEND_TARGETS_LIST) {
		err = ((starg = (send_target_t *)(void *)find_target(par->list_kind,
							&par->id)) == NULL);
		if (!err) {
			delete_send_target(starg);
		}
	} else if (par->list_kind == PORTAL_LIST) {
		err = ((portal = find_portal(&par->id)) == NULL);
		if (!err) {
			delete_portal(portal, FALSE);
		}
	} else {
		target = find_target(par->list_kind, &par->id);
		err = (target == NULL);
		if (!err) {
			delete_target(target);
		}
	}

	return err ? ISCSID_STATUS_INVALID_PORTAL_ID : ISCSID_STATUS_SUCCESS;
}



/*
 * cl_get_address:
 *    Get an address specification that may include port and group tag.
 *
 *    Parameter:
 *          portal   The portal address
 *          str      The parameter string to scan
 *
 *    Returns:    0 on error, 1 if OK.
 */

static int
cl_get_address(iscsi_portal_address_t * portal, char *str)
{
	char *sp, *sp2;
	int val;

	/* is there a port? don't check inside square brackets (IPv6 addr) */
	for (sp = str + 1, val = 0; *sp && (*sp != ':' || val); sp++) {
		if (*sp == '[')
			val = 1;
		else if (*sp == ']')
			val = 0;
	}

	/* */
	if (*sp) {
		for (sp2 = sp + 1; *sp2 && *sp2 != ':'; sp2++);
		/* if there's a second colon, assume it's an unbracketed IPv6
		 * address */
		if (!*sp2) {
			/* truncate source, that's the address */
			*sp++ = '\0';
			if (sscanf(sp, "%d", &val) != 1)
				return 0;
			if (val < 0 || val > 0xffff)
				return 0;
			portal->port = (uint16_t) val;
		}
		/* is there a group tag? */
		for (; isdigit((unsigned char)*sp); sp++) {
		}
		if (*sp && *sp != ',')
			return 0;
	} else
		for (sp = str + 1; *sp && *sp != ','; sp++);

	if (*sp) {
		if (sscanf(sp + 1, "%d", &val) != 1)
			return 0;
		if (val < 0 || val > 0xffff)
			return 0;
		portal->group_tag = (uint16_t) val;
		/* truncate source, that's the address */
		*sp = '\0';
	}
	/* only check length, don't verify correct format (too many
	 * possibilities) */
	if (strlen(str) >= sizeof(portal->address))
		return 0;

	strlcpy((char *)portal->address, str, sizeof(portal->address));

	return 1;
}

/*
 * refresh_send_target:
 *    Handle REFRESH_TARGETS request for a Send Target
 *
 *    Parameter:
 *          id    The send target ID.
 *
 *    Returns:     status
 */


static uint32_t
refresh_send_target(uint32_t id)
{
	uint8_t *response_buffer = NULL;
	uint32_t response_size;
	uint32_t ret;
	uint8_t *TargetName;
	iscsi_portal_address_t addr;
	uint8_t *tp, *sp;
	generic_entry_t *curr;
	generic_entry_t *next;
	send_target_t *sendtarg;
	int rc;

	/*
	 * Go through our list of portals and mark each one
	 * belonging to the current sendtargets group to refreshing
	 * This mark is used afterwards to remove any portals that
	 * were not refreshed (because the mark gets reset for portals
	 * that are refreshed).
	 */

	TAILQ_FOREACH(curr, &list[PORTAL_LIST].list, link) {
		portal_t *p = (void *)curr;
		if (p->portaltype == PORTAL_TYPE_SENDTARGET &&
		    p->discoveryid == id) {
			p->portaltype = PORTAL_TYPE_REFRESHING;
		}
	}

	if ((ret = send_targets(id, &response_buffer, &response_size)) == 0) {
		/*
		 * Parse the response target list
		 * The SendTargets response in response_buffer is a list of
		 * target strings. Each target string consists of a TargetName
		 * string, followed by 0 or more TargetAddress strings:
		 *
		 *      TargetName=<target-name>
		 *      TargetAddress=<hostname-or-ip>[:<tcp-port>],
		 *				<portal-group-tag>
		 * The entire list is terminated by a null (after
		 * response_size bytes) (this terminating NULL was placed
		 * there by send_targets routine.)
		 */

		tp = response_buffer;
		while (*tp) {
			if (strncmp((char *)tp, "TargetName=", 11) != 0) {
				DEBOUT(("Response not TargetName <%s>\n", tp));
				break;
			}
			tp += 11;
			TargetName = tp; /*Point to target name */
			while (*tp++) {
			}
			rc = -1; /* Mark no address found yet */

			/*Now process any "TargetAddress" entries following */
			while (*tp && strncmp((char *)tp, "TargetAddress=", 14) == 0) {
				tp += 14;
				sp = tp; /* save start of address */
				while (*tp++) {
				}
				/*Get the target address */
				rc = cl_get_address(&addr, (char *)sp);
				if (rc) {
					add_discovered_target(TargetName,
						&addr, PORTAL_TYPE_SENDTARGET,
						id);
				} else {
					DEBOUT(("Syntax error in returned target address <%s>\n", sp));
					break;
				}
			}

			if (rc == -1) {
				/* There are no TargetAddress entries
				 * associated with TargetName. This means the
				 * sendtarget address is used. */
				sendtarg = find_send_target_id(id);
				if (sendtarg != NULL) {
					add_discovered_target(TargetName,
						&sendtarg->addr,
						PORTAL_TYPE_SENDTARGET, id);
				}
			}
		} /* end of while */
	}
	/*
	 * Go through our list of portals and look for ones
	 * that are still marked for refreshing.
	 * These are ones that are no longer there and should be removed.
	 */

	for (curr = TAILQ_FIRST(&list[PORTAL_LIST].list); curr != NULL;
		 curr = next) {
		portal_t *p = (void *)curr;
		next = TAILQ_NEXT(curr, link);
		if (p->portaltype == PORTAL_TYPE_REFRESHING)
			delete_portal(p, TRUE);
	}

	/*
	 * Clean up
	 */

	if (response_buffer != NULL)
		free(response_buffer);

	return ret;
}


/*
 * cleanup_send_target_orphans:
 *    Delete portals that were discovered through a now deleted send target.
 */


static void
cleanup_orphans(iscsi_portal_types_t type)
{
	generic_entry_t *curr;
	generic_entry_t *next;

	/*
	 * Go through the list of portals and look for ones marked with a zero
	 * discovery ID, those are associated with send targets that no
	 * longer exist.
	 */

	for (curr = TAILQ_FIRST(&list[PORTAL_LIST].list); curr != NULL;
		 curr = next) {
		portal_t *p = (void *)curr;
		next = TAILQ_NEXT(curr, link);
		if (p->portaltype == type && p->discoveryid == 0) {
			delete_portal(p, TRUE);
		}
	}
}


/*
 * refresh_targets:
 *    Handle REFRESH_TARGETS request:
 *    Refreshes the list of targets discovered via SendTargets or iSNS
 *
 *    Parameter:
 *          The request parameter = iscsid_refresh_targets_req_t containing:
 *             iscsid_list_kind_t   kind;       Kind: 
 *                                                  SEND_TARGETS_LIST
 *                                                  ISNS_LIST
 *             uint32_t                num_ids;    # of targets in list 
 *             uint32_t            id [1];     List of targets 
 *
 *    Returns:     status
 */

uint32_t
refresh_targets(iscsid_refresh_req_t * par)
{
	uint32_t t;
	uint32_t rc, retval;
	generic_entry_t *curr;

	retval = ISCSID_STATUS_NO_TARGETS_FOUND;

	switch (par->kind) {
	case TARGET_LIST:
		/*
		 * Refreshing for a specific target makes no sense if it's
		 * static. Maybe implement it for dynamically dicovered
		 * targets? But then it's best done through the discovering
		 * instance, or we'll refresh much more than just the given
		 * target. And refreshing the whole list is iffy as well. So
		 * refuse this op on the target list for now.
		 */
		break;

	case SEND_TARGETS_LIST:
		DEB(9, ("Refresh Send Targets List - num_ids = %d\n",
				par->num_ids));
		if (par->num_ids) {
			/* Target ids are specified */
			for (t = 0; t < par->num_ids; t++) {
				rc = refresh_send_target(par->id[t]);
				if (rc == 0) {
					retval = ISCSID_STATUS_SUCCESS;
				}
			}
		} else {
			cleanup_orphans(PORTAL_TYPE_SENDTARGET);

			/* No target ids specified - refresh all. */
			TAILQ_FOREACH(curr, &list[SEND_TARGETS_LIST].list, link)
				if ((rc = refresh_send_target(curr->sid.id)) == 0)
					retval = ISCSID_STATUS_SUCCESS;
		}
		return retval;

#ifndef ISCSI_MINIMAL
	case ISNS_LIST:
		DEB(9, ("Refresh iSNS List - num_ids = %d\n", par->num_ids));
		if (par->num_ids) {
			/*Target ids are specified */
			for (t = 0; t < par->num_ids; t++)
				if ((rc = refresh_isns_server(par->id[t])) == 0)
					retval = ISCSI_STATUS_SUCCESS;
		} else {
			cleanup_orphans(PORTAL_TYPE_ISNS);

			/*No target ids specified - refresh all. */
			TAILQ_FOREACH(curr, &list[ISNS_LIST].list, link)
				if ((rc = refresh_isns_server(curr->sid.id)) == 0)
					retval = ISCSI_STATUS_SUCCESS;
		}
		return retval;
#endif

	default:
		break;
	}

	return ISCSID_STATUS_PARAMETER_INVALID;
}
