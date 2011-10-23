/*	$NetBSD: iscsic_test.c,v 1.1 2011/10/23 21:11:23 agc Exp $	*/

/*-
 * Copyright (c) 2006,2011 The NetBSD Foundation, Inc.
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

#ifdef ISCSI_TEST_MODE

#include <ctype.h>
#include <err.h>
#include <fcntl.h>

#include "iscsic_globals.h"
#include "iscsi_test.h"

#define WRITE_BUFFER_SIZE  (64 * 1024)


#define hexdig(c)    ((c & 0x40) ? (c & 0x07) + 9 : c & 0x0f)


typedef struct {
	char *name;
	int offset;
	int size;
} off_sel_t;

typedef struct {
	char *name;
	int opt;
} str_sel_t;

typedef struct {
	char *name;
	int opt;
	int type;
} mlopt_sel_t;


off_sel_t offtab[] = {
	{"op", 0, 1},				/* Opcode */
	{"dsl", 5, 3},				/* Data Segment Length */
	{"lun", 8, 8},				/* SCSI,LUN */
	{"itt", 16, 4},				/* Initiator Task Tag */
	{"ttt", 20, 4},				/* Target Transfer Tag */
	{"cmdsn", 24, 4},			/* Command Sequence Number (tx) */
	{"statsn", 24, 4},			/* Status Sequence Number (rx) */
	{"expstatsn", 28, 4},		/* Expected StatSN (tx) */
	{"expcmdsn", 28, 4},		/* Expected CmdSN (rx) */
	{"maxcmdsn", 32, 4},		/* Maximum CmdSN (rx) */
	{"datasn", 36, 4},			/* Data Sequence Number */
	{"expdatasn", 36, 4},		/* Expected DataSN */
	{"boff", 40, 4},			/* Buffer Offset */
	{"rescnt", 44, 4},			/* Residual Count */
	{"hdigest", ISCSITEST_OFFSET_HEADERDIGEST, 4},	/* Header Digest */
	{"data", ISCSITEST_OFFSET_DATA, 0},	/* Data Segment */
	{"ddigest", ISCSITEST_OFFSET_DATADIGEST, 4},	/* Data Digest */
	{"drvcmdsn", ISCSITEST_OFFSET_DRV_CMDSN, 4},	/* Driver CmdSN */
	{NULL, 0, 0}
};


str_sel_t pdutab[] = {
	{"any", ANY_PDU},
	{"command", COMMAND_PDU},
	{"response", RESPONSE_PDU},
	{"taskreq", TASK_REQ_PDU},
	{"taskrsp", TASK_RSP_PDU},
	{"dataout", DATA_OUT_PDU},
	{"datain", DATA_IN_PDU},
	{"r2t", R2T_PDU},
	{"asynch", ASYNCH_PDU},
	{"textreq", TEXT_REQ_PDU},
	{"textrsp", TEXT_RSP_PDU},
	{"loginreq", LOGIN_REQ_PDU},
	{"loginrsp", LOGIN_RSP_PDU},
	{"logoutreq", LOGOUT_REQ_PDU},
	{"logoutrsp", LOGOUT_RSP_PDU},
	{"snack", SNACK_PDU},
	{"reject", REJECT_PDU},
	{"nopout", NOP_OUT_PDU},
	{"nopin", NOP_IN_PDU},
	{"invalid", INVALID_PDU},
	{NULL, 0}
};


str_sel_t authtab[] = {
	{"secneg", NEG_SECURITY},	/* First security negotiation PDU */
	{"chapalg", NEG_CHAP_ALG},	/* CHAP authentication: algorithm specification. */
	{"chapnr", NEG_CHAP_NAME_RESPONSE},	/* CHAP: Name and Response. */
	{"opneg", NEG_OP_NEG},		/* First operational negotiation PDU. */
	{NULL, 0}
};


str_sel_t offkindtab[] = {
	{"aa", ABSOLUTE_ANY},	  /* Absolute to start of connection, any kind */
	{"ra", RELATIVE_ANY},	  /* Relative to last processed PDU, any kind */
	{"ar", ABSOLUTE_RX},	  /* Absolute to start of connection, any kind, rx */
	{"rr", RELATIVE_RX},	  /* Relative to last processed PDU, any kind, rx */
	{"at", ABSOLUTE_TX},	  /* Absolute to start of connection, any kind, tx */
	{"rt", RELATIVE_TX},	  /* Relative to last processed PDU, any kind, tx */
	{"ak", ABSOLUTE_PDUKIND}, /* Absolute to start of connection, same PDU kind */
	{"ra", RELATIVE_PDUKIND}, /* Relative to last processed PDU, same PDU kind */
	{NULL, 0}
};


/*
 * get_hexbyte:
 *    Get a hex byte from argument.
 *
 *    Parameter:
 *       arg      argument
 *
 *    Returns:    value of byte, or -1 if not a hex byte.
 */

STATIC int
get_hexbyte(char *arg)
{
	if (isxdigit(arg[0]) && !arg[1]) {
		return hexdig(arg[0]);
	}
	if (isxdigit(arg[1]) && !arg[2]) {
		return hexdig(arg[0]) << 4 | hexdig(arg[1]);
	}
	return -1;
}


/*
 * get_connid:
 *    Get connection ID. Queries Daemon for numeric ID if the user specifies
 *    a symbolic connection name.
 *
 *    Parameter:
 *       argc, argv (shifted)
 *       session
 *
 *    Returns:    connection ID or 0 if OK - else it doesn't return at all.
 */

STATIC uint32_t
get_connid(int argc, char **argv, uint32_t session)
{
	iscsid_sym_id_t sid;
	iscsid_get_connection_info_req_t req;
	iscsid_response_t *rsp;

	if (!cl_get_id('C', &sid, argc, argv)) {
		return 0;
	}
	if (!sid.id) {
		req.session_id.name[0] = '\0';
		req.session_id.id = session;
		strlcpy(req.connection_id.name, sid.name,
			sizeof(req.connection_id.name));
		req.connection_id.id = 0;

		send_request(ISCSID_GET_CONNECTION_INFO, sizeof(req), &req);
		rsp = get_response(FALSE);
		if (rsp->status) {
			status_error(rsp->status);
		} else {
			sid.id = ((iscsid_get_connection_info_rsp_t *) &rsp->parameter)
						->connection_id.id;
		}
		free_response(rsp);
	}
	return sid.id;
}

/*
 * get_offset:
 *    Get a PDU modification record
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *          modp        pointer to PDU modification record
 *
 *    Returns:    1 if parameter present, 0 if not.
 *                Aborts app on bad parameter.
 */

STATIC int
get_pdumod(int argc, char **argv, iscsi_pdu_mod_t * modp)
{
	int i, j, found;
	char *sp;
	int off = 0, size = 4, flags = 0;

	for (i = 0, found = FALSE; i < argc; i++) {
		if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'o') {
			found = TRUE;
			break;
		}
	}
	if (!found) {
		return 0;
	}
	sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);
	if (!sp || !*sp) {
		arg_error(argv[i], "Offset value missing");
	}
	if (isdigit(*sp)) {
		if (!sscanf(sp, "%i", &off)) {
			arg_error(argv[i], "Numeric value expected");
		}
	} else {
		for (j = 0; offtab[j].name != NULL; j++) {
			if (strcmp(sp, offtab[j].name) == 0) {
				break;
			}
		}
		if (offtab[j].name == NULL) {
			arg_error(argv[i], "Invalid offset '%s'", sp);
		}
		off = offtab[j].offset;
		size = offtab[j].size;
	}

	if (!argv[i][2]) {
		argv[i] = argv[i + 1] = NULL;
		i += 2;
	} else
		argv[i++] = NULL;

	if (i >= argc)
		arg_missing("Modification Value");

	if (argv[i] && argv[i][0] == '-' && argv[i][1] == 's') {
		sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);

		if (!sp || !*sp)
			arg_error(argv[i], "Size value missing");
		if (sscanf(sp, "%i", &size) == 0)
			arg_error(argv[i], "Numeric value expected");
		if (size <= 0 || size > 8)
			arg_error(argv[i], "Invalid size %d", size);

		if (!argv[i][2]) {
			argv[i] = argv[i + 1] = NULL;
			i += 2;
		} else
			argv[i++] = NULL;

		if (i >= argc)
			arg_missing("Modification Value");
	}

	if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'a') {
		flags |= ISCSITEST_MOD_FLAG_ADD_VAL;
		argv[i++] = NULL;
		if (i >= argc)
			arg_missing("Modification Value");
	}

	sp = argv[i];
	if (!sp || (!isxdigit(*sp) && *sp != '-' && *sp != '+'))
		arg_missing("Modification Value");

	if (i + 1 < argc && argv[i + 1] && isxdigit(argv[i + 1][0])) {
		char *dp = modp->value;
		int val, n = size;

		do {
			if (0 > (val = get_hexbyte(sp)))
				arg_error(argv[i], "Hex byte expected");
			if (n) {
				*dp++ = (uint8_t) val;
				n--;
			}
			argv[i++] = NULL;
			sp = (i < argc) ? argv[i] : NULL;
		} while (sp && isxdigit(*sp));
	} else {
		int64_t val;

		if (!sscanf(sp, "%qi", &val))
			arg_error(argv[i], "Numeric value expected");
		*((int64_t *) modp->value) = val;
		argv[i] = NULL;
		modp->flags |= ISCSITEST_MOD_FLAG_REORDER;
	}

	modp->offset = (uint32_t) off;
	modp->size = (uint16_t) size;
	modp->flags = (uint16_t) flags;

	DEB(9, ("get_pdumod offset %d, size %d, flags %x, val %qd\n", off, size,
			flags, *((uint64_t *) modp->value)));

	return 1;
}

/*
 * get_updopt:
 *    Get PDU update options for send_pdu
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    Value of option, 0 if not present.
 *                Aborts app on bad parameter.
 */

STATIC int
get_updopt(int argc, char **argv, int send)
{
	int i;
	char *sp;
	int val = ISCSITEST_SFLAG_UPDATE_FIELDS;

	for (i = 0;
	     i < argc && (!argv[i] || argv[i][0] != '-' || argv[i][1] != 'u');
	     i++) {
	}
	if (i >= argc) {
		return 0;
	}
	sp = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);
	if (!sp || !*sp || *sp == '-') {
		argv[i] = NULL;
		return val;
	}

	while (*sp) {
		switch (*sp) {
		case 'h':
			val |= ISCSITEST_SFLAG_NO_HEADER_DIGEST;
			break;

		case 'i':
			val |= ISCSITEST_SFLAG_NO_DATA_DIGEST;
			break;

		case 'p':
			if (send) {
				val |= ISCSITEST_SFLAG_NO_PADDING;
				break;
			}
			/* FALLTHROUGH */
		default:
			arg_error(argv[i], "Invalid update option '%c'", *sp);
		}
		sp++;
	}
	if (!argv[i][2]) {
		argv[i + 1] = NULL;
	}
	argv[i] = NULL;

	return val;
}

/*
 * get_stropt:
 *    Get an option specified by a string
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *          flagp       pointer to flags result
 *
 *    Returns:    Value of string, -1 if not present.
 *                Aborts app on bad parameter.
 */

STATIC int
get_stropt(int argc, char **argv, char selc, str_sel_t * table)
{
	int i, j;
	char *sp;
	int val = -1;

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-') {
			continue;
		}

		if (argv[i][1] == selc) {
			sp = (argv[i][2]) ? &argv[i][2]
				: ((i + 1 < argc) ? argv[i + 1] : NULL);

			if (!sp || !*sp) {
				arg_error(argv[i], "Option missing");
			}
			for (j = 0; table[j].name != NULL; j++) {
				if (strcmp(sp, table[j].name) == 0) {
					break;
				}
			}
			if (table[j].name == NULL) {
				arg_error(argv[i], "Invalid option '%s'", sp);
			}
			val = table[j].opt;

			if (!argv[i][2]) {
				argv[i + 1] = NULL;
			}
			argv[i] = NULL;
			break;
		}
	}
	DEB(10, ("get_stropt -%c returns %d\n", selc, val));

	return val;
}

/*
 * get_mlint:
 *    Get a multiletter integer option
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    Value of option, -1 if not present.
 *                Aborts app on bad parameter.
 */

STATIC int
get_mlint(int argc, char **argv, char *opt)
{
	int i;
	char *sp;
	int val = -1;

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-') {
			continue;
		}
		if (strcmp(&argv[i][1], opt) == 0) {
			sp = (i + 1 < argc) ? argv[i + 1] : NULL;

			if (!sp || !*sp) {
				arg_error(argv[i], "Missing numeric value");
			}
			if (sscanf(sp, "%i", &val) == 0) {
				arg_error(argv[i], "Invalid number");
			}
			argv[i] = argv[i + 1] = NULL;
			break;
		}
	}
	DEB(10, ("get_mlint returns %d\n", val));

	return val;
}

/*
 * get_mlbool:
 *    Get a multiletter boolean option
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *
 *    Returns:    Value of option (0 or 1), -1 if not present.
 *                Aborts app on bad parameter.
 */

STATIC int
get_mlbool(int argc, char **argv, char *opt)
{
	int i;
	char *sp;
	int val = -1;

	for (i = 0; i < argc; i++) {
		if (!argv[i] || argv[i][0] != '-') {
			continue;
		}
		if (strcmp(&argv[i][1], opt) == 0) {
			sp = (i + 1 < argc) ? argv[i + 1] : NULL;

			if (!sp || !*sp) {
				arg_error(argv[i], "Missing truth value");
			}
			if (sp[1]) {
				arg_error(argv[i], "Invalid truth value '%s'", sp);
			}
			switch (*sp) {
			case 't':
			case 'T':
			case '1':
				val = 1;
				break;

			case 'f':
			case 'F':
			case '0':
				val = 0;
				break;

			default:
				arg_error(argv[i], "Bad truth value '%c'", *sp);
			}

			argv[i] = argv[i + 1] = NULL;
			break;
		}
	}
	DEB(10, ("get_mlbool returns %d\n", val));

	return val;
}

/*
 * get_blob:
 *    Get a large binary value.
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *          dp          destination
 *          maxlen      maximum size
 *          name        name of parameter for error message if missing
 *
 *    Returns:    Size of value in bytes.
 *                Aborts app on bad parameter.
 */

STATIC int
get_blob(char ident, int argc, char **argv, uint8_t * dp, int maxlen, char *name)
{
	int i, l;
	char *arg;
	int size = 0;

	for (i = 0;
	     i < argc && (!argv[i] || argv[i][0] != '-' || argv[i][1] != ident);
	     i++) {
	}
	if (i >= argc) {
		arg_missing(name);
	}
	arg = (argv[i][2]) ? &argv[i][2] : ((i + 1 < argc) ? argv[i + 1] : NULL);
	if (!arg || !*arg) {
		arg_missing(name);
	}
	if (!argv[i][2]) {
		argv[i++] = NULL;
	}
	while (arg && *arg != '-') {
		if (0 <= (l = get_hexbyte(arg))) {
			if (!maxlen) {
				arg_error(argv[i], "Buffer size exceeded");
			}
			*dp++ = (uint8_t) l;
			size++;
			maxlen--;
		} else {
			if (arg[0] == '.') {
				arg++;
			}
			if (!arg[0]) {
				arg_error(argv[i], "Leading dot removed, nothing left");
			}
			l = strlen(arg);
			if (l >= maxlen) {
				arg_error(argv[i], "String exceeds buffer size");
			}
			strlcpy(dp, arg, maxlen);
			dp += l;
			size += l;
			maxlen -= l;
		}
		argv[i++] = NULL;
		if (i >= argc) {
			break;
		}
		arg = argv[i];
	}
	if (!size) {
		arg_missing(name);
	}
	DEB(10, ("get_blob returns %d\n", size));

	return size;
}


/*
 * get_file:
 *    Get a large binary value from file, or, if no file parameter is present,
 *    allocate buffer for blob.
 *
 *    Parameter:
 *          argc, argv  program parameters (shifted)
 *          pbufp       pointer to buffer pointer
 *          plen        pointer to size of buffer. The initial size contained
 *                      in this parameter is used as an offset into the target
 *                      buffer and is added to the allocation size.
 *          psize       pointer to int to receive size option.
 *          rd          TRUE if read, FALSE if write
 *
 *    Returns:    handle of file if file parameter present, -1 if not.
 *                Aborts app on bad parameter.
 */

STATIC int
get_file(int argc, char **argv, caddr_t *pbufp, uint32_t *plen, int *psize,
		 int rd)
{
	char fname[ISCSI_STRING_LENGTH];
	int hnd = -1, size;
	off_t fsize = 0;
	uint8_t *bp = buf;

	size = *psize = cl_get_int('s', argc, argv);

	if (cl_get_string('f', fname, argc, argv)) {
		hnd = open(fname, (rd) ? O_RDONLY : (O_WRONLY | O_CREAT | O_TRUNC),
					0x666);
		if (hnd < 0) {
			io_error("Opening file '%s'", fname);
		}
		if (rd) {
			fsize = lseek(hnd, 0, SEEK_END);
			lseek(hnd, 0, SEEK_SET);
			if (!fsize) {
				gen_error("File '%s' is empty", fname);
			}
		}
	}
	if (!size) {
		if (hnd >= 0) {
			size = (rd) ? fsize : WRITE_BUFFER_SIZE;
		} else {
			size = BUF_SIZE - *plen;
		}
	}

	if (size < fsize) {
		fsize = size;
	}
	size += *plen;

	if (size > BUF_SIZE && (bp = calloc(1, size)) == NULL) {
		gen_error("Can't allocate %d bytes for buffer", size);
	}
	(void) memset(bp, 0x0, size);

	*pbufp = (caddr_t) bp;
	bp += *plen;

	*plen = size;

	if (rd && hnd >= 0) {
		if (read(hnd, bp, fsize) != fsize) {
			io_error("Reading file '%s'", fname);
		}
		close(hnd);
	}

	return hnd;
}


/*
 * t_define:
 *    Handle the test define command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

STATIC int
t_define(int argc, char **argv)
{
	iscsi_test_define_parameters_t def = { 0 };
	int rc;

	if ((def.test_id = cl_get_int('T', argc, argv)) == 0) {
		arg_missing("define: Test ID");
	}
	if ((def.session_id = get_sessid(argc, argv, TRUE)) != 0) {
		def.connection_id = get_connid(argc, argv, def.session_id);
	}
	if ((rc = get_mlint(argc, argv, "mr2t")) != -1) {
		def.r2t_val = (uint32_t) rc;
		def.options |= ISCSITEST_NEGOTIATE_R2T;
	}
	if ((rc = get_mlint(argc, argv, "mbl")) != -1) {
		def.maxburst_val = (uint32_t) rc;
		def.options |= ISCSITEST_NEGOTIATE_MAXBURST;
	}
	if ((rc = get_mlint(argc, argv, "fbl")) != -1) {
		def.firstburst_val = (uint32_t) rc;
		def.options |= ISCSITEST_NEGOTIATE_FIRSTBURST;
	}
	if (get_mlbool(argc, argv, "ir2t") > 0) {
		def.options |= ISCSITEST_OVERRIDE_INITIALR2T;
	}
	if (get_mlbool(argc, argv, "imm") == 0) {
		def.options |= ISCSITEST_OVERRIDE_IMMDATA;
	}
	if ((rc = get_mlbool(argc, argv, "tcmd")) != -1) {
		def.options |= (rc) ? ISCSITEST_OPT_ENABLE_CCB_TIMEOUT
					: ISCSITEST_OPT_DISABLE_CCB_TIMEOUT;
	}
	if ((rc = get_mlbool(argc, argv, "tconn")) != -1) {
		def.options |= (rc) ? ISCSITEST_OPT_ENABLE_CONN_TIMEOUT
			: ISCSITEST_OPT_DISABLE_CONN_TIMEOUT;
	}
	if ((rc = get_mlint(argc, argv, "rlrx")) != -1) {
		def.options |= ISCSITEST_OPT_USE_RANDOM_RX;
		def.lose_random_rx = (uint8_t) rc;
	}
	if ((rc = get_mlint(argc, argv, "rltx")) != -1) {
		def.options |= ISCSITEST_OPT_USE_RANDOM_TX;
		def.lose_random_tx = (uint8_t) rc;
	}
	check_extra_args(argc, argv);

	DEB(1, ("DefTest: tid=%d, opt=%x, lrtx=%d, lrrx=%d, sid=%d, cid=%d\n",
			def.test_id, def.options, def.lose_random_tx, def.lose_random_rx,
			def.session_id, def.connection_id));
	DEB(1, ("         r2t=%d, mbl=%d, fbl=%d, erl=%d\n",
			def.r2t_val, def.maxburst_val, def.firstburst_val, def.erl_val));

	rc = ioctl(driver, ISCSI_TEST_DEFINE, &def);

	if (def.status) {
		status_error(def.status);
	}
	if (rc) {
		io_error("define: I/O command");
	}
	return rc;
}


/*
 * t_add_pdumod:
 *    Handle the test add_pdumod command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

STATIC int
t_add_pdumod(int argc, char **argv)
{
	iscsi_test_add_modification_parameters_t def = { 0 };
	int rc, rd, hnd = -1;

	if ((def.test_id = cl_get_int('T', argc, argv)) == 0) {
		arg_missing("add_pdumod: Test ID");
	}
	if ((def.pdu_offset = cl_get_uint('p', argc, argv)) == (uint32_t) -1) {
		arg_missing("add_pdumod: PDU Number");
	}
	if ((rc = get_stropt(argc, argv, 'b', offkindtab)) != -1) {
		def.which_offset = rc;
	}
	if ((rc = get_stropt(argc, argv, 'k', pdutab)) != -1) {
		def.which_pdu = rc;
	}
	def.options = get_updopt(argc, argv, FALSE);

	if (cl_get_opt('w', argc, argv)) {
		def.options |= ISCSITEST_OPT_WAIT_FOR_COMPLETION;
	}
	if (cl_get_opt('n', argc, argv)) {
		def.options |= ISCSITEST_OPT_NO_RESPONSE_PDU;
	}
	if ((rc = get_mlbool(argc, argv, "tcmd")) != -1) {
		def.options |= (rc) ? ISCSITEST_OPT_ENABLE_CCB_TIMEOUT
							: ISCSITEST_OPT_DISABLE_CCB_TIMEOUT;
	}
	if ((rc = get_mlbool(argc, argv, "tconn")) != -1) {
		def.options |= (rc) ? ISCSITEST_OPT_ENABLE_CONN_TIMEOUT
			: ISCSITEST_OPT_DISABLE_CONN_TIMEOUT;
	}
	if ((rc = get_mlint(argc, argv, "rlrx")) != -1) {
		def.options |= ISCSITEST_OPT_USE_RANDOM_RX;
		def.lose_random_rx = (uint8_t) rc;
	}
	if ((rc = get_mlint(argc, argv, "rltx")) != -1) {
		def.options |= ISCSITEST_OPT_USE_RANDOM_TX;
		def.lose_random_tx = (uint8_t) rc;
	}


	if ((rd = cl_get_opt('r', argc, argv)) != 0)
		hnd = get_file(argc, argv, &def.pdu_ptr, &def.pdu_size, &rc, FALSE);

	if (cl_get_opt('D', argc, argv))
		def.options |= ISCSITEST_OPT_DISCARD_PDU;

	if (cl_get_opt('K', argc, argv))
		def.options |= ISCSITEST_KILL_CONNECTION;

	if (!(def.options & (ISCSITEST_OPT_DISCARD_PDU |
						 ISCSITEST_KILL_CONNECTION))) {
		iscsi_pdu_mod_t *modp = def.mod_ptr = (iscsi_pdu_mod_t *) buf;

		while (get_pdumod(argc, argv, modp)) {
			if (modp->offset == ISCSITEST_OFFSET_HEADERDIGEST)
				def.options |= ISCSITEST_SFLAG_NO_HEADER_DIGEST;
			else if (modp->offset == ISCSITEST_OFFSET_DATADIGEST)
				def.options |= ISCSITEST_SFLAG_NO_DATA_DIGEST;

			modp++;
			def.num_pdu_mods++;
		}
		if (!rd && !def.num_pdu_mods)
			arg_missing("add_pdumod: PDU Modification");
	}

	check_extra_args(argc, argv);

	DEB(1, ("AddMod: tid=%d, opt=%x, offs=%d, which=%d, pdukind=%d, "
			"num=%d, rd_size=%d\n",
			def.test_id, def.options, def.pdu_offset, def.which_offset,
			def.which_pdu, def.num_pdu_mods, def.pdu_size));

#ifdef ISCSI_DEBUG
	if (def.num_pdu_mods)
		DEB(1, ("    0:  offset=%d, size=%d, flags=%d, val=%qx\n",
				def.mod_ptr[0].offset, def.mod_ptr[0].size,
				def.mod_ptr[0].flags, *((uint64_t *) def.mod_ptr[0].value)));
#endif

	rc = ioctl(driver, ISCSI_TEST_ADD_MODIFICATION, &def);

	if (def.status)
		status_error(def.status);
	if (rc)
		io_error("add_pdumod: I/O command");

	if (!rd)
		return 0;

	if (!def.pdu_actual_size)
		gen_error("add_pdumod: No data read");

	if (hnd >= 0) {
		rc = def.pdu_actual_size;
		rc -= write(hnd, def.pdu_ptr, def.pdu_actual_size);
		close(hnd);
		if (rc) {
			io_error("add_pdumod: Writing file");
		}
	} else {
		dump_data("PDU Data", def.pdu_ptr, def.pdu_actual_size);
	}
	return rc;
}


/*
 * t_add_negmod:
 *    Handle the test add_negmod command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

STATIC int
t_add_negmod(int argc, char **argv)
{
	iscsi_test_add_negotiation_parameters_t def = { 0 };
	iscsi_test_negotiation_descriptor_t *neg;
	int hnd, rc, size;

	if ((def.test_id = cl_get_int('T', argc, argv)) == 0) {
		arg_missing("add_negmond: Test ID");
	}
	def.neg_descriptor_size = sizeof(iscsi_test_negotiation_descriptor_t);
	hnd = get_file(argc, argv, &def.neg_descriptor_ptr,
					&def.neg_descriptor_size, &size, TRUE);
	neg = (iscsi_test_negotiation_descriptor_t *) def.neg_descriptor_ptr;
	neg->size = def.neg_descriptor_size
				- sizeof(iscsi_test_negotiation_descriptor_t);

	if (hnd < 0) {
		neg->size = get_blob('d', argc, argv, neg->value, neg->size,
							"Negotiation Value");
		if (size) {
			neg->size = size;
		}
		def.neg_descriptor_size = neg->size
			+ sizeof(iscsi_test_negotiation_descriptor_t);
	}

/* XXX - agc */
	if (0 > (neg->state = get_stropt(argc, argv, 'p', authtab)))
		arg_missing("add_negmod: Negotiation Phase");

	if (cl_get_opt('r', argc, argv))
		neg->flags |= ISCSITEST_NEGOPT_REPLACE;

	check_extra_args(argc, argv);

	DEB(1, ("AddNeg: tid=%d, phase=%d, flags=%x, hnd=%d, size=%d, "
			"ptr=%x, data=%s\n",
			def.test_id, neg->state, neg->flags, hnd, def.neg_descriptor_size,
			(int) def.neg_descriptor_ptr, neg->value));

	rc = ioctl(driver, ISCSI_TEST_ADD_NEGOTIATION, &def);

	if (def.status) {
		status_error(def.status);
	}
	if (rc) {
		io_error("add_negmod: I/O command");
	}
	return rc;
}


/*
 * t_send_pdu:
 *    Handle the test send_pdu command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

STATIC int
t_send_pdu(int argc, char **argv)
{
	iscsi_test_send_pdu_parameters_t def = { 0 };
	int rc, size;

	if ((def.test_id = cl_get_int('T', argc, argv)) == 0) {
		arg_missing("send_pdu: Test ID");
	}
	def.options = get_updopt(argc, argv, TRUE);

	if (cl_get_opt('K', argc, argv)) {
		def.options |= ISCSITEST_KILL_CONNECTION;
	}
/* weird */
	if (0 > get_file(argc, argv, &def.pdu_ptr, &def.pdu_size, &size, TRUE)) {
		def.pdu_size = get_blob('d', argc, argv, buf, def.pdu_size, "PDU Data");
		if (size) {
			def.pdu_size = size;
		}
	}
	check_extra_args(argc, argv);

	rc = ioctl(driver, ISCSI_TEST_SEND_PDU, &def);

	if (def.status) {
		status_error(def.status);
	}
	if (rc) {
		io_error("send_pdu: I/O command");
	}
	return rc;
}


/*
 * t_iocommand:
 *    Handle the test iocommand command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

STATIC int
t_iocommand(int argc, char **argv)
{
	iscsi_iocommand_parameters_t io = { 0 };
	int nodata, rd, rc, size, hnd = -1;

	if ((io.session_id = get_sessid(argc, argv, FALSE)) == 0) {
		return 1;
	}
	io.connection_id = get_connid(argc, argv, io.session_id);

	io.lun = cl_get_longlong('l', argc, argv);

	if (cl_get_opt('i', argc, argv)) {
		io.options.immediate = 1;
	}
	rd = cl_get_opt('r', argc, argv);

	nodata = cl_get_opt('n', argc, argv);

	if (rd && nodata) {
		gen_error("iocommand: Conflicting options -r -n\n");
	}
	io.req.cmdlen = get_blob('c', argc, argv, io.req.cmd, sizeof(io.req.cmd),
							"CCB");

	if ((rc = cl_get_int('S', argc, argv)) != 0) {
		if (rc < 0 || rc > 16) {
			gen_error("Iocommand: Invalid CCB size %d", rc);
		}
		io.req.cmdlen = rc;
	}

	if (!nodata) {
/* weird */
		if (0 > (hnd = get_file(argc, argv, &io.req.databuf,
					   	(uint32_t *) &io.req.datalen, &size, !rd)) && !rd) {
			io.req.datalen = get_blob('d', argc, argv, io.req.databuf,
									  io.req.datalen, "Command Data");
			if (size) {
				io.req.datalen = size;
			}
		}
	}

	check_extra_args(argc, argv);

	DEB(1, ("IoCmd: sid=%d, rd=%d, cmdlen=%d, cmd=%02x %02x %02x.., "
			"len=%d, ptr=%x\n",
			io.session_id, rd, io.req.cmdlen, io.req.cmd[0], io.req.cmd[1],
			io.req.cmd[2], (int) io.req.datalen, (int) io.req.databuf));

	if ((rc = do_ioctl(&io, rd)) != 0) {
		return rc;
	}
	if (rd) {
		if (!io.req.datalen_used) {
			gen_error("iocommand: No Data!");
		}
		if (hnd >= 0) {
			rc = io.req.datalen_used;
			rc -= write(hnd, io.req.databuf, io.req.datalen_used);
			close(hnd);
			if (rc) {
				io_error("iocommand: Writing file");
			}
		} else {
			dump_data("Command Data", io.req.databuf, io.req.datalen_used);
		}
	}

	return rc;
}


/*
 * t_cancel:
 *    Handle the test cancel command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

STATIC int
t_cancel(int argc, char **argv)
{
	iscsi_test_cancel_parameters_t def = { 0 };
	int rc;

	if ((def.test_id = cl_get_int('T', argc, argv)) == 0) {
		arg_missing("cancel: Test ID");
	}
	check_extra_args(argc, argv);

	DEB(1, ("Cancel Test: tid=%d\n", def.test_id));

	rc = ioctl(driver, ISCSI_TEST_CANCEL, &def);

	if (def.status) {
		status_error(def.status);
	}
	if (rc) {
		io_error("cancel: I/O command");
	}
	return rc;
}


/*------------------------------------------------------------------------ */

static command_t tests[] = {
	{"define", t_define},
	{"add_pdumod", t_add_pdumod},
	{"add_negmod", t_add_negmod},
	{"send_pdu", t_send_pdu},
	{"iocommand", t_iocommand},
	{"cancel", t_cancel},
	{NULL, NULL}
};

/*
 * test:
 *    Handle the test command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK.
 */

int
test(int argc, char **argv)
{
	command_t *c;

	if (argv[0] == NULL || argv[0][1] == '-') {
		arg_missing("Test command");
	}
	for (c = tests; c->cmd != NULL; c++) {
		if (strcmp(c->cmd, argv[0]) == 0) {
			break;
		}
	}

	if (c->cmd == NULL) {
		gen_error("Unkown test command %s", argv[0]);
	}
	return c->proc(argc - 1, &argv[1]);
}


#endif
