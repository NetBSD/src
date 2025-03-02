/* $NetBSD: main.c,v 1.4 2025/03/02 00:03:41 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.4 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <sys/efiio.h>
#include <sys/queue.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <util.h>

#include "efiio.h"
#include "defs.h"
#include "bootvar.h"
#include "devpath.h"
#include "getvars.h"
#include "gptsubr.h"
#include "map.h"
#include "setvar.h"
#include "showvar.h"
#include "utils.h"

/*
 * The UEFI spec is quite clear that it is intended for little endian
 * machines only.  As a result (and lazyness), byte ordering is
 * ignored in this code and we build/run only on little endian
 * machines.
 */
__CTASSERT(_BYTE_ORDER == _LITTLE_ENDIAN);

#define _PATH_EFI	"/dev/efi"	/* XXX: should be in <paths.h> */

#define DEFAULT_PARTITION	1
#define DEFAULT_LABEL		"NetBSD"
#define DEFAULT_LOADER		"\\EFI\\NetBSD\\grubx64.efi"
#define DEFAULT_DEVICE		parent_of_fname(".")

/****************************************/

static uint __used
get_max_namelen(efi_var_t **var_array, size_t var_cnt)
{
	uint max_len = 0;

	for (size_t i = 0; i < var_cnt; i++) {
		uint len = (uint)strlen(var_array[i]->name);
		if (len > max_len)
			max_len = len;
	}
	return max_len;
}

/************************************************************************/

enum {
	OPT_BRIEF = UCHAR_MAX + 1,
	OPT_DEBUG = UCHAR_MAX + 2,
};

#define OPTIONS		"@:A::a::B::b:CcDd:FfG::hL:l:Nn:Oo:p:qR:rTt:Vvw::X:x:y"
#define OPTION_LIST \
  _X("brief",		  _NA, OPT_BRIEF, "set brief mode") \
  _X("debug",		  _OA, OPT_DEBUG, "raise or set the debug level") \
  _X("append-binary-args",_RA, '@',	 "Append extra variable args from file") \
  _X("inactive",	  _OA, 'A',	 "clear active bit on '-b' variable") \
  _X("active",		  _OA, 'a',	 "set active bit on '-b' variable") \
  _X("delete-bootnum",	  _OA, 'B',	 "delete '-b' variable or arg") \
  _X("bootnum",		  _RA, 'b',	 "specify a boot number") \
  _X("create-only",	  _NA, 'C',	 "create a new boot variable") \
  _X("create",		  _NA, 'c',	 "create a new boot variable and prefix BootOrder") \
  _X("remove-dups",	  _NA, 'D',	 "remove duplicates in BootOrder") \
  _X("disk",		  _RA, 'd',	 "specify disk device") \
  _X("no-reconnect",	  _NA, 'F',	 "do not force driver reconnect after loading (requires '-r')") \
  _X("reconnect",	  _NA, 'f',	 "force driver reconnect after loading (requires '-r')") \
  _X("show-gpt",	  _OA, 'G',	 "show GPT for device") \
  _X("help",		  _NA, 'h',	 "this help") \
  _X("label",		  _RA, 'L',	 "specify boot label") \
  _X("loader",		  _RA, 'l',	 "specify boot loader on EFI partition") \
  _X("delete-bootnext",	  _NA, 'N',	 "delete NextBoot variable") \
  _X("bootnext",	  _RA, 'n',	 "set NextBoot variable") \
  _X("delete-bootorder",  _NA, 'O',	 "delete BootOrder entirely") \
  _X("bootorder",	  _RA, 'o',	 "set BootOrder") \
  _X("part",		  _RA, 'p',	 "specify partition number") \
  _X("quiet",		  _NA, 'q',	 "quiet mode") \
  _X("regexp",		  _RA, 'R',	 "regular expression for variable search (default: '^Boot')") \
  _X("driver",		  _NA, 'r',	 "operate on Driver#### instead of Boot####") \
  _X("delete-timeout",	  _NA, 'T',	 "delete timeout") \
  _X("timeout",		  _RA, 't',	 "set timeout to argument, in seconds") \
  _X("version",		  _NA, 'V',	 "show Version") \
  _X("verbose",		  _NA, 'v',	 "increment verboseness") \
  _X("write-signature",	  _OA, 'w',	 "write MBR signature") \
  _X("remove-bootorder",  _RA, 'X',	 "remove argument from BootOrder") \
  _X("prefix-bootorder",  _RA, 'x',	 "prefix argument to BootOrder") \
  _X("sysprep",		  _NA, 'y',	 "operate on SysPrep#### instead of Boot####")

#define TARGET_BOOT	"Boot"
#define TARGET_DRIVER	"Driver"
#define TARGET_SYSPREP	"SysPrep"

#define IS_TARGET_DRIVER(opt)	((opt).target[0] == TARGET_DRIVER[0])

#define ACTION_LIST \
  _X(active,			NULL) \
  _X(create,			NULL) \
  _X(delete,			NULL) \
  _X(del_bootnext,		del_bootnext) \
  _X(set_bootnext,		set_bootnext) \
  _X(del_bootorder,		del_bootorder) \
  _X(set_bootorder,		set_bootorder) \
  _X(prefix_bootorder,		prefix_bootorder) \
  _X(remove_bootorder,		remove_bootorder) \
  _X(del_bootorder_dups,	del_bootorder_dups) \
  _X(set_timeout,		set_timeout) \
  _X(show,			NULL) \
  _X(show_gpt,			show_gpt)

static void __dead __printflike(1, 2)
usage(const char *fmt, ...)
{
	const char *progname = getprogname();
	struct {
		const char *help;
		const char *name;
		int opt;
	} tbl[] = {
#define _NA	""
#define _OA	"[=arg]"
#define _RA	"=<arg>"
#define _X(n,a,o,m) { .name = n a, .opt = o, .help = m, },
		OPTION_LIST
#undef _X
#undef _RA
#undef _OA
#undef _NA
	};

	if (fmt != NULL) {
		va_list ap;

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}

	printf("%s version %u\n", progname, VERSION);
	printf("Usage: %s [options]\n", progname);
	for (size_t i = 0; i < __arraycount(tbl); i++) {
		int n;
		if (isprint(tbl[i].opt))
			n = printf("-%c | --%s", tbl[i].opt, tbl[i].name);
		else
			n = printf("     --%s", tbl[i].name);
		printf("%*s %s\n", 32 - n, "", tbl[i].help);
	}
	exit(EXIT_SUCCESS);
}

static int __used
append_optional_data(const char *fname, efi_var_ioc_t *ev)
{
	char *buf, *cp;
	size_t cnt;

	buf = read_file(fname, &cnt);

	ev->data = erealloc(ev->data, ev->datasize + cnt);
	cp = ev->data;
	cp += ev->datasize;
	memcpy(cp, buf, cnt);
	ev->datasize += cnt;

	return 0;
}

typedef enum {
	MBR_SIG_WRITE_NEVER = 0,
	MBR_SIG_WRITE_MAYBE,
	MBR_SIG_WRITE_FORCE,
} mbr_sig_write_t;

#define OPT_LIST \
  _X(bool,		active,			  false ) \
  _X(bool,		b_flag,		  false ) \
  _X(bool,		brief,		  false ) \
  _X(bool,		prefix_bootorder, false ) \
  _X(bool,		quiet,		  false ) \
  _X(bool,		reconnect,	  false ) \
  _X(char *,		bootorder,	  NULL ) \
  _X(char *,		csus,		  NULL ) \
  _X(char *,		device,		  NULL ) \
  _X(char *,		opt_fname,	  NULL ) \
  _X(char *,		regexp,		  NULL ) \
  _X(const char *,	label,		  DEFAULT_LABEL ) \
  _X(const char *,	loader,		  DEFAULT_LOADER ) \
  _X(const char *,	target,		  NULL ) \
  _X(int,		verbose,	  0 ) \
  _X(uint,		debug,		  0 ) \
  _X(mbr_sig_write_t,	mbr_sig_write,    MBR_SIG_WRITE_NEVER ) \
  _X(uint16_t,		bootnum,	  0 ) \
  _X(uint16_t,		partnum,	  DEFAULT_PARTITION ) \
  _X(uint16_t,		timeout,	  0 ) \
  _X(uint32_t,		mbr_sig,	  0 )

#define IS_MBR_SIG_FORCE(o)	((o).mbr_sig_write == MBR_SIG_WRITE_FORCE)

static struct options {	/* setable options */
#define _X(t,n,v)	t n;
	OPT_LIST
#undef _X
} opt = {
#define _X(t,n,v)	.n = v,
	OPT_LIST
#undef _X
};

static inline void
get_bootnum(struct options *op, const char *oarg)
{

	op->b_flag = true;
	op->bootnum = strtous(oarg, NULL, 16);
}

int
main(int argc, char **argv)
{
	static struct option longopts[] = {
#define _NA	no_argument
#define _OA	optional_argument
#define _RA	required_argument
#define _X(n,a,o,m) { n, a, NULL, o },
		OPTION_LIST
		{ NULL, 0, NULL, 0 },
#undef _X
#undef _RA
#undef _OA
#undef _NA
	};
	enum {
		act_create,
		act_set_active,
		act_del_variable,
		act_del_bootnext,
		act_set_bootnext,
		act_del_bootorder,
		act_set_bootorder,
		act_prefix_bootorder,
		act_remove_bootorder,
		act_del_bootorder_dups,
		act_set_timeout,
		act_del_timeout,
		act_show,
		act_show_gpt,
	} action = act_show;
	efi_var_t **var_array;
	void *var_hdl;
	char *fname = NULL;
	size_t i, var_cnt;
	int ch, efi_fd;

	union {	/* Just in case the above __CTASSERT() is ignored ... */
		uint32_t val;
		uint8_t b[4];
	} byte_order = { .val = 0x01020304, };
	if (byte_order.b[0] != 4 ||
	    byte_order.b[1] != 3 ||
	    byte_order.b[2] != 2 ||
	    byte_order.b[3] != 1) {
		errx(EXIT_FAILURE, "sorry: %s only runs on little-endian machines!",
		    getprogname());
	}

	setprogname(argv[0]);

	optreset = 1;
	optind = 1;
	opterr = 1;
	while ((ch = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != -1) {
		switch (ch) {
		case OPT_BRIEF:
			opt.brief = true;
			break;

		case OPT_DEBUG:
			if (optarg)
				opt.debug = strtous(optarg, NULL, 0);
			else
				opt.debug++;
			opt.debug &= DEBUG_MASK;
			break;

		case '@':
			opt.opt_fname = estrdup(optarg);
			break;

		case 'A':
			if (optarg)
				get_bootnum(&opt, optarg);

			opt.active = false;
			action = act_set_active;
			break;

		case 'a':
			if (optarg)
				get_bootnum(&opt, optarg);

			opt.active = true;
			action = act_set_active;
			break;

		case 'B':
			if (optarg)
				get_bootnum(&opt, optarg);

			action = act_del_variable;
			break;

		case 'b':
			get_bootnum(&opt, optarg);
			break;

		case 'C':
			opt.prefix_bootorder = false;
			action = act_create;
			break;

		case 'c':
			opt.prefix_bootorder = true;
			action = act_create;
			break;

		case 'D':
			action = act_del_bootorder_dups;
			break;

		case 'd':
			opt.device = estrdup(optarg);
			break;

		case 'F':
			opt.reconnect = false;
			break;

		case 'f':
			opt.reconnect = true;
			break;

		case 'G':
			fname = estrdup(optarg ? optarg : ".");
			action = act_show_gpt;
			break;

		case 'L':
			opt.label = estrdup(optarg);
			break;

		case 'l':
			opt.loader = estrdup(optarg);
			break;

		case 'N':
			action = act_del_bootnext;
			break;

		case 'n':
			opt.b_flag = true;
			opt.bootnum = strtous(optarg, NULL, 16);
			action = act_set_bootnext;
			break;

		case 'O':
			action = act_del_bootorder;
			break;

		case 'o':
			opt.bootorder = estrdup(optarg);
			action = act_set_bootorder;
			break;

		case 'p':
			opt.partnum = strtous(optarg, NULL, 0);
			break;

		case 'R':
			if (opt.regexp != NULL)
				free(opt.regexp);
			opt.regexp = estrdup(optarg);
			break;

		case 'r':
			if (opt.target != NULL)
				errx(EXIT_FAILURE,
				    "only one of '-r' or '-y' are allowed");
			opt.target = TARGET_DRIVER;
			break;

		case 'T':
			action = act_del_timeout;
			break;

		case 't':
			opt.timeout = strtous(optarg, NULL, 0);
			action = act_set_timeout;
			break;

		case 'q':
			opt.quiet = true;
			opt.verbose = 0;
			break;

		case 'V':
			printf("version: %u\n", VERSION);
			exit(EXIT_SUCCESS);

		case 'v':
			opt.verbose++;
			opt.brief = false;
			break;

		case 'w':
			if (optarg != NULL) {
				opt.mbr_sig_write = MBR_SIG_WRITE_FORCE;
				opt.mbr_sig = (uint32_t)estrtou(optarg, 0, 0, 0xffffffff);
			}
			else {
				opt.mbr_sig_write = MBR_SIG_WRITE_MAYBE;
				srandom((uint)time(NULL));
				opt.mbr_sig = (uint32_t)random();
			}
			break;

		case 'X':
			action = act_remove_bootorder;
			if (opt.csus != NULL) {
				usage("Comma Separated Hex list already specified!\n");
			}
			opt.csus = estrdup(optarg);
			break;

		case 'x':
			action = act_prefix_bootorder;
			if (opt.csus != NULL) {
				usage("Comma Separated Hex list already specified!\n");
			}
			opt.csus = estrdup(optarg);
			break;

		case 'y':
			if (opt.target != NULL)
				errx(EXIT_FAILURE,
				    "only one of '-r' or '-y' are allowed");
			opt.target = TARGET_SYSPREP;
			break;

		case 'h':
			usage(NULL);
		default:
			usage("unknown option: '%c'\n", ch);
		}
	}
	if (opt.target == NULL)
		opt.target = TARGET_BOOT;

	argv += optind;
	argc -= optind;

	if (argc != 0)
		usage(NULL);

	/*
	 * Check some option requirements/overrides here.
	 */
	if (opt.quiet)
		opt.verbose = 0;

	switch (action) {
	case act_create:
		if (opt.regexp != NULL) {/* override any previous setting */
			printf("Ignoring specified regexp: '%s'\n",
			    opt.regexp);
			free(opt.regexp);
		}
		break;

	case act_show_gpt:
		return show_gpt(fname, opt.verbose);

	case act_set_active:
	case act_del_variable:
		if (!opt.b_flag)
			usage("please specify a boot number\n");
		/*FALLTHROUGH*/
	default:
		if (opt.mbr_sig_write) {
			/*
			 * This overrides all but act_create and
			 * act_show_gpt.
			 */
			return mbr_sig_write(opt.device, opt.mbr_sig,
			    IS_MBR_SIG_FORCE(opt), opt.verbose);
		}
		break;
	}

	efi_fd = open(_PATH_EFI, O_RDONLY);
	if (efi_fd == -1)
		err(EXIT_FAILURE, "open");

	switch (action) {
	case act_del_bootorder_dups:	return del_bootorder_dups(efi_fd, opt.target);
	case act_del_bootorder:		return del_bootorder(efi_fd, opt.target);
	case act_del_bootnext:		return del_bootnext(efi_fd);
	case act_del_timeout:		return del_timeout(efi_fd);
	case act_del_variable:		return del_variable(efi_fd, opt.target, opt.bootnum);

	case act_set_active:		return set_active(efi_fd, opt.target, opt.bootnum, opt.active);
	case act_set_bootnext:		return set_bootnext(efi_fd, opt.bootnum);
	case act_set_bootorder:		return set_bootorder(efi_fd, opt.target, opt.bootorder);
	case act_set_timeout:		return set_timeout(efi_fd, opt.timeout);

	case act_remove_bootorder:	return remove_bootorder(efi_fd, opt.target, opt.csus, 0);
	case act_prefix_bootorder:	return prefix_bootorder(efi_fd, opt.target, opt.csus, 0);

	case act_show_gpt:		assert(0); break; /* handled above */
	default:			break;
	}

	/*
	 * The following actions are handled below and require a call
	 * to get_variables() using a regexp.  Setup the regexp here.
	 * XXX: merge with above switch()?
	 */
	switch (action) {
	case act_create:
		easprintf(&opt.regexp, "^%s[0-9,A-F]{4}$", opt.target);
		break;

	case act_show:
	default:
		if (opt.regexp != NULL)
			break;

		if (opt.b_flag)
			easprintf(&opt.regexp, "^%s%04X$", opt.target, opt.bootnum);
		else
			easprintf(&opt.regexp, "^%s", opt.target);
		break;
	}

	var_hdl = get_variables(efi_fd, opt.regexp, &var_array, &var_cnt);

	free(opt.regexp);
	opt.regexp = NULL;

	/*
	 * preform the remaining actions.
	 */
	switch (action) {
	case act_create: {
		uint16_t bootnum;
		efi_var_t v;
		uint32_t attrib;
		int rv;

		if (opt.device == NULL) {
			opt.device = DEFAULT_DEVICE;
			if (opt.device == NULL)
				errx(EXIT_FAILURE, "specify disk with '-d'");
		}
		attrib = LOAD_OPTION_ACTIVE;
		if (opt.reconnect && IS_TARGET_DRIVER(opt))
			attrib |= LOAD_OPTION_FORCE_RECONNECT;

		/*
		 * Get a new variable name
		 */
		bootnum = (uint16_t)find_new_bootvar(var_array, var_cnt, opt.target);
		easprintf(&v.name, "%s%04X", opt.target, bootnum);

		if (!opt.quiet)
			printf("creating: %s\n", v.name);

		/*
		 * Initialize efi_ioc structure.
		 */
		efi_var_init(&v.ev, v.name,
		    &EFI_GLOBAL_VARIABLE,
		    EFI_VARIABLE_NON_VOLATILE |
		    EFI_VARIABLE_BOOTSERVICE_ACCESS |
		    EFI_VARIABLE_RUNTIME_ACCESS);

		/*
		 * Setup the efi_ioc data section
		 */
		v.ev.data = make_bootvar_data(opt.device, opt.partnum,
		    attrib, opt.label, opt.loader, opt.opt_fname, &v.ev.datasize);
#if 1
		if (!opt.quiet) {
			/*
			 * Prompt user for confirmation.
			 * XXX: Should this go away?
			 */
			opt.debug &= (uint)~DEBUG_BRIEF_BIT;
			opt.debug |= DEBUG_VERBOSE_BIT;
			show_variable(&v, opt.debug, 0);

			printf("are you sure? [y/n] ");
			if (getchar() != 'y')
				goto done;
		}
#endif
		/*
		 * Write the variable.
		 */
		rv = set_variable(efi_fd, &v.ev);
		if (rv == -1)
			err(EXIT_FAILURE, "set_variable");

		/*
		 * Prefix the boot order if required.
		 */
		if (opt.prefix_bootorder)
			rv = prefix_bootorder(efi_fd, opt.target, NULL,
			    bootnum);

		/*
		 * Possibly write the MBR signature.
		 * XXX: do we really want this here?
		 */
		if (opt.mbr_sig_write) {
			assert(opt.device != NULL);
			mbr_sig_write(opt.device, opt.mbr_sig,
			    IS_MBR_SIG_FORCE(opt), opt.verbose);
		}
		break;
	}

	case act_show: {
		uint max_namelen = get_max_namelen(var_array, var_cnt);
		uint flags = opt.debug;

		if (opt.verbose)
			flags |= DEBUG_VERBOSE_BIT;

		if (opt.brief)
			flags |= DEBUG_BRIEF_BIT;

		if (max_namelen > 32)
			max_namelen = 32;

		for (i = 0; i < var_cnt; i++) {
			if (opt.brief)
				show_generic_data(var_array[i], max_namelen);
			else
				show_variable(var_array[i], flags, 0);
		}
		break;
	}

	default:
		assert(0);
		break;
	}

 done:
	free_variables(var_hdl);
	close(efi_fd);

	return 0;
}
