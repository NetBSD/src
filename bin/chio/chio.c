/*	$NetBSD: chio.c,v 1.12 1999/09/08 04:57:37 thorpej Exp $	*/

/*
 * Copyright (c) 1996, 1998 Jason R. Thorpe <thorpej@and.com>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *	This product includes software developed by Jason R. Thorpe
 *	for And Communications, http://www.and.com/
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Additional Copyright (c) 1997, by Matthew Jacob, for NASA/Ames Research Ctr.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
    "@(#) Copyright (c) 1996, 1998 Jason R. Thorpe.  All rights reserved.");
__RCSID("$NetBSD: chio.c,v 1.12 1999/09/08 04:57:37 thorpej Exp $");
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/chio.h> 
#include <sys/cdio.h>	/* for ATAPI CD changer; too bad it uses a lame API */
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "pathnames.h"

extern	char *__progname;	/* from crt0.o */

int	main __P((int, char *[]));
static	void usage __P((void));
static	void cleanup __P((void));
static	int parse_element_type __P((char *));
static	int parse_element_unit __P((char *));
static	int parse_special __P((char *));
static	int is_special __P((char *));
static	const char *bits_to_string __P((int, const char *));

static	int do_move __P((char *, int, char **));
static	int do_exchange __P((char *, int, char **));
static	int do_position __P((char *, int, char **));
static	int do_params __P((char *, int, char **));
static	int do_getpicker __P((char *, int, char **));
static	int do_setpicker __P((char *, int, char **));
static	int do_status __P((char *, int, char **));
static	int do_ielem __P((char *, int, char **));
static	int do_cdlu __P((char *, int, char **));

/* Valid changer element types. */
const struct element_type elements[] = {
	{ "picker",		CHET_MT },
	{ "slot",		CHET_ST },
	{ "portal",		CHET_IE },
	{ "drive",		CHET_DT },
	{ NULL,			0 },
};

/* Valid commands. */
const struct changer_command commands[] = {
	{ "move", 	" <from ET> <from EU> <to ET> <to EU> [inv]",
	  do_move },

	{ "exchange",	" <src ET> <src EU> <dst1 ET> <dst1 EU>\n"
	                "\t\t [<dst2 ET> <dst2 EU>] [inv1] [inv2]",
	  do_exchange },

	{ "position",	" <to ET> <to EU> [inv]", do_position },

	{ "params",	"",
	  do_params },

	{ "getpicker",	"",
	  do_getpicker },

	{ "setpicker",	" <picker>",
	  do_setpicker },

	{ "status",	" [<element type>]",
	  do_status },

	{ "ielem", 	"",
	  do_ielem },

	{ "cdlu",	" load|unload <slot>\n"
	                "\t     abort",
	  do_cdlu },

	{ NULL,		NULL,
	  NULL },
};

/* Valid special words. */
const struct special_word specials[] = {
	{ "inv",		SW_INVERT },
	{ "inv1",		SW_INVERT1 },
	{ "inv2",		SW_INVERT2 },
	{ NULL,			0 },
};

static	int changer_fd;
static	const char *changer_name;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, i;

	while ((ch = getopt(argc, argv, "f:")) != -1) {
		switch (ch) {
		case 'f':
			changer_name = optarg;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	/* Get the default changer if not already specified. */
	if (changer_name == NULL)
		if ((changer_name = getenv(CHANGER_ENV_VAR)) == NULL)
			changer_name = _PATH_CH;

	/* Open the changer device. */
	if ((changer_fd = open(changer_name, O_RDWR, 0600)) == -1)
		err(1, "%s: open", changer_name);

	/* Register cleanup function. */
	if (atexit(cleanup))
		err(1, "can't register cleanup function");

	/* Find the specified command. */
	for (i = 0; commands[i].cc_name != NULL; ++i)
		if (strcmp(*argv, commands[i].cc_name) == 0)
			break;
	if (commands[i].cc_name == NULL)
		errx(1, "unknown command: %s", *argv);

	/* Skip over the command name and call handler. */
	++argv; --argc;
	exit ((*commands[i].cc_handler)(commands[i].cc_name, argc, argv));
	/* NOTREACHED */
}

static int
do_move(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct changer_move cmd;
	int val;

	/*
	 * On a move command, we expect the following:
	 *
	 * <from ET> <from EU> <to ET> <to EU> [inv]
	 *
	 * where ET == element type and EU == element unit.
	 */
	if (argc < 4) {
		warnx("%s: too few arguments", cname);
		usage();
	} else if (argc > 5) {
		warnx("%s: too many arguments", cname);
		usage();
	}
	(void) memset(&cmd, 0, sizeof(cmd));

	/* <from ET>  */
	cmd.cm_fromtype = parse_element_type(*argv);
	++argv; --argc;

	/* <from EU> */
	cmd.cm_fromunit = parse_element_unit(*argv);
	++argv; --argc;

	/* <to ET> */
	cmd.cm_totype = parse_element_type(*argv);
	++argv; --argc;

	/* <to EU> */
	cmd.cm_tounit = parse_element_unit(*argv);
	++argv; --argc;

	/* Deal with optional command modifier. */
	if (argc) {
		val = parse_special(*argv);
		switch (val) {
		case SW_INVERT:
			cmd.cm_flags |= CM_INVERT;
			break;

		default:
			errx(1, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOMOVE, &cmd))
		err(1, "%s: CHIOMOVE", changer_name);

	return (0);
}

static int
do_exchange(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct changer_exchange cmd;
	int val;

	/*
	 * On an exchange command, we expect the following:
	 *
  * <src ET> <src EU> <dst1 ET> <dst1 EU> [<dst2 ET> <dst2 EU>] [inv1] [inv2]
	 *
	 * where ET == element type and EU == element unit.
	 */
	if (argc < 4) {
		warnx("%s: too few arguments", cname);
		usage();
	} else if (argc > 8) {
		warnx("%s: too many arguments", cname);
		usage();
	}
	(void) memset(&cmd, 0, sizeof(cmd));

	/* <src ET>  */
	cmd.ce_srctype = parse_element_type(*argv);
	++argv; --argc;

	/* <src EU> */
	cmd.ce_srcunit = parse_element_unit(*argv);
	++argv; --argc;

	/* <dst1 ET> */
	cmd.ce_fdsttype = parse_element_type(*argv);
	++argv; --argc;

	/* <dst1 EU> */
	cmd.ce_fdstunit = parse_element_unit(*argv);
	++argv; --argc;

	/*
	 * If the next token is a special word or there are no more
	 * arguments, then this is a case of simple exchange.
	 * dst2 == src.
	 */
	if ((argc == 0) || is_special(*argv)) {
		cmd.ce_sdsttype = cmd.ce_srctype;
		cmd.ce_sdstunit = cmd.ce_srcunit;
		goto do_special;
	}

	/* <dst2 ET> */
	cmd.ce_sdsttype = parse_element_type(*argv);
	++argv; --argc;

	/* <dst2 EU> */
	cmd.ce_sdstunit = parse_element_unit(*argv);
	++argv; --argc;

 do_special:
	/* Deal with optional command modifiers. */
	while (argc) {
		val = parse_special(*argv);
		++argv; --argc;
		switch (val) {
		case SW_INVERT1:
			cmd.ce_flags |= CE_INVERT1;
			break;

		case SW_INVERT2:
			cmd.ce_flags |= CE_INVERT2;
			break;

		default:
			errx(1, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOEXCHANGE, &cmd))
		err(1, "%s: CHIOEXCHANGE", changer_name);

	return (0);
}

static int
do_position(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct changer_position cmd;
	int val;

	/*
	 * On a position command, we expect the following:
	 *
	 * <to ET> <to EU> [inv]
	 *
	 * where ET == element type and EU == element unit.
	 */
	if (argc < 2) {
		warnx("%s: too few arguments", cname);
		usage();
	} else if (argc > 3) {
		warnx("%s: too many arguments", cname);
		usage();
	}
	(void) memset(&cmd, 0, sizeof(cmd));

	/* <to ET>  */
	cmd.cp_type = parse_element_type(*argv);
	++argv; --argc;

	/* <to EU> */
	cmd.cp_unit = parse_element_unit(*argv);
	++argv; --argc;

	/* Deal with optional command modifier. */
	if (argc) {
		val = parse_special(*argv);
		switch (val) {
		case SW_INVERT:
			cmd.cp_flags |= CP_INVERT;
			break;

		default:
			errx(1, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOPOSITION, &cmd))
		err(1, "%s: CHIOPOSITION", changer_name);

	return (0);
}

/* ARGSUSED */
static int
do_params(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct changer_params data;

	/* No arguments to this command. */
	if (argc) {
		warnx("%s: no arguements expected", cname);
		usage();
	}

	/* Get params from changer and display them. */
	(void) memset(&data, 0, sizeof(data));
	if (ioctl(changer_fd, CHIOGPARAMS, &data))
		err(1, "%s: CHIOGPARAMS", changer_name);

	(void) printf("%s: %d slot%s, %d drive%s, %d picker%s",
	    changer_name,
	    data.cp_nslots, (data.cp_nslots > 1) ? "s" : "",
	    data.cp_ndrives, (data.cp_ndrives > 1) ? "s" : "",
	    data.cp_npickers, (data.cp_npickers > 1) ? "s" : "");
	if (data.cp_nportals)
		(void) printf(", %d portal%s", data.cp_nportals,
		    (data.cp_nportals > 1) ? "s" : "");
	(void) printf("\n%s: current picker: %d\n", changer_name,
	    data.cp_curpicker);

	return (0);
}

/* ARGSUSED */
static int
do_getpicker(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	int picker;

	/* No arguments to this command. */
	if (argc) {
		warnx("%s: no arguments expected", cname);
		usage();
	}

	/* Get current picker from changer and display it. */
	if (ioctl(changer_fd, CHIOGPICKER, &picker))
		err(1, "%s: CHIOGPICKER", changer_name);

	(void) printf("%s: current picker: %d\n", changer_name, picker);

	return (0);
}

static int
do_setpicker(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	int picker;

	if (argc < 1) {
		warnx("%s: too few arguments", cname);
		usage();
	} else if (argc > 1) {
		warnx("%s: too many arguments", cname);
		usage();
	}

	picker = parse_element_unit(*argv);

	/* Set the changer picker. */
	if (ioctl(changer_fd, CHIOSPICKER, &picker))
		err(1, "%s: CHIOSPICKER", changer_name);

	return (0);
}

static int
do_status(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct changer_element_status cmd;
	struct changer_params data;
	u_int8_t *statusp;
	int i, chet, schet, echet;
	size_t count;
	char *description;

	/*
	 * On a status command, we expect the following:
	 *
	 * [<ET>]
	 *
	 * where ET == element type.
	 *
	 * If we get no arguments, we get the status of all
	 * known element types.
	 */
	if (argc > 1) {
		warnx("%s: too many arguments", cname);
		usage();
	}

	/*
	 * Get params from changer.  Specifically, we need the element
	 * counts.
	 */
	(void) memset(&data, 0, sizeof(data));
	if (ioctl(changer_fd, CHIOGPARAMS, &data))
		err(1, "%s: CHIOGPARAMS", changer_name);

	if (argc)
		schet = echet = parse_element_type(*argv);
	else {
		schet = CHET_MT;
		echet = CHET_DT;
	}

	for (chet = schet; chet <= echet; ++chet) {
		switch (chet) {
		case CHET_MT:
			count = data.cp_npickers;
			description = "picker";
			break;

		case CHET_ST:
			count = data.cp_nslots;
			description = "slot";
			break;

		case CHET_IE:
			count = data.cp_nportals;
			description = "portal";
			break;

		case CHET_DT:
			count = data.cp_ndrives;
			description = "drive";
			break;

		default:
			/* To appease gcc -Wuninitialized. */
			count = 0;
			description = NULL;
		}

		if (count == 0) {
			if (argc == 0)
				continue;
			else {
				(void) printf("%s: no %s elements\n",
				    changer_name, description);
				return (0);
			}
		}

		/* Allocate storage for the status bytes. */
		if ((statusp = (u_int8_t *)malloc(count)) == NULL)
			errx(1, "can't allocate status storage");

		(void) memset(statusp, 0, count);
		(void) memset(&cmd, 0, sizeof(cmd));

		cmd.ces_type = chet;
		cmd.ces_data = statusp;

		if (ioctl(changer_fd, CHIOGSTATUS, &cmd)) {
			free(statusp);
			err(1, "%s: CHIOGSTATUS", changer_name);
		}

		/* Dump the status for each element of this type. */
		for (i = 0; i < count; ++i) {
			(void) printf("%s %d: %s\n", description, i,
			    bits_to_string(statusp[i], CESTATUS_BITS));
		}

		free(statusp);
	}

	return (0);
}

/* ARGSUSED */
static int
do_ielem(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	if (ioctl(changer_fd, CHIOIELEM, NULL))
		err(1, "%s: CHIOIELEM", changer_name);

	return (0);
}

static int
do_cdlu(cname, argc, argv)
	char *cname;
	int argc;
	char **argv;
{
	struct ioc_load_unload cmd;
	int i;
	static const struct special_word cdlu_subcmds[] = {
		{ "load",	CD_LU_LOAD },
		{ "unload",	CD_LU_UNLOAD },
		{ "abort",	CD_LU_ABORT },
		{ NULL,		0 },
	};

	/*
	 * This command is a little different, since we are mostly dealing
	 * with ATAPI CD changers, which have a lame API (since ATAPI doesn't
	 * have LUNs).
	 *
	 * We have 3 sub-commands: "load", "unload", and "abort".  The
	 * first two take a slot number.  The latter does not.
	 */

	if (argc < 1 || argc > 2)
		usage();

	for (i = 0; cdlu_subcmds[i].sw_name != NULL; i++) {
		if (strcmp(argv[0], cdlu_subcmds[i].sw_name) == 0) {
			cmd.options = cdlu_subcmds[i].sw_value;
			break;
		}
	}
	if (cdlu_subcmds[i].sw_name == NULL)
		usage();

	if (strcmp(argv[0], "abort") == 0)
		cmd.slot = 0;
	else
		cmd.slot = parse_element_unit(argv[1]);

	/*
	 * XXX Should maybe do something different with the device
	 * XXX handling for cdlu; think about this some more.
	 */
	if (ioctl(changer_fd, CDIOCLOADUNLOAD, &cmd))
		err(1, "%s: CDIOCLOADUNLOAD", changer_name);

	return (0);
}

static int
parse_element_type(cp)
	char *cp;
{
	int i;

	for (i = 0; elements[i].et_name != NULL; ++i)
		if (strcmp(elements[i].et_name, cp) == 0)
			return (elements[i].et_type);

	errx(1, "invalid element type `%s'", cp);
	/* NOTREACHED */
}

static int
parse_element_unit(cp)
	char *cp;
{
	int i;
	char *p;

	i = (int)strtol(cp, &p, 10);
	if ((i < 0) || (*p != '\0'))
		errx(1, "invalid unit number `%s'", cp);

	return (i);
}

static int
parse_special(cp)
	char *cp;
{
	int val;

	val = is_special(cp);
	if (val)
		return (val);

	errx(1, "invalid modifier `%s'", cp);
	/* NOTREACHED */
}

static int
is_special(cp)
	char *cp;
{
	int i;

	for (i = 0; specials[i].sw_name != NULL; ++i)
		if (strcmp(specials[i].sw_name, cp) == 0)
			return (specials[i].sw_value);

	return (0);
}

static const char *
bits_to_string(v, cp)
	int v;
	const char *cp;
{
	const char *np;
	char f, *bp;
	int first;
	static char buf[128];

	bp = buf;
	*bp++ = '<';
	for (first = 1; (f = *cp++) != 0; cp = np) {
		for (np = cp; *np >= ' ';)
			np++;
		if ((v & (1 << (f - 1))) == 0)
			continue;
		if (first)
			first = 0;
		else
			*bp++ = ',';
		(void) memcpy(bp, cp, np - cp);
		bp += np - cp;
	}
	*bp++ = '>';
	*bp = '\0';

	return (buf);
}

static void
cleanup()
{
	/* Simple enough... */
	(void)close(changer_fd);
}

static void
usage()
{
	int i;

	(void) fprintf(stderr, "Usage: %s command arg1 arg2 ...\n", __progname);
	
	(void) fprintf(stderr, "Where command (and args) are:\n");
	for (i=0; commands[i].cc_name != NULL; i++)
		(void) fprintf(stderr, "\t%s%s\n", commands[i].cc_name,
					    commands[i].cc_args);
	exit(1);
	/* NOTREACHED */
}
