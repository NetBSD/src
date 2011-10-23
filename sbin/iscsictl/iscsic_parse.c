/*	$NetBSD: iscsic_parse.c,v 1.1 2011/10/23 21:11:23 agc Exp $	*/

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

#include <ctype.h>

/*
 * get_address:
 *    Get an address specification that may include port and group tag.
 *
 *    Parameter:
 *       portal   The portal address
 *       str      The parameter string to scan
 *
 *    Aborts app on error.
 */

STATIC void
get_address(iscsi_portal_address_t * portal, char *str, char *arg)
{
	char *sp, *sp2;
	int val;

	if (!str || !*str)
		arg_error(arg, "Address is missing");

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
		/* if there's a second colon, assume it's an unbracketed IPv6 address */
		if (!*sp2) {
			/* truncate source, that's the address */
			*sp++ = '\0';
			if (sscanf(sp, "%d", &val) != 1)
				arg_error(arg, "Bad address format: Expected port number");
			if (val < 0 || val > 0xffff)
				arg_error(arg, "Bad address format: Port number out of range");
			portal->port = (uint16_t) val;
		}
		/* is there a group tag? */
		for (; isdigit((unsigned char)*sp); sp++);
		if (*sp && *sp != ',')
			arg_error(arg, "Bad address format: Extra character(s) '%c'", *sp);
	} else
		for (sp = str + 1; *sp && *sp != ','; sp++);

	if (*sp) {
		if (sscanf(sp + 1, "%d", &val) != 1)
			arg_error(arg, "Bad address format: Expected group tag");
		if (val < 0 || val > 0xffff)
			arg_error(arg, "Bad address format: Group tag out of range");
		portal->group_tag = (uint16_t) val;
		/* truncate source, that's the address */
		*sp = '\0';
	}
	/* only check length, don't verify correct format (too many possibilities) */
	if (strlen(str) >= sizeof(portal->address))
		arg_error(arg, "Bad address format: Address string too long");

	strlcpy((char *)portal->address, str, sizeof(portal->address));
}



/*
 * get_short_int:
 *    Get a short integer.
 *
 *    Parameter:
 *       sp       The parameter string to scan
 *       arg      The associated option argument (for error message)
 *       name     The argument name
 *
 *    Returns given integer, aborts app on error.
 */

STATIC uint16_t
get_short_int(char *sp, char *arg, const char *name)
{
	int val;

	if (!sp || !*sp)
		arg_error(arg, "%s is missing", name);

	if (!sscanf(sp, "%d", &val))
		arg_error(arg, "Expected integer %s", name);
	if (val < 0 || val > 0xffff)
		arg_error(arg, "%s out of range", name);

	return (uint16_t) val;
}


/*
 * get_dsl:
 *    Get MaxRecvDataSegmentLength
 *
 *    Parameter:
 *       sp       The parameter string to scan
 *       arg      The associated option argument (for error message)
 *
 *    Returns given integer, aborts app on error.
 */

STATIC uint32_t
get_dsl(char *sp, char *arg)
{
	int val;

	if (!sp || !*sp)
		arg_error(arg, "Missing MaxRecvDataSegmentLength");
	if (!sscanf(sp, "%d", &val))
		arg_error(arg, "Integer MaxRecvDataSegmentLength expected");
	if (val < 512 || val > 0xffffff)
		arg_error(arg, "MaxRecvDataSegmentLength out of range");

	return (uint32_t) val;
}


/*
 * get_str:
 *    Get a string.
 *
 *    Parameter:
 *       dest     The destination string
 *       sp       The parameter string to scan
 *       arg      The associated option argument (for error message)
 *       name     The argument name
 *
 *    Aborts app on error.
 */

STATIC void
get_str(char *dest, char *sp, char *arg, const char *name)
{

	if (!sp || !*sp)
		arg_error(arg, "%s is missing", name);
	if (strlen(sp) >= ISCSI_STRING_LENGTH)
		arg_error(arg, "%s is too long", name);

	strlcpy(dest, sp, ISCSI_STRING_LENGTH);
}

/*
 * cl_get_target:
 *    Get a target address specification that may include name, address, port,
 *    and group tag, with address/port/tag possibly repeated.
 *
 *    Parameter:
 *       ptarg       pointer to hold the resulting add target request parameter
 *       argc, argv  program parameters (shifted)
 *       nreq        target name is required if TRUE
 *
 *    Returns:    0 if there is no target, else the size of the allocated
 *                  request.
 *                Aborts app on bad parameter or mem allocation error.
 */

int
cl_get_target(iscsid_add_target_req_t ** ptarg, int argc, char **argv, int nreq)
{
	iscsid_add_target_req_t *targ;
	char *sp;
	int num, i, len, name;

	/* count number of addreses first, so we know how much memory to allocate */
	for (i = num = name = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;
		if (argv[i][1] == 'a')
			num++;
		if (argv[i][1] == 'n')
			name++;
	}

	if (!name && nreq)
		return 0;

	len = sizeof(iscsid_add_target_req_t) +
		num * sizeof(iscsi_portal_address_t);

	if (NULL == (targ = calloc(1, len)))
		gen_error("Can't allocate %d bytes of memory", len);

	*ptarg = targ;
	num = -1;

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		switch (argv[i][1]) {
		case 'n':				/* target name */
			get_str((char *)targ->TargetName, sp, argv[i], "Target name");
			break;

		case 'a':				/* target address */
			get_address(&targ->portal[++num], sp, argv[i]);
			break;

		case 'p':				/* port */
			targ->portal[num].port = get_short_int(sp, argv[i], "Port");
			break;

		case 'g':				/* group tag */
			targ->portal[num].group_tag = get_short_int(sp, argv[i],
														"Group tag");
			break;

		default:
			continue;
		}
		if (!argv[i][2])
			argv[i + 1] = NULL;

		argv[i] = NULL;
	}
	targ->num_portals = num + 1;

	DEB(9, ("cl_get_target returns %d\n", len));
	return len;
}


/*
 * cl_get_isns:
 *    Get an iSNS server address specification that may include name, address
 *    and port.
 *
 *    Parameter:
 *       srv         add_isns_server request parameter
 *       argc, argv  program parameters (shifted)
 *
 *    Returns:    0 on error, 1 if OK.
 */

int
cl_get_isns(iscsid_add_isns_server_req_t * srv, int argc, char **argv)
{
	iscsi_portal_address_t addr;
	char *sp;
	int i, found;

	(void) memset(&addr, 0x0, sizeof(addr));
	found = FALSE;

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		switch (argv[i][1]) {
		case 'N':				/* symbolic name */
			get_str((char *)srv->name, sp, argv[i], "Server name");
			break;

		case 'a':				/* target address */
			get_address(&addr, sp, argv[i]);
			found = TRUE;
			break;

		case 'p':				/* port */
			addr.port = get_short_int(sp, argv[i], "Port");
			break;

		default:
			continue;
		}
		if (!argv[i][2]) {
			argv[i + 1] = NULL;
		}
		argv[i] = NULL;
	}

	strlcpy((char *)srv->address, (char *)addr.address, sizeof(srv->address));
	srv->port = addr.port;

	return found;
}


/*
 * cl_get_auth_opts:
 *    Get authentication options.
 *
 *    Parameter:
 *          auth        authentication parameters
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if there are no authorization options, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_auth_opts(iscsid_set_target_authentication_req_t *auth,
				 int argc, char **argv)
{
	int n, i, found;
	char *sp;

	found = FALSE;
	memset(auth, 0, sizeof(*auth));

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-') {
			continue;
		}
		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		switch (argv[i][1]) {
		case 't':				/* authentication type */
			if (!sp || !*sp)
				arg_error(argv[i], "Missing authentication type");
			n = 0;
			while (*sp) {
				switch (*sp) {
				case 'n':		/* no authentication */
					auth->auth_info.auth_type[n] = ISCSI_AUTH_None;
					break;
				case 'c':		/* CHAP authentication */
					auth->auth_info.auth_type[n] = ISCSI_AUTH_CHAP;
					break;
				case 'C':		/* Mutual CHAP authentication */
					auth->auth_info.auth_type[n] = ISCSI_AUTH_CHAP;
					auth->auth_info.mutual_auth = 1;
					break;
				default:
					arg_error(argv[i], "Bad authentication type '%c'", *sp);
				}
				sp++;
				n++;
			}
			auth->auth_info.auth_number = n;
			break;

		case 'u':				/* user name */
			get_str((char *)auth->user_name, sp, argv[i], "User name");
			break;

		case 's':				/* secret */
			get_str((char *)auth->password, sp, argv[i], "Secret");
			break;

		case 'S':				/* target secret */
			get_str((char *)auth->target_password, sp, argv[i], "Target secret");
			break;

		default:
			continue;
		}
		if (!argv[i][2])
			argv[i + 1] = NULL;

		argv[i] = NULL;
		found = TRUE;
	}
	DEB(9, ("cl_get_auth_opts returns %d\n", found));
	return found;
}


/*
 * cl_get_target_opts:
 *    Get session/connection options.
 *
 *    Parameter:
 *          opt         target options
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if there are no target options, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_target_opts(iscsid_get_set_target_options_t * opt, int argc, char **argv)
{
	int i, found;
	char *sp;

	found = FALSE;
	memset(opt, 0, sizeof(*opt));

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		switch (argv[i][1]) {
		case 'h':				/* Header Digest */
			opt->HeaderDigest = ISCSI_DIGEST_CRC32C;
			opt->is_present.HeaderDigest = 1;
			break;

		case 'd':				/* Data Digest */
			opt->DataDigest = ISCSI_DIGEST_CRC32C;
			opt->is_present.DataDigest = 1;
			break;

		case 'w':				/* Time 2 Wait */
			opt->DefaultTime2Wait = get_short_int(sp, argv[i], "Time to wait");
			opt->is_present.DefaultTime2Wait = 1;
			if (!argv[i][2])
				argv[i + 1] = NULL;
			break;

		case 'r':				/* Time 2 Retain */
			opt->DefaultTime2Retain = get_short_int(sp, argv[i],
													"Time to retain");
			opt->is_present.DefaultTime2Retain = 1;
			if (!argv[i][2])
				argv[i + 1] = NULL;
			break;

		case 'e':				/* Error Recovery Level */
			opt->ErrorRecoveryLevel = get_short_int(sp, argv[i],
													"ErrorRecoveryLevel");
			opt->is_present.ErrorRecoveryLevel = 1;
			if (!argv[i][2])
				argv[i + 1] = NULL;
			break;

		case 'l':				/* Data Segment Length */
			opt->MaxRecvDataSegmentLength = get_dsl(sp, argv[i]);
			opt->is_present.MaxRecvDataSegmentLength = 1;
			if (!argv[i][2])
				argv[i + 1] = NULL;
			break;

		default:
			continue;
		}
		argv[i] = NULL;
		found = TRUE;
	}
	DEB(9, ("cl_get_target_opts returns %d\n", found));
	return found;
}


/*
 * cl_get_portal:
 *    Get a portal address specification that may include address, port,
 *    and group tag, plus portal options.
 *
 *    Parameter:
 *          port        add portal request parameter
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    FALSE if there is no portal, else TRUE.
 *                Aborts app on bad parameter or mem allocation error.
 */

int
cl_get_portal(iscsid_add_portal_req_t * port, int argc, char **argv)
{
	char *sp;
	int i, found;
	iscsid_portal_options_t *opt = &port->options;

	found = FALSE;
	memset(port, 0, sizeof(*port));

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		switch (argv[i][1]) {
		case 'a':				/* target address */
			get_address(&port->portal, sp, argv[i]);
			found = TRUE;
			break;

		case 'p':				/* port */
			port->portal.port = get_short_int(sp, argv[i], "Port");
			break;

		case 'g':				/* group tag */
			port->portal.group_tag = get_short_int(sp, argv[i], "Group tag");
			break;

		case 'h':				/* Header Digest */
			opt->HeaderDigest = ISCSI_DIGEST_CRC32C;
			opt->is_present.HeaderDigest = 1;
			break;

		case 'd':				/* Data Digest */
			opt->DataDigest = ISCSI_DIGEST_CRC32C;
			opt->is_present.DataDigest = 1;
			break;

		case 'l':				/* Data Segment Length */
			opt->MaxRecvDataSegmentLength = get_dsl(sp, argv[i]);
			opt->is_present.MaxRecvDataSegmentLength = 1;
			if (!argv[i][2])
				argv[i + 1] = NULL;
			break;

		default:
			continue;
		}
		if (!argv[i][2])
			argv[i + 1] = NULL;

		argv[i] = NULL;
	}
	DEB(9, ("cl_get_portal returns %d\n", found));
	return found;
}


/*
 * cl_get_id:
 *    Get an identifier (symbolic or numeric)
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          sid         the ID
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if there is no ID, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_id(char ident, iscsid_sym_id_t * sid, int argc, char **argv)
{
	int i, found;
	char *sp;

	found = FALSE;
	memset(sid, 0, sizeof(*sid));

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2] :
				((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Missing ID");
			if (strlen(sp) >= ISCSI_STRING_LENGTH)
				arg_error(argv[i], "ID String too long");
			if (!sscanf(sp, "%d", &sid->id))
				strlcpy((char *)sid->name, sp, sizeof(sid->name));
			else if (!sid->id)
				arg_error(argv[i], "Invalid ID");

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_id returns %d\n", found));
	return found;
}


/*
 * cl_get_symname:
 *    Get a symbolic name
 *
 *    Parameter:
 *          sn          the name
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if there is no symbolic name, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_symname(uint8_t * sn, int argc, char **argv)
{
	int i, found;
	char *sp;

	found = FALSE;
	*sn = '\0';

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == 'N') {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Symbolic name missing");
			if (isdigit((unsigned char)*sp))
				arg_error(argv[i], "Symbolic name must not be numeric");
			if (strlen(sp) >= ISCSI_STRING_LENGTH)
				arg_error(argv[i], "Symbolic name too long");

			strlcpy((char *)sn, sp, ISCSI_STRING_LENGTH);

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_symname returns %d\n", found));
	return found;
}


/*
 * cl_get_string:
 *    Get a string value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          pstr        the result string
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if there is no string, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_string(char ident, char *pstr, int argc, char **argv)
{
	int i, found;
	char *sp;

	found = FALSE;
	*pstr = '\0';

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			get_str(pstr, sp, argv[i], "String");

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_string returns %d\n", found));
	return found;
}


/*
 * cl_get_opt:
 *    Get an option with no value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    0 if the option was not found, 1 otherwise.
 *                Aborts app on bad parameter.
 */

int
cl_get_opt(char ident, int argc, char **argv)
{
	int i, found;

	found = FALSE;

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_opt returns %d\n", found));
	return found;
}


/*
 * cl_get_char:
 *    Get an option with a character value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    The option character (0 if not found).
 *                Aborts app on bad parameter.
 */

char
cl_get_char(char ident, int argc, char **argv)
{
	int i, found;
	char *sp;
	char ch = 0;

	found = FALSE;

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Option character missing");
			if (strlen(sp) > 1)
				arg_error(argv[i], "Option invalid");
			ch = *sp;

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_charopt returns %c\n", ch));

	return ch;
}


/*
 * cl_get_int:
 *    Get an option with an integer value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    The option value (0 if not found).
 *                Aborts app on bad parameter.
 */

int
cl_get_int(char ident, int argc, char **argv)
{
	int i, found;
	char *sp;
	int val = 0;

	found = FALSE;

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Option value missing");
			if (!sscanf(sp, "%i", &val))
				arg_error(argv[i], "Integer expected");

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_int returns %d\n", val));

	return val;
}


/*
 * cl_get_uint:
 *    Get an option with a positive integer value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    The option value (-1 if not found).
 *                Aborts app on bad parameter.
 */

int
cl_get_uint(char ident, int argc, char **argv)
{
	int i, found;
	char *sp;
	int val = -1;

	found = FALSE;

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Option value missing");
			if (!sscanf(sp, "%i", &val))
				arg_error(argv[i], "Positive integer expected");

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_uint returns %d\n", val));

	return val;
}


/*
 * cl_get_longlong:
 *    Get an option with a 64-bit value
 *
 *    Parameter:
 *          ident       the parameter identifier character
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    The option value (0 if not found).
 *                Aborts app on bad parameter.
 */

uint64_t
cl_get_longlong(char ident, int argc, char **argv)
{
	int i, found;
	char *sp;
	uint64_t val = 0;

	found = FALSE;

	for (i = 0; i < argc && !found; i++) {
		if (!argv[i] || argv[i][0] != '-')
			continue;

		if (argv[i][1] == ident) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp)
				arg_error(argv[i], "Option value missing");
			if (!sscanf(sp, "%qi", (long long *)(void *)&val))
				arg_error(argv[i], "Integer expected");

			if (!argv[i][2])
				argv[i + 1] = NULL;

			argv[i] = NULL;
			found = TRUE;
		}
	}
	DEB(9, ("cl_get_longlong returns %qd\n", val));

	return val;
}
