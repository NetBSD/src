/* $NetBSD: chio.c,v 1.21 2003/08/21 04:30:25 jschauma Exp $ */

/*-
 * Copyright (c) 1996, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Additional Copyright (c) 1997, by Matthew Jacob, for NASA/Ames Research Ctr.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1996, 1998, 1999\
	The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: chio.c,v 1.21 2003/08/21 04:30:25 jschauma Exp $");
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/chio.h>
#include <sys/cdio.h>	/* for ATAPI CD changer; too bad it uses a lame API */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "defs.h"
#include "pathnames.h"

int stdout_ok;

int main(int, char *[]);
static void usage(void);
static void cleanup(void);
static int parse_element_type(const char *);
static int parse_element_unit(const char *);
static int parse_special(const char *);
static int is_special(const char *);
static const char *bits_to_string(int, const char *);
char *printescaped(const char *);

static int do_move(const char *, int, char **);
static int do_exchange(const char *, int, char **);
static int do_position(const char *, int, char **);
static int do_params(const char *, int, char **);
static int do_getpicker(const char *, int, char **);
static int do_setpicker(const char *, int, char **);
static int do_status(const char *, int, char **);
static int do_ielem(const char *, int, char **);
static int do_cdlu(const char *, int, char **);

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

	{ "status",	" [<ET> [unit [count]]] [voltags]",
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
	{ "voltags",		SW_VOLTAGS },
	{ NULL,			0 },
};

static const char *changer_name;
static int changer_fd;

int
main(int argc, char *argv[])
{
	int ch, i;

	setprogname(argv[0]);
	while ((ch = getopt(argc, argv, "f:")) != -1) {
		switch (ch) {
		case 'f':
			changer_name = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();
		/* NOTREACHED */

	stdout_ok = isatty(STDOUT_FILENO);

	/* Get the default changer if not already specified. */
	if (changer_name == NULL)
		if ((changer_name = getenv(CHANGER_ENV_VAR)) == NULL)
			changer_name = _PATH_CH;

	/* Open the changer device. */
	if ((changer_fd = open(changer_name, O_RDWR, 0600)) == -1)
		err(EXIT_FAILURE, "%s: open", printescaped(changer_name));
		/* NOTREACHED */

	/* Register cleanup function. */
	if (atexit(cleanup))
		err(EXIT_FAILURE, "can't register cleanup function");
		/* NOTREACHED */

	/* Find the specified command. */
	for (i = 0; commands[i].cc_name != NULL; ++i)
		if (strcmp(*argv, commands[i].cc_name) == 0)
			break;
	if (commands[i].cc_name == NULL)
		errx(EXIT_FAILURE, "unknown command: %s", *argv);
		/* NOTREACHED */

	/* Skip over the command name and call handler. */
	++argv; --argc;
	exit((*commands[i].cc_handler)(commands[i].cc_name, argc, argv));
	/* NOTREACHED */
}

static int
do_move(const char *cname, int argc, char **argv)
{
	struct changer_move_request cmd;
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
		/*NOTREACHED*/
	} else if (argc > 5) {
		warnx("%s: too many arguments", cname);
		usage();
		/*NOTREACHED*/
	}
	(void)memset(&cmd, 0, sizeof(cmd));

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
			errx(EXIT_FAILURE, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOMOVE, &cmd))
		err(EXIT_FAILURE, "%s: CHIOMOVE", printescaped(changer_name));
		/* NOTREACHED */

	return (0);
}

static int
do_exchange(const char *cname, int argc, char **argv)
{
	struct changer_exchange_request cmd;
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
		/*NOTREACHED*/
	} else if (argc > 8) {
		warnx("%s: too many arguments", cname);
		usage();
		/*NOTREACHED*/
	}
	(void)memset(&cmd, 0, sizeof(cmd));

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
			errx(EXIT_FAILURE, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOEXCHANGE, &cmd))
		err(EXIT_FAILURE, "%s: CHIOEXCHANGE", printescaped(changer_name));
		/* NOTREACHED */

	return (0);
}

static int
do_position(const char *cname, int argc, char **argv)
{
	struct changer_position_request cmd;
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
		/*NOTREACHED*/
	} else if (argc > 3) {
		warnx("%s: too many arguments", cname);
		usage();
		/*NOTREACHED*/
	}
	(void)memset(&cmd, 0, sizeof(cmd));

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
			errx(EXIT_FAILURE, "%s: inappropriate modifier `%s'",
			    cname, *argv);
			/* NOTREACHED */
		}
	}

	/* Send command to changer. */
	if (ioctl(changer_fd, CHIOPOSITION, &cmd))
		err(EXIT_FAILURE, "%s: CHIOPOSITION", printescaped(changer_name));
		/* NOTREACHED */

	return (0);
}

/* ARGSUSED */
static int
do_params(const char *cname, int argc, char **argv)
{
	struct changer_params data;
	char *cn;

	/* No arguments to this command. */
	if (argc) {
		warnx("%s: no arguements expected", cname);
		usage();
		/* NOTREACHED */
	}
	
	cn = printescaped(changer_name);

	/* Get params from changer and display them. */
	(void)memset(&data, 0, sizeof(data));
	if (ioctl(changer_fd, CHIOGPARAMS, &data))
		err(EXIT_FAILURE, "%s: CHIOGPARAMS", cn);
		/* NOTREACHED */

#define	PLURAL(n)	(n) > 1 ? "s" : ""

	(void)printf("%s: %d slot%s, %d drive%s, %d picker%s",
	    cn,
	    data.cp_nslots, PLURAL(data.cp_nslots),
	    data.cp_ndrives, PLURAL(data.cp_ndrives),
	    data.cp_npickers, PLURAL(data.cp_npickers));
	if (data.cp_nportals)
		(void)printf(", %d portal%s", data.cp_nportals,
		    PLURAL(data.cp_nportals));

#undef PLURAL

	(void)printf("\n%s: current picker: %d\n", cn, data.cp_curpicker);

	free(cn);

	return (0);
}

/* ARGSUSED */
static int
do_getpicker(const char *cname, int argc, char **argv)
{
	int picker;
	char *cn;

	/* No arguments to this command. */
	if (argc) {
		warnx("%s: no arguments expected", cname);
		usage();
		/*NOTREACHED*/
	}

	cn = printescaped(changer_name);

	/* Get current picker from changer and display it. */
	if (ioctl(changer_fd, CHIOGPICKER, &picker))
		err(EXIT_FAILURE, "%s: CHIOGPICKER", cn);
		/* NOTREACHED */

	(void)printf("%s: current picker: %d\n", cn, picker);
	free(cn);

	return (0);
}

static int
do_setpicker(const char *cname, int argc, char **argv)
{
	int picker;

	if (argc < 1) {
		warnx("%s: too few arguments", cname);
		usage();
		/*NOTREACHED*/
	} else if (argc > 1) {
		warnx("%s: too many arguments", cname);
		usage();
		/*NOTREACHED*/
	}

	picker = parse_element_unit(*argv);

	/* Set the changer picker. */
	if (ioctl(changer_fd, CHIOSPICKER, &picker))
		err(EXIT_FAILURE, "%s: CHIOSPICKER", printescaped(changer_name));

	return (0);
}

static int
do_status(const char *cname, int argc, char **argv)
{
	struct changer_element_status_request cmd;
	struct changer_params data;
	struct changer_element_status *ces;
	int i, chet, count, echet, flags, have_ucount, have_unit;
	int schet, ucount, unit;
	size_t size;
	char *cn;

	flags = 0;
	have_ucount = 0;
	have_unit = 0;

	/*
	 * On a status command, we expect the following:
	 *
	 * [<ET> [unit [count]]] [voltags]
	 *
	 * where ET == element type.
	 *
	 * If we get no element-related arguments, we get the status of all
	 * known element types.
	 */
	if (argc > 4) {
		warnx("%s: too many arguments", cname);
		usage();
		/*NOTREACHED*/
	}

	cn = printescaped(changer_name);

	/*
	 * Get params from changer.  Specifically, we need the element
	 * counts.
	 */
	(void)memset(&data, 0, sizeof(data));
	if (ioctl(changer_fd, CHIOGPARAMS, &data))
		err(EXIT_FAILURE, "%s: CHIOGPARAMS", cn);
		/* NOTREACHED */

	schet = CHET_MT;
	echet = CHET_DT;

	for (; argc != 0; argc--, argv++) {
		/*
		 * If we have the voltags modifier, it must be the
		 * last argument.
		 */
		if (is_special(argv[0])) {
			if (argc != 1) {
				warnx("%s: malformed command line", cname);
				usage();
				/*NOTREACHED*/
			}
			if (parse_special(argv[0]) != SW_VOLTAGS)
				errx(EXIT_FAILURE,
				    "%s: inappropriate special word: %s",
				    cname, argv[0]);
				/* NOTREACHED */
			flags |= CESR_VOLTAGS;
			continue;
		}

		/*
		 * If we get an element type, we can't have specified
		 * anything else.
		 */
		if (isdigit(*argv[0]) == 0) {
			if (schet == echet || flags != 0 || have_unit ||
			    have_ucount) {
				warnx("%s: malformed command line", cname);
				usage();
				/*NOTREACHED*/
			}
			schet = echet = parse_element_type(argv[0]);
			continue;
		}

		/*
		 * We know we have a digit here.  If we do, we must
		 * have specified an element type.
		 */
		if (schet != echet) {
			warnx("%s: malformed command line", cname);
			usage();
			/*NOTREACHED*/
		}

		i = parse_element_unit(argv[0]);

		if (have_unit == 0) {
			unit = i;
			have_unit = 1;
		} else if (have_ucount == 0) {
			ucount = i;
			have_ucount = 1;
		} else {
			warnx("%s: malformed command line", cname);
			usage();
			/*NOTREACHED*/
		}
	}

	for (chet = schet; chet <= echet; ++chet) {
		switch (chet) {
		case CHET_MT:
			count = data.cp_npickers;
			break;
		case CHET_ST:
			count = data.cp_nslots;
			break;
		case CHET_IE:
			count = data.cp_nportals;
			break;
		case CHET_DT:
			count = data.cp_ndrives;
			break;
		default:
			/* To appease gcc -Wuninitialized. */
			count = 0;
		}

		if (count == 0) {
			if (schet != echet)
				continue;
			else {
				(void)printf("%s: no %s elements\n",
				    cn,
				    elements[chet].et_name);
				free(cn);
				return (0);
			}
		}

		/*
		 * If we have a unit, we may or may not have a count.
		 * If we don't have a unit, we don't have a count, either.
		 *
		 * Make sure both are initialized.
		 */
		if (have_unit) {
			if (have_ucount == 0)
				ucount = 1;
		} else {
			unit = 0;
			ucount = count;
		}

		if ((unit + ucount) > count)
			errx(EXIT_FAILURE, "%s: unvalid unit/count %d/%d",
			    cname, unit, ucount);
			/* NOTREACHED */

		size = ucount * sizeof(struct changer_element_status);

		/* Allocate storage for the status bytes. */
		if ((ces = malloc(size)) == NULL)
			errx(EXIT_FAILURE, "can't allocate status storage");
			/* NOTREACHED */

		(void)memset(ces, 0, size);
		(void)memset(&cmd, 0, sizeof(cmd));

		cmd.cesr_type = chet;
		cmd.cesr_unit = unit;
		cmd.cesr_count = ucount;
		cmd.cesr_flags = flags;
		cmd.cesr_data = ces;

		/*
		 * Should we deal with this eventually?
		 */
		cmd.cesr_vendor_data = NULL;

		if (ioctl(changer_fd, CHIOGSTATUS, &cmd)) {
			free(ces);
			err(EXIT_FAILURE, "%s: CHIOGSTATUS", cn);
			/* NOTREACHED */
		}

		/* Dump the status for each element of this type. */
		for (i = 0; i < ucount; i++) {
			(void)printf("%s %d: ", elements[chet].et_name,
			    unit + i);
			if ((ces[i].ces_flags & CESTATUS_STATUS_VALID) == 0) {
				(void)printf("status not available\n");
				continue;
			}
			(void)printf("%s", bits_to_string(ces[i].ces_flags,
			    CESTATUS_BITS));
			if (ces[i].ces_flags & CESTATUS_XNAME_VALID)
				(void)printf(" (%s)", ces[i].ces_xname);
			(void)printf("\n");
			if (ces[i].ces_flags & CESTATUS_PVOL_VALID)
				(void)printf("\tPrimary volume tag: %s "
				    "ver. %d\n",
				    ces[i].ces_pvoltag.cv_tag,
				    ces[i].ces_pvoltag.cv_serial);
			if (ces[i].ces_flags & CESTATUS_AVOL_VALID)
				(void)printf("\tAlternate volume tag: %s "
				    "ver. %d\n",
				    ces[i].ces_avoltag.cv_tag,
				    ces[i].ces_avoltag.cv_serial);
			if (ces[i].ces_flags & CESTATUS_FROM_VALID)
				(void)printf("\tFrom: %s %d\n",
				    elements[ces[i].ces_from_type].et_name,
				    ces[i].ces_from_unit);
			if (ces[i].ces_vendor_len)
				(void)printf("\tVendor-specific data size: "
				    "%lu\n", (u_long)ces[i].ces_vendor_len);
		}
		free(ces);
	}

	free(cn);
	return (0);
}

/* ARGSUSED */
static int
do_ielem(const char *cname, int argc, char **argv)
{

	if (ioctl(changer_fd, CHIOIELEM, NULL))
		err(EXIT_FAILURE, "%s: CHIOIELEM", printescaped(changer_name));
		/* NOTREACHED */

	return (0);
}

/* ARGSUSED */
static int
do_cdlu(const char *cname, int argc, char **argv)
{
	static const struct special_word cdlu_subcmds[] = {
		{ "load",	CD_LU_LOAD },
		{ "unload",	CD_LU_UNLOAD },
		{ "abort",	CD_LU_ABORT },
		{ NULL,		0 },
	};
	struct ioc_load_unload cmd;
	int i;

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
		/*NOTREACHED*/

	for (i = 0; cdlu_subcmds[i].sw_name != NULL; i++) {
		if (strcmp(argv[0], cdlu_subcmds[i].sw_name) == 0) {
			cmd.options = cdlu_subcmds[i].sw_value;
			break;
		}
	}
	if (cdlu_subcmds[i].sw_name == NULL)
		usage();
		/*NOTREACHED*/

	if (strcmp(argv[0], "abort") == 0)
		cmd.slot = 0;
	else
		cmd.slot = parse_element_unit(argv[1]);

	/*
	 * XXX Should maybe do something different with the device
	 * XXX handling for cdlu; think about this some more.
	 */
	if (ioctl(changer_fd, CDIOCLOADUNLOAD, &cmd))
		err(EXIT_FAILURE, "%s: CDIOCLOADUNLOAD", printescaped(changer_name));
		/* NOTREACHED */

	return (0);
}

static int
parse_element_type(const char *cp)
{
	int i;

	for (i = 0; elements[i].et_name != NULL; ++i)
		if (strcmp(elements[i].et_name, cp) == 0)
			return (elements[i].et_type);

	errx(EXIT_FAILURE, "invalid element type `%s'", cp);
	/* NOTREACHED */
}

static int
parse_element_unit(const char *cp)
{
	char *p;
	int i;

	i = (int)strtol(cp, &p, 10);
	if ((i < 0) || (*p != '\0'))
		errx(EXIT_FAILURE, "invalid unit number `%s'", cp);

	return (i);
}

static int
parse_special(const char *cp)
{
	int val;

	val = is_special(cp);
	if (val)
		return (val);

	errx(EXIT_FAILURE, "invalid modifier `%s'", cp);
	/* NOTREACHED */
}

static int
is_special(const char *cp)
{
	int i;

	for (i = 0; specials[i].sw_name != NULL; ++i)
		if (strcmp(specials[i].sw_name, cp) == 0)
			return (specials[i].sw_value);

	return (0);
}

static const char *
bits_to_string(int v, const char *cp)
{
	static char buf[128];
	const char *np;
	char *bp, f;
	int first;

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
		(void)memcpy(bp, cp, np - cp);
		bp += np - cp;
	}
	*bp++ = '>';
	*bp = '\0';

	return (buf);
}

static void
cleanup(void)
{

	/* Simple enough... */
	(void)close(changer_fd);
}

static void
usage(void)
{
	int i;

	(void)fprintf(stderr, "Usage: %s command arg1 arg2 ...\n",
	    getprogname());

	(void)fprintf(stderr, "Where command (and args) are:\n");
	for (i = 0; commands[i].cc_name != NULL; i++)
		(void)fprintf(stderr, "\t%s%s\n", commands[i].cc_name,
		    commands[i].cc_args);
	exit(1);
	/* NOTREACHED */
}

char *
printescaped(const char *src)
{
	size_t len;
	char *retval;

	len = strlen(src);
	if (len != 0 && SIZE_T_MAX/len <= 4) {
		errx(EXIT_FAILURE, "%s: name too long", src);
		/* NOTREACHED */
	}

	retval = (char *)malloc(4*len+1);
	if (retval != NULL) {
		if (stdout_ok)
			(void)strvis(retval, src, VIS_NL | VIS_CSTYLE);
		else
			(void)strcpy(retval, src);
		return retval;
	} else
		errx(EXIT_FAILURE, "out of memory!");
		/* NOTREACHED */
}
