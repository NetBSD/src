/*	$NetBSD: iscsid_discover.c,v 1.3.2.1 2012/07/03 20:48:40 jdc Exp $	*/

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

#ifndef ISCSI_MINIMAL

#include "iscsid_globals.h"
#include "isns.h"
#include "isns_defs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#define MY_FLAGS  ISNS_FLAG_REPLACE_REG


/**********************************************************************
**********************************************************************/

uint32_t isns_id = 0;			/* isns ID counter */

ISNS_HANDLE isns_handle = ISNS_INVALID_HANDLE;

/**********************************************************************
**********************************************************************/

/*
 * xlate_ip
 *  Support routine to translate binary IP into string form for storage in
 *  target object. Handles IPv6 and IPv4 formats.
 *
 * Parameters:
 *       dest  the destination string
 *       data  the source (from iSNS address field)
 *
 * Returns:     status
 */

static void
xlate_ip(uint8_t *dest, size_t size, void *data)
{
	uint16_t *wdt = (uint16_t *) data;
	size_t	cc;
	int i;
	char *dst = (char *)dest;
	char *dt = data;

	for (i = 0; i < 5 && !wdt[i]; i++) {
	}
	if (i == 5 && wdt[5] == 0xffff) {
		snprintf(dst, size, "%d.%d.%d.%d",
			dt[12], dt[13], dt[14], dt[15]);
	} else {
		for (cc = 0, i = 0; i < 7; i++) {
			cc += snprintf(&dst[cc], size - cc, "%x:", wdt[i]);
		}
		snprintf(&dst[cc], size - cc, "%x", wdt[7]);
	}
}


/*
 * get_isns_target_info
 *  Support routine to query the server for the attributes of the given target.
 *
 * Parameters:
 *       TargetName  The target name to query.
 *
 * Returns:     status
 */

static uint32_t
get_isns_target_info(isns_t * isns, uint8_t * TargetName)
{
	int retval;
	ISNS_TRANS t;
	uint32_t tag;
	uint32_t data_len;
	void *data_p;
	uint32_t u32;
	struct timespec tout = { 5, 0 };
	uint32_t status;
	target_t *targ;
	char name[ISCSI_STRING_LENGTH];
	char alias[ISCSI_STRING_LENGTH];
	iscsi_portal_address_t addr;

	t = isns_new_trans(isns_handle, isnsp_DevAttrQry, MY_FLAGS);
	if (ISNS_INVALID_TRANS == t) {
		DEBOUT(("%s: get_targets iscsi_new_trans failed\n", __func__));
		return ISCSID_STATUS_NO_RESOURCES;
	}
	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);
	isns_add_string(t, isnst_iSCSIName, (char *)TargetName);

	isns_add_tlv(t, isnst_Delimiter, 0, NULL);

	isns_add_tlv(t, isnst_iSCSIName, 0, NULL);	/* 32: name */
	isns_add_tlv(t, isnst_iSCSINodeType, 0, NULL);	/* 33: node type */
	isns_add_tlv(t, isnst_iSCSIAlias, 0, NULL);	/* 34: alias */
	/* ToDo: get security attributes... */
	/* isns_add_tlv (t, isnst_PortalSecBmap, 0, NULL); */
	/*tag=27: security bitmap */

	retval = isns_send_trans(t, &tout, &status);
	DEB(9, ("isns_send_trans called, returns %d, status %d\n", retval, status));
	if (retval) {
		DEBOUT(("iSNS Attribute Query failed, rc = %d\n", retval));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	/* First is target name (the one we put in), ignore */
	if (isns_get_tlv(t, ISNS_TLV_FIRST, &tag, &data_len, &data_p)) {
		DEBOUT(("iSNS Attribute Query returned nothing\n"));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	if (tag != isnst_iSCSIName) {
		DEBOUT(("iSNS Query returned bad type (tag = %d, length = %d)\n",
				tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}

	isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	if (tag != isnst_Delimiter) {
		DEBOUT(("Attr Query Missing Delimiter (tag = %d, length = %d)\n",
				tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}

	isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	if (tag != isnst_iSCSIName || !data_len || data_len >= ISCSI_STRING_LENGTH) {
		DEBOUT(("iSNS Query returned no or invalid name (tag = %d, "
				"length = %d)\n", tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	strlcpy(name, (char *) data_p, sizeof(name));

	isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	if (tag != isnst_iSCSINodeType || data_len != 4) {
		DEBOUT(("iSNS Query returned no or invalid node type (tag = %d, "
				"length = %d)\n", tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	u32 = ntohl(*((uint32_t *) data_p));
	if (!(u32 & 1)) {
		DEBOUT(("iSNS Query returned bad type (type=%x, should be 1)\n", u32));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}

	isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	if (tag == isnst_iSCSIAlias) {
		if (data_len >= ISCSI_STRING_LENGTH) {
			DEBOUT(("iSNS Query returned invalid alias (tag=%d, length=%d)\n",
					tag, data_len));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		strlcpy(alias, (char *) data_p, sizeof(alias));
		isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	} else
		alias[0] = 0;

	isns_free_trans(t);

	if (ISNS_INVALID_TRANS ==
		(t = isns_new_trans(isns_handle, isnsp_DevAttrQry, MY_FLAGS))) {
		DEBOUT(("get_targets iscsi_new_trans failed\n"));
		return ISCSID_STATUS_NO_RESOURCES;
	}
	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);
	isns_add_string(t, isnst_iSCSIName, (char *)TargetName);

	isns_add_tlv(t, isnst_Delimiter, 0, NULL);

	isns_add_tlv(t, isnst_PGiSCSIName, 0, NULL);	/* 48: portal name */
	isns_add_tlv(t, isnst_PGPortIPAddr, 0, NULL);	/* 49: portal IP */
	isns_add_tlv(t, isnst_PGPortIPPort, 0, NULL);	/* 50: portal port */
	isns_add_tlv(t, isnst_PGTag, 0, NULL);	/* 51: group tag */

	retval = isns_send_trans(t, &tout, &status);
	DEB(9, ("isns_send_trans called, returns %d, status %d\n", retval, status));
	if (retval) {
		DEBOUT(("iSNS Attribute Query failed, rc = %d\n", retval));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	/* First is target name (the one we put in), ignore */
	if (isns_get_tlv(t, ISNS_TLV_FIRST, &tag, &data_len, &data_p)) {
		DEBOUT(("iSNS Attribute Query returned nothing\n"));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}
	if (tag != isnst_iSCSIName) {
		DEBOUT(("iSNS Query2 returned bad name (tag = %d, length = %d)\n",
				tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}

	isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
	if (tag != isnst_Delimiter) {
		DEBOUT(("Attr Query2 Missing Delimiter (tag = %d, length = %d)\n",
				tag, data_len));
		isns_free_trans(t);
		return ISCSID_STATUS_ISNS_SERVER_ERROR;
	}

	while (!isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p)) {
		if (tag != isnst_PGiSCSIName || !data_len ||
			data_len >= ISCSI_STRING_LENGTH) {
			DEBOUT(("iSNS Query2 returned no or invalid name (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		strlcpy(name, (char *) data_p, sizeof(name));

		isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
		if (tag != isnst_PGPortIPAddr || data_len != 16) {
			DEBOUT(("iSNS Query returned no or invalid address (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		xlate_ip(addr.address, sizeof(addr.address), (uint8_t *) data_p);

		/* Now comes the port */
		isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
		if (tag != isnst_PGPortIPPort || data_len != 4) {
			DEBOUT(("iSNS Query returned no or invalid port (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		u32 = ntohl(*((uint32_t *) data_p));
		if (u32 & 0xffff0000) {
			DEBOUT(("iSNS Query returned invalid port (flags=%x, "
					"should be 0)\n", u32 >> 16));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		addr.port = (uint16_t) u32;

		/* And each target must have a group tag */
		isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p);
		if (tag != isnst_PGTag || (data_len && data_len != 4)) {
			DEBOUT(("iSNS Query returned no or invalid group tag (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			return ISCSID_STATUS_ISNS_SERVER_ERROR;
		}
		if (data_len) {
			u32 = ntohl(*((uint32_t *) data_p));
			addr.group_tag = (uint16_t) u32;
		} else
			addr.group_tag = 0;

		/* we have everything necessary to describe the target, add it. */

		DEB(1, ("Adding <%s>, IP <%s>, Port %d, Tag %d\n",
				name, addr.address, addr.port, addr.group_tag));

		if ((targ = add_discovered_target((unsigned char *)name, &addr, PORTAL_TYPE_ISNS,
				isns->entry.sid.id)) == NULL) {
			isns_free_trans(t);
			return ISCSID_STATUS_NO_RESOURCES;
		}

		if (alias[0]) {
			strlcpy((char *)targ->TargetAlias, alias,
				sizeof(targ->TargetAlias));
		}
	}
	isns_free_trans(t);

	return ISCSID_STATUS_SUCCESS;
}


/*
 * deregister_isns_server
 *  Support routine to deregister the initiator from the iSNS server
 *
 * Parameters:  The server descriptor
 *
 * Returns:     status
 */

static uint32_t
deregister_isns_server(isns_t * isns)
{
	int retval;
	ISNS_TRANS t;
	struct timespec tout = { 5, 0 };
	uint32_t status;

	/*
	 * Create transaction for deregistering with iSNS server
	 */

	if (ISNS_INVALID_TRANS == (t = isns_new_trans(isns_handle, isnsp_DevDereg,
												  MY_FLAGS))) {
		DEBOUT(("dereg_isns_server iscsi_new_trans failed\n"));
		return ISCSID_STATUS_NO_RESOURCES;
	}

	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);
	isns_add_tlv(t, isnst_Delimiter, 0, NULL);
	isns_add_string(t, isnst_EID, (char *)isns->reg_entity_id);
	isns_add_tlv(t, isnst_PortalIPAddr, (uint32_t)sizeof(isns->reg_ip_addr),
				 isns->reg_ip_addr);
	isns_add_tlv(t, isnst_PortalPort, (uint32_t)sizeof(isns->reg_ip_port),
				 &isns->reg_ip_port);
	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);

	retval = isns_send_trans(t, &tout, &status);
	DEB(9, ("DevAttrReg request returns %d, status %d\n", retval, status));

	isns_free_trans(t);
	return ISCSID_STATUS_SUCCESS;
}

/*
 * register_isns_server
 *
 * Parameters:  The server descriptor
 *
 * Returns:     status
 */


static uint32_t
register_isns_server(isns_t * isns)
{
	int retval;
	ISNS_TRANS t;
	uint32_t u32;
	struct timespec tout = { 5, 0 };
	uint32_t status;

	if (ISNS_INVALID_TRANS == (t = isns_new_trans(isns_handle, isnsp_DevAttrReg,
												  MY_FLAGS))) {
		DEBOUT(("iscsi_new_trans failed\n"));
		return ISCSID_STATUS_NO_RESOURCES;
	}

	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);	/*tag=32 */
	isns_add_tlv(t, isnst_Delimiter, 0, NULL);
	isns_add_string(t, isnst_EID, (char *)isns->reg_entity_id);
	u32 = htonl(2);
	isns_add_tlv(t, isnst_EntProtocol, (uint32_t)sizeof(u32), &u32);
	isns_add_tlv(t, isnst_PortalIPAddr, (uint32_t)sizeof(isns->reg_ip_addr),
				 isns->reg_ip_addr);
	isns_add_tlv(t, isnst_PortalPort, (uint32_t)sizeof(isns->reg_ip_port),
				 &isns->reg_ip_port);
	isns_add_string(t, isnst_iSCSIName, (char *)isns->reg_iscsi_name);	/*tag=32 */
	u32 = htonl(2);
	isns_add_tlv(t, isnst_iSCSINodeType, (uint32_t)sizeof(u32), &u32);
		/*tag=33 (node type = intiator) */

	retval = isns_send_trans(t, &tout, &status);
	DEB(9, ("DevAttrReg request returns %d, status %d\n", retval, status));
	isns_free_trans(t);

	if (retval || status)
		return ISCSID_STATUS_ISNS_ERROR;

	return ISCSID_STATUS_SUCCESS;
}


/*
 * get_registration_info
 *
 * Parameters:  The server descriptor
 *
 * Returns:     status
 */


static uint32_t
get_registration_info(isns_t * isns)
{
	struct sockaddr_storage sa;
	unsigned n;

	strlcpy((char *)isns->reg_iscsi_name, (char *)node_name.InitiatorName,
			sizeof(isns->reg_iscsi_name));
	strlcpy((char *)isns->reg_entity_id, (char *)node_name.InitiatorAlias,
			sizeof(isns->reg_entity_id));

	/*Get our source IP and port numbers */
	n = sizeof(sa);
	if (getsockname(isns->sock, (struct sockaddr *)(void *)&sa, &n)) {
		DEBOUT(("Getsockname returned error %d\n", errno));
		return ISCSID_STATUS_GENERAL_ERROR;
	}
	switch (sa.ss_family) {
	case AF_INET:
		{
			struct sockaddr_in *si =
			    (struct sockaddr_in *)(void *)&sa;
			uint32_t *u32 = (uint32_t *)(void *)isns->reg_ip_addr;

			u32[0] = u32[1] = 0;
			u32[2] = htonl(0xffff);
			u32[3] = htonl(si->sin_addr.s_addr);
			isns->reg_ip_port = htons(si->sin_port);
		}
		break;

	case AF_INET6:
		{
			struct sockaddr_in6 *si =
			    (struct sockaddr_in6 *)(void *) &sa;

			memcpy(isns->reg_ip_addr, &si->sin6_addr,
					sizeof(isns->reg_ip_addr));
			isns->reg_ip_port = htons(si->sin6_port);
		}
		break;

	default:
		DEBOUT(("Getsockname returned unknown address family: %d\n",
				sa.ss_family));
		return ISCSID_STATUS_GENERAL_ERROR;
	}
	return ISCSID_STATUS_SUCCESS;
}


/*
 * iscsi_isns_serverconn - given a set of server address, try connecting
 *
 * Parameters:  The descriptor for the iSNS server to query
 *
 * Returns:     status
 */

static uint32_t
iscsi_isns_serverconn(isns_t * isns)
{
	int sock = -1;
	char port[16];
	struct addrinfo hints;
	struct addrinfo *ai, *addr;
	int retval;

	/*
	 * Initialize the iSNS library if it needs it
	 */

	if (isns_handle == ISNS_INVALID_HANDLE) {
		if ((retval = isns_init(&isns_handle, 0)) != 0) {
			/*Couldn't initialize the iSNS library */
			DEBOUT(("isns_init failed with code %d\n", retval));
			isns_handle = ISNS_INVALID_HANDLE;
			return ISCSID_STATUS_GENERAL_ERROR;
		}
	}

	/*
	 * Find the server address from the iSNS server list entry,
	 * and try to connect to the iSNS server
	 */

	snprintf(port, sizeof(port), "%d", (isns->port) ? isns->port : ISCSI_DEFAULT_ISNS_PORT);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	retval = getaddrinfo((char *)isns->address, port, &hints, &ai);
	if (retval) {
		DEBOUT(("getaddrinfo failed with code %d (%s)\n",
				retval, gai_strerror(retval)));
		return ISCSID_STATUS_GENERAL_ERROR;
	}

	for (addr = ai; addr != NULL; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype,
		    addr->ai_protocol);

		if (sock == -1) {
			DEBOUT(("%s: socket call FAILED!\n", __func__));
			freeaddrinfo(ai);
			return (uint32_t)-1;
		}

		if (connect(sock, addr->ai_addr, addr->ai_addrlen) != -1)
			break;

		DEB(1, ("%s: connect call FAILED!\n", __func__));
		close(sock);
		sock = -1;
	}

	if (addr == NULL) {
		DEBOUT(("%s: couldn't connect!\n", __func__));
		freeaddrinfo(ai);
		return ISCSID_STATUS_GENERAL_ERROR;
	}

	if (isns_add_servercon(isns_handle, sock, addr)) {
		DEBOUT(("%s: FAILED!\n", __func__));
		close(sock);
		freeaddrinfo(ai);
		return ISCSID_STATUS_GENERAL_ERROR;
	}

	freeaddrinfo(ai);
	isns->sock = sock;

	if ((retval = get_registration_info(isns)) != 0) {
		return retval;
	}

	deregister_isns_server(isns);

	return register_isns_server(isns);
}


/*
 * update_isns_server_info
 *  Support routine to query the specified iSNS server for all targets
 *  Called from add_isns_server and refresh_isns_server
 *
 * Parameters:  The descriptor for the iSNS server to query
 *
 * Returns:     status
 */


static uint32_t
update_isns_server_info(isns_t * isns)
{
	int retval;
	ISNS_TRANS t;
	uint32_t tag;
	uint32_t data_len;
	void *data_p;
	uint32_t u32;
	struct timespec tout = { 5, 0 };
	uint32_t status;
	uint8_t TargetName[ISCSI_STRING_LENGTH];


	DEB(9, ("update_isns_server_info for iSNS %s\n", isns->address));

	if (isns->sock < 0) {
		if ((status = iscsi_isns_serverconn(isns)) != 0) {
			/*We couldn't connect to the iSNS server */
			DEB(9, ("update_isns_server_info iscsi_isns_serverconn failed\n"));
			return status;
		}
	}

	for (TargetName[0] = 0;;) {
		if (ISNS_INVALID_TRANS == (t = isns_new_trans(isns_handle,
												isnsp_DevGetNext, MY_FLAGS))) {
			DEBOUT(("update_isns_server_info iscsi_new_trans failed\n"));
			return ISCSID_STATUS_NO_RESOURCES;
		}

		isns_add_string(t, isnst_iSCSIName, (char *)node_name.InitiatorName);

		if (TargetName[0])
			isns_add_string(t, isnst_iSCSIName, (char *)TargetName);
		else
			isns_add_tlv(t, isnst_iSCSIName, 0, NULL);

		isns_add_tlv(t, isnst_Delimiter, 0, NULL);
		isns_add_tlv(t, isnst_iSCSINodeType, 0, NULL);

		if ((retval = isns_send_trans(t, &tout, &status)) != 0) {
			DEBOUT(("isns_send_trans returns rc %d, status %d\n",
					retval, status));
			isns_free_trans(t);
			break;
		}
		if (status) {
			DEB(9, ("DevGetNext Status = %d\n", status));
			break;
		}

		if (isns_get_tlv(t, ISNS_TLV_FIRST, &tag, &data_len, &data_p)) {
			DEBOUT(("No TLV in DevGetNext response!\n"));
			isns_free_trans(t);
			break;
		}
		/* We need the name, or there's nothing left to do */

		if (tag != isnst_iSCSIName || !data_len ||
			data_len >= ISCSI_STRING_LENGTH) {
			DEBOUT(("iSNS GetNextDev returned no or invalid name (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			break;
		}
		strlcpy((char *)TargetName, (char *) data_p, sizeof(TargetName));

		/* We must get at least the node type, and it must be a target */
		if (isns_get_tlv(t, ISNS_TLV_NEXT, &tag, &data_len, &data_p)) {
			DEBOUT(("iSNS GetDevNext did not return node type\n"));
			isns_free_trans(t);
			break;
		}
		if (tag == isnst_Delimiter && isns_get_tlv(t, ISNS_TLV_NEXT, &tag,
													&data_len, &data_p)) {
			DEBOUT(("iSNS GetDevNext did not return node type (past delim)\n"));
			isns_free_trans(t);
			break;
		}
		if (tag != isnst_iSCSINodeType || data_len != 4) {
			DEBOUT(("iSNS Query returned no or invalid node type (tag=%d, "
					"length=%d)\n", tag, data_len));
			isns_free_trans(t);
			break;
		}
		u32 = ntohl(*((uint32_t *) data_p));
		isns_free_trans(t);

		if (u32 & 1)
			get_isns_target_info(isns, TargetName);
	}

	DEB(9, ("update_isns_server_info returning SUCCESS!\n"));
	return ISCSID_STATUS_SUCCESS;
}


/**********************************************************************
**********************************************************************/

/*
 * create_isns:
 *    Create an isns structure and initialize it.
 *
 *    Parameter:
 *          name        The iSNS server name
 *
 *    Returns:    Pointer to isns structure, NULL if allocation failed.
 */

static isns_t *
create_isns(iscsid_add_isns_server_req_t * req)
{
	isns_t *isns;

	DEB(9, ("Create iSNS %s\n", req->address));

	if ((isns = calloc(1, sizeof(*isns))) == NULL)
		return NULL;

	for (isns_id++; !isns_id || find_isns_id(isns_id) != NULL;)
		isns_id++;

	isns->entry.sid.id = isns_id;
	strlcpy((char *)isns->entry.sid.name, (char *)req->name, sizeof(isns->entry.sid.name));
	strlcpy((char *)isns->address, (char *)req->address, sizeof(isns->address));
	isns->port = req->port;
	isns->sock = -1;

	return isns;
}


/*
 * add_isns_server
 *    Adds an iSNS server to the server list.
 *    This command will add the address of an iSNS server to the list
 *      of iSNS servers that the discovery daemon queries to discover targets.
 *      The daemon will then register itself with the iSNS server,
 *      and query the iSNS server for the list of targets.
 *      The discovered targets will be added to the list of target portals.
 *      The response contains the ID of the iSNS server.
 *
 * Parameter:  The parameter contains the address of the server.
 *
 * Returns:    Nothing
 *             The response parameter is an iscsid_add_isns_server_rsp_t
 *             containing:
 *                  server_id = Unique ID for the iSNS server
 */

void
add_isns_server(iscsid_add_isns_server_req_t * req, iscsid_response_t ** prsp,
				int *prsp_temp)
{
	iscsid_response_t *rsp = *prsp;
	iscsid_add_isns_server_rsp_t *res;
	isns_t *isns;

	DEB(9, ("IN add_isns_server\n"));

	/*
	 * Make a response
	 */

	rsp = make_rsp(sizeof(iscsid_add_isns_server_rsp_t), prsp, prsp_temp);
	if (rsp == NULL) {
		DEB(9, ("OUT add_isns_server: make_rsp FAILED\n"));
		return;
	}

	res = (iscsid_add_isns_server_rsp_t *)(void *)rsp->parameter;

	/*
	 * First, allocate the isns server structure to put on the list
	 */

	isns = create_isns(req);
	if (isns == NULL) {
		rsp->status = ISCSID_STATUS_NO_RESOURCES;
		DEB(9, ("OUT add_isns_server: create_isns FAILED!\n"));
		return;
	}

	TAILQ_INSERT_TAIL(&list[ISNS_LIST].list, &isns->entry, link);
	list[ISNS_LIST].num_entries++;
	res->server_id = isns->entry.sid.id;

	DEB(9, ("OUT add_isns_server: server_id = %d, name = %s\n",
			isns->entry.sid.id, isns->address));

	/*
	 * Now try to connect to the iSNS server...
	 */

	update_isns_server_info(isns);
}


/*
 * get_isns_server
 *    Returns the address of the iSNS server with the specified ID
 *
 * Parameters:  The unique ID of the server
 *
 * Returns:     The status returned by the driver
 *              The response parameter contains the iSNS server address as a
 *              zero-terminated UTF-8 string
 */

void
get_isns_server(iscsid_sym_id_t * preq, iscsid_response_t ** prsp,
				int *prsp_temp)
{
	iscsid_response_t *rsp = *prsp;
	iscsid_get_isns_server_rsp_t *res;
	isns_t *isns;

	DEB(9, ("IN get_isns_server\n"));
	isns = find_isns(preq);
	if (isns == NULL) {
		rsp->status = ISCSID_STATUS_INVALID_ISNS_ID;
		DEB(9, ("OUT get_isns_server: find_isns FAILED!\n"));
		return;
	}

	rsp = make_rsp(sizeof(iscsid_get_isns_server_rsp_t), prsp, prsp_temp);
	if (rsp == NULL) {
		DEB(9, ("OUT get_isns_server: make_rsp FAILED!\n"));
		return;
	}
	res = (iscsid_get_isns_server_rsp_t *)(void *)rsp->parameter;

	strlcpy((char *)res->address, (char *)isns->address,
	    sizeof(res->address));
	res->port = isns->port;
	res->server_id = isns->entry.sid;
	DEB(9, ("OUT get_isns_server: id = %d, address = %s\n",
			res->server_id.id, res->address));
}


/*
 * slp_find_isns_servers
 */

/* More Here Later... */


/*
 * refresh_isns_server
 *    Query the specified iSNS servers for the list of targets.
 *
 *    Parameters:
 *          id    Server ID
 *
 *    Returns:     Status
 */

uint32_t
refresh_isns_server(uint32_t id)
{
	uint32_t rc;
	isns_t *isns;
	generic_entry_t *curr;
	generic_entry_t *next;

	isns = find_isns_id(id);
	if (isns == NULL)
		return ISCSID_STATUS_INVALID_ISNS_ID;

	TAILQ_FOREACH(curr, &list[PORTAL_LIST].list, link) {
		portal_t *p = (portal_t *)(void *)curr;
		if (p->portaltype == PORTAL_TYPE_ISNS && p->discoveryid == id)
			p->portaltype = PORTAL_TYPE_REFRESHING;
	}

	rc = update_isns_server_info(isns);

	/*
	 * Go through our list of portals and look for ones
	 * that are still marked for refreshing.
	 * These are ones that are no longer there and should be removed.
	 */

	for (curr = TAILQ_FIRST(&list[PORTAL_LIST].list); curr != NULL;
		 curr = next) {
		portal_t *p = (portal_t *)(void *)curr;
		next = TAILQ_NEXT(curr, link);
		if (p->portaltype == PORTAL_TYPE_REFRESHING)
			delete_portal(p, TRUE);
	}

	return rc;
}


/*
 * remove_isns_server
 *    Removed an iSNS server.
 *    This does not remove the discovered targets from the list.
 *
 * Parameters:  The iscid_remove_isns_req_t structure containing:
 *                  server_id = unique ID of server to remove
 *
 * Returns:     The status returned.
 */

uint32_t
remove_isns_server(iscsid_sym_id_t * preq)
{
	generic_entry_t *curr;
	isns_t *isns;
	uint32_t id;

	isns = find_isns(preq);
	if (isns == NULL)
		return ISCSID_STATUS_INVALID_ISNS_ID;

	/*Deregister with the iSNS server. */
	/*Ignore any errors during deregistration... */
	if (isns->sock >= 0) {
		deregister_isns_server(isns);
		close(isns->sock);
	}

	TAILQ_REMOVE(&list[ISNS_LIST].list, &isns->entry, link);
	list[ISNS_LIST].num_entries--;

	id = isns->entry.sid.id;
	free(isns);

	TAILQ_FOREACH(curr, &list[PORTAL_LIST].list, link) {
		portal_t *p = (portal_t *)(void *)curr;
		if (p->portaltype == PORTAL_TYPE_ISNS && p->discoveryid == id)
			p->discoveryid = 0; /* mark deleted */
	}

	return ISCSID_STATUS_SUCCESS;
}


/*
   Deregister all isns servers on daemon termination
*/

void
dereg_all_isns_servers(void)
{
	generic_list_t *plist;
	generic_entry_t *curr;

	plist = &list[ISNS_LIST].list;
	TAILQ_FOREACH(curr, plist, link)
		deregister_isns_server((isns_t *)(void *)curr);
}

#endif

