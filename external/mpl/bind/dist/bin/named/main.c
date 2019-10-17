/*	$NetBSD: main.c,v 1.4.4.2 2019/10/17 19:34:14 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <isc/app.h>
#include <isc/backtrace.h>
#include <isc/commandline.h>
#include <isc/dir.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/httpd.h>
#include <isc/os.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/resource.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <isccc/result.h>

#include <dns/dispatch.h>
#include <dns/dyndb.h>
#include <dns/name.h>
#include <dns/result.h>
#include <dns/resolver.h>
#include <dns/view.h>

#include <dst/result.h>
#if USE_PKCS11
#include <pk11/result.h>
#endif

#include <dlz/dlz_dlopen_driver.h>

#ifdef HAVE_GPERFTOOLS_PROFILER
#include <gperftools/profiler.h>
#endif


/*
 * Defining NAMED_MAIN provides storage declarations (rather than extern)
 * for variables in named/globals.h.
 */
#define NAMED_MAIN 1

#include <ns/interfacemgr.h>

#include <named/builtin.h>
#include <named/control.h>
#include <named/fuzz.h>
#include <named/globals.h>	/* Explicit, though named/log.h includes it. */
#include <named/log.h>
#include <named/os.h>
#include <named/server.h>
#include <named/main.h>
#ifdef HAVE_LIBSCF
#include <named/smf_globals.h>
#endif

#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#ifdef HAVE_LIBXML2
#include <libxml/xmlversion.h>
#endif

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#include <ns/pfilter.h>
/*
 * Include header files for database drivers here.
 */
/* #include "xxdb.h" */

#ifdef CONTRIB_DLZ
/*
 * Include contributed DLZ drivers if appropriate.
 */
#include <dlz/dlz_drivers.h>
#endif

/*
 * The maximum number of stack frames to dump on assertion failure.
 */
#ifndef BACKTRACE_MAXFRAME
#define BACKTRACE_MAXFRAME 128
#endif

LIBISC_EXTERNAL_DATA extern int isc_dscp_check_value;
LIBDNS_EXTERNAL_DATA extern unsigned int dns_zone_mkey_hour;
LIBDNS_EXTERNAL_DATA extern unsigned int dns_zone_mkey_day;
LIBDNS_EXTERNAL_DATA extern unsigned int dns_zone_mkey_month;

static bool	want_stats = false;
static char		program_name[NAME_MAX] = "named";
static char		absolute_conffile[PATH_MAX];
static char		saved_command_line[512];
static char		version[512];
static unsigned int	maxsocks = 0;
static int		maxudp = 0;

/*
 * -T options:
 */
static bool clienttest = false;
static bool dropedns = false;
static bool ednsformerr = false;
static bool ednsnotimp = false;
static bool ednsrefused = false;
static bool fixedlocal = false;
static bool noaa = false;
static bool noedns = false;
static bool nonearest = false;
static bool nosoa = false;
static bool notcp = false;
static bool sigvalinsecs = false;
static unsigned int delay = 0;

/*
 * -4 and -6
 */
static bool disable6 = false;
static bool disable4 = false;

void
named_main_earlywarning(const char *format, ...) {
	va_list args;

	va_start(args, format);
	if (named_g_lctx != NULL) {
		isc_log_vwrite(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			       NAMED_LOGMODULE_MAIN, ISC_LOG_WARNING,
			       format, args);
	} else {
		fprintf(stderr, "%s: ", program_name);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	va_end(args);
}

void
named_main_earlyfatal(const char *format, ...) {
	va_list args;

	va_start(args, format);
	if (named_g_lctx != NULL) {
		isc_log_vwrite(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			       NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			       format, args);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			       NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			       "exiting (due to early fatal error)");
	} else {
		fprintf(stderr, "%s: ", program_name);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	va_end(args);

	exit(1);
}

ISC_PLATFORM_NORETURN_PRE static void
assertion_failed(const char *file, int line, isc_assertiontype_t type,
		 const char *cond) ISC_PLATFORM_NORETURN_POST;

static void
assertion_failed(const char *file, int line, isc_assertiontype_t type,
		 const char *cond)
{
	void *tracebuf[BACKTRACE_MAXFRAME];
	int i, nframes;
	isc_result_t result;
	const char *logsuffix = "";
	const char *fname;

	/*
	 * Handle assertion failures.
	 */

	if (named_g_lctx != NULL) {
		/*
		 * Reset the assertion callback in case it is the log
		 * routines causing the assertion.
		 */
		isc_assertion_setcallback(NULL);

		result = isc_backtrace_gettrace(tracebuf, BACKTRACE_MAXFRAME,
						&nframes);
		if (result == ISC_R_SUCCESS && nframes > 0)
			logsuffix = ", back trace";
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			      "%s:%d: %s(%s) failed%s", file, line,
			      isc_assertion_typetotext(type), cond, logsuffix);
		if (result == ISC_R_SUCCESS) {
			for (i = 0; i < nframes; i++) {
				unsigned long offset;

				fname = NULL;
				result = isc_backtrace_getsymbol(tracebuf[i],
								 &fname,
								 &offset);
				if (result == ISC_R_SUCCESS) {
					isc_log_write(named_g_lctx,
						      NAMED_LOGCATEGORY_GENERAL,
						      NAMED_LOGMODULE_MAIN,
						      ISC_LOG_CRITICAL,
						      "#%d %p in %s()+0x%lx", i,
						      tracebuf[i], fname,
						      offset);
				} else {
					isc_log_write(named_g_lctx,
						      NAMED_LOGCATEGORY_GENERAL,
						      NAMED_LOGMODULE_MAIN,
						      ISC_LOG_CRITICAL,
						      "#%d %p in ??", i,
						      tracebuf[i]);
				}
			}
		}
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			      "exiting (due to assertion failure)");
	} else {
		fprintf(stderr, "%s:%d: %s(%s) failed\n",
			file, line, isc_assertion_typetotext(type), cond);
		fflush(stderr);
	}

	if (named_g_coreok)
		abort();
	exit(1);
}

ISC_PLATFORM_NORETURN_PRE static void
library_fatal_error(const char *file, int line, const char *format,
		    va_list args)
ISC_FORMAT_PRINTF(3, 0) ISC_PLATFORM_NORETURN_POST;

static void
library_fatal_error(const char *file, int line, const char *format,
		    va_list args)
{
	/*
	 * Handle isc_error_fatal() calls from our libraries.
	 */

	if (named_g_lctx != NULL) {
		/*
		 * Reset the error callback in case it is the log
		 * routines causing the assertion.
		 */
		isc_error_setfatal(NULL);

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			      "%s:%d: fatal error:", file, line);
		isc_log_vwrite(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			       NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			       format, args);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_CRITICAL,
			      "exiting (due to fatal error in library)");
	} else {
		fprintf(stderr, "%s:%d: fatal error: ", file, line);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
		fflush(stderr);
	}

	if (named_g_coreok)
		abort();
	exit(1);
}

static void
library_unexpected_error(const char *file, int line, const char *format,
			 va_list args) ISC_FORMAT_PRINTF(3, 0);

static void
library_unexpected_error(const char *file, int line, const char *format,
			 va_list args)
{
	/*
	 * Handle isc_error_unexpected() calls from our libraries.
	 */

	if (named_g_lctx != NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_ERROR,
			      "%s:%d: unexpected error:", file, line);
		isc_log_vwrite(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			       NAMED_LOGMODULE_MAIN, ISC_LOG_ERROR,
			       format, args);
	} else {
		fprintf(stderr, "%s:%d: fatal error: ", file, line);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
}

static void
usage(void) {
	fprintf(stderr,
		"usage: named [-4|-6] [-c conffile] [-d debuglevel] "
		"[-E engine] [-f|-g]\n"
		"             [-n number_of_cpus] [-p port] [-s] "
		"[-S sockets] [-t chrootdir]\n"
		"             [-u username] [-U listeners] "
		"[-m {usage|trace|record|size|mctx}]\n"
		"usage: named [-v|-V]\n");
}

static void
save_command_line(int argc, char *argv[]) {
	int i;
	char *src;
	char *dst;
	char *eob;
	const char truncated[] = "...";
	bool quoted = false;

	dst = saved_command_line;
	eob = saved_command_line + sizeof(saved_command_line);

	for (i = 1; i < argc && dst < eob; i++) {
		*dst++ = ' ';

		src = argv[i];
		while (*src != '\0' && dst < eob) {
			/*
			 * This won't perfectly produce a shell-independent
			 * pastable command line in all circumstances, but
			 * comes close, and for practical purposes will
			 * nearly always be fine.
			 */
			if (quoted || isalnum(*src & 0xff) ||
			    *src == ',' || *src == '-' || *src == '_' ||
			    *src == '.' || *src == '/') {
				*dst++ = *src++;
				quoted = false;
			} else {
				*dst++ = '\\';
				quoted = true;
			}
		}
	}

	INSIST(sizeof(saved_command_line) >= sizeof(truncated));

	if (dst == eob)
		strcpy(eob - sizeof(truncated), truncated);
	else
		*dst = '\0';
}

static int
parse_int(char *arg, const char *desc) {
	char *endp;
	int tmp;
	long int ltmp;

	ltmp = strtol(arg, &endp, 10);
	tmp = (int) ltmp;
	if (*endp != '\0')
		named_main_earlyfatal("%s '%s' must be numeric", desc, arg);
	if (tmp < 0 || tmp != ltmp)
		named_main_earlyfatal("%s '%s' out of range", desc, arg);
	return (tmp);
}

static struct flag_def {
	const char *name;
	unsigned int value;
	bool negate;
} mem_debug_flags[] = {
	{ "none", 0, false },
	{ "trace",  ISC_MEM_DEBUGTRACE, false },
	{ "record", ISC_MEM_DEBUGRECORD, false },
	{ "usage", ISC_MEM_DEBUGUSAGE, false },
	{ "size", ISC_MEM_DEBUGSIZE, false },
	{ "mctx", ISC_MEM_DEBUGCTX, false },
	{ NULL, 0, false }
}, mem_context_flags[] = {
	{ "external", ISC_MEMFLAG_INTERNAL, true },
	{ "fill", ISC_MEMFLAG_FILL, false },
	{ "nofill", ISC_MEMFLAG_FILL, true },
	{ NULL, 0, false }
};

static void
set_flags(const char *arg, struct flag_def *defs, unsigned int *ret) {
	bool clear = false;

	for (;;) {
		const struct flag_def *def;
		const char *end = strchr(arg, ',');
		int arglen;
		if (end == NULL)
			end = arg + strlen(arg);
		arglen = (int)(end - arg);
		for (def = defs; def->name != NULL; def++) {
			if (arglen == (int)strlen(def->name) &&
			    memcmp(arg, def->name, arglen) == 0) {
				if (def->value == 0)
					clear = true;
				if (def->negate)
					*ret &= ~(def->value);
				else
					*ret |= def->value;
				goto found;
			}
		}
		named_main_earlyfatal("unrecognized flag '%.*s'", arglen, arg);
	 found:
		if (clear || (*end == '\0'))
			break;
		arg = end + 1;
	}

	if (clear)
		*ret = 0;
}

static void
printversion(bool verbose) {
	char rndcconf[PATH_MAX], *dot = NULL;

	printf("%s %s%s%s <id:%s>\n",
	       named_g_product, named_g_version,
	       (*named_g_description != '\0') ? " " : "",
	       named_g_description, named_g_srcid);

	if (!verbose) {
		return;
	}

	printf("running on %s\n", named_os_uname());
	printf("built by %s with %s\n",
	       named_g_builder, named_g_configargs);
#ifdef __clang__
	printf("compiled by CLANG %s\n", __VERSION__);
#else
#if defined(__ICC) || defined(__INTEL_COMPILER)
	printf("compiled by ICC %s\n", __VERSION__);
#else
#ifdef __GNUC__
	printf("compiled by GCC %s\n", __VERSION__);
#endif
#endif
#endif
#ifdef _MSC_VER
	printf("compiled by MSVC %d\n", _MSC_VER);
#endif
#ifdef __SUNPRO_C
	printf("compiled by Solaris Studio %x\n", __SUNPRO_C);
#endif
	printf("compiled with OpenSSL version: %s\n",
	       OPENSSL_VERSION_TEXT);
#if !defined(LIBRESSL_VERSION_NUMBER) && \
OPENSSL_VERSION_NUMBER >= 0x10100000L /* 1.1.0 or higher */
	printf("linked to OpenSSL version: %s\n",
	       OpenSSL_version(OPENSSL_VERSION));

#else
	printf("linked to OpenSSL version: %s\n",
	       SSLeay_version(SSLEAY_VERSION));
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
#ifdef HAVE_LIBXML2
	printf("compiled with libxml2 version: %s\n",
	       LIBXML_DOTTED_VERSION);
	printf("linked to libxml2 version: %s\n",
	       xmlParserVersion);
#endif
#if defined(HAVE_JSON) && defined(JSON_C_VERSION)
	printf("compiled with libjson-c version: %s\n",
	       JSON_C_VERSION);
	printf("linked to libjson-c version: %s\n",
	       json_c_version());
#endif
#if defined(HAVE_ZLIB) && defined(ZLIB_VERSION)
	printf("compiled with zlib version: %s\n",
	       ZLIB_VERSION);
	printf("linked to zlib version: %s\n",
	       zlibVersion());
#endif
	printf("threads support is enabled\n\n");


	/*
	 * The default rndc.conf and rndc.key paths are in the same
	 * directory, but named only has rndc.key defined internally.
	 * We construct the rndc.conf path from it. (We could use
	 * NAMED_SYSCONFDIR here but the result would look wrong on
	 * Windows.)
	 */
	strlcpy(rndcconf, named_g_keyfile, sizeof(rndcconf));
	dot = strrchr(rndcconf, '.');
	if (dot != NULL) {
		size_t len = dot - rndcconf + 1;
		snprintf(dot + 1, PATH_MAX - len, "conf");
	}

	/*
	 * Print default configuration paths.
	 */
	printf("default paths:\n");
	printf("  named configuration:  %s\n", named_g_conffile);
	printf("  rndc configuration:   %s\n", rndcconf);
	printf("  DNSSEC root key:      %s\n", named_g_defaultbindkeys);
	printf("  nsupdate session key: %s\n", named_g_defaultsessionkeyfile);
	printf("  named PID file:       %s\n", named_g_defaultpidfile);
	printf("  named lock file:      %s\n", named_g_defaultlockfile);

}

static void
parse_fuzz_arg(void) {
	if (!strncmp(isc_commandline_argument, "client:", 7)) {
		named_g_fuzz_addr = isc_commandline_argument + 7;
		named_g_fuzz_type = isc_fuzz_client;
	} else if (!strncmp(isc_commandline_argument, "tcp:", 4)) {
		named_g_fuzz_addr = isc_commandline_argument + 4;
		named_g_fuzz_type = isc_fuzz_tcpclient;
	} else if (!strncmp(isc_commandline_argument, "resolver:", 9)) {
		named_g_fuzz_addr = isc_commandline_argument + 9;
		named_g_fuzz_type = isc_fuzz_resolver;
	} else if (!strncmp(isc_commandline_argument, "http:", 5)) {
		named_g_fuzz_addr = isc_commandline_argument + 5;
		named_g_fuzz_type = isc_fuzz_http;
	} else if (!strncmp(isc_commandline_argument, "rndc:", 5)) {
		named_g_fuzz_addr = isc_commandline_argument + 5;
		named_g_fuzz_type = isc_fuzz_rndc;
	} else {
		named_main_earlyfatal("unknown fuzzing type '%s'",
				   isc_commandline_argument);
	}
}

static void
parse_T_opt(char *option) {
	const char *p;
	char *last = NULL;
	/*
	 * force the server to behave (or misbehave) in
	 * specified ways for testing purposes.
	 *
	 * clienttest: make clients single shot with their
	 * 	       own memory context.
	 * delay=xxxx: delay client responses by xxxx ms to
	 *	       simulate remote servers.
	 * dscp=x:     check that dscp values are as
	 * 	       expected and assert otherwise.
	 */
	if (!strcmp(option, "clienttest")) {
		clienttest = true;
	} else if (!strncmp(option, "delay=", 6)) {
		delay = atoi(option + 6);
	} else if (!strcmp(option, "dropedns")) {
		dropedns = true;
	} else if (!strncmp(option, "dscp=", 5)) {
		isc_dscp_check_value = atoi(option + 5);
	} else if (!strcmp(option, "ednsformerr")) {
		ednsformerr = true;
	} else if (!strcmp(option, "ednsnotimp")) {
		ednsnotimp = true;
	} else if (!strcmp(option, "ednsrefused")) {
		ednsrefused = true;
	} else if (!strcmp(option, "fixedlocal")) {
		fixedlocal = true;
	} else if (!strcmp(option, "keepstderr")) {
		named_g_keepstderr = true;
	} else if (!strcmp(option, "noaa")) {
		noaa = true;
	} else if (!strcmp(option, "noedns")) {
		noedns = true;
	} else if (!strcmp(option, "nonearest")) {
		nonearest = true;
	} else if (!strcmp(option, "nosoa")) {
		nosoa = true;
	} else if (!strcmp(option, "nosyslog")) {
		named_g_nosyslog = true;
	} else if (!strcmp(option, "notcp")) {
		notcp = true;
	} else if (!strcmp(option, "maxudp512")) {
		maxudp = 512;
	} else if (!strcmp(option, "maxudp1460")) {
		maxudp = 1460;
	} else if (!strncmp(option, "maxudp=", 7)) {
		maxudp = atoi(option + 7);
		if (maxudp <= 0) {
			named_main_earlyfatal("bad maxudp");
		}
	} else if (!strncmp(option, "mkeytimers=", 11)) {
		p = strtok_r(option + 11, "/", &last);
		if (p == NULL) {
			named_main_earlyfatal("bad mkeytimer");
		}

		dns_zone_mkey_hour = atoi(p);
		if (dns_zone_mkey_hour == 0) {
			named_main_earlyfatal("bad mkeytimer");
		}

		p = strtok_r(NULL, "/", &last);
		if (p == NULL) {
			dns_zone_mkey_day = (24 * dns_zone_mkey_hour);
			dns_zone_mkey_month = (30 * dns_zone_mkey_day);
			return;
		}

		dns_zone_mkey_day = atoi(p);
		if (dns_zone_mkey_day < dns_zone_mkey_hour)
			named_main_earlyfatal("bad mkeytimer");

		p = strtok_r(NULL, "/", &last);
		if (p == NULL) {
			dns_zone_mkey_month = (30 * dns_zone_mkey_day);
			return;
		}

		dns_zone_mkey_month = atoi(p);
		if (dns_zone_mkey_month < dns_zone_mkey_day) {
			named_main_earlyfatal("bad mkeytimer");
		}
	} else if (!strcmp(option, "sigvalinsecs")) {
		sigvalinsecs = true;
	} else if (!strncmp(option, "tat=", 4)) {
		named_g_tat_interval = atoi(option + 4);
	} else {
		fprintf(stderr, "unknown -T flag '%s'\n", option);
	}
}

static void
parse_command_line(int argc, char *argv[]) {
	int ch;
	int port;
	const char *p;

	save_command_line(argc, argv);

	/*
	 * NAMED_MAIN_ARGS is defined in main.h, so that it can be used
	 * both by named and by ntservice hooks.
	 */
	isc_commandline_errprint = false;
	while ((ch = isc_commandline_parse(argc, argv,
					   NAMED_MAIN_ARGS)) != -1)
	{
		switch (ch) {
		case '4':
			if (disable4)
				named_main_earlyfatal("cannot specify "
						      "-4 and -6");
			if (isc_net_probeipv4() != ISC_R_SUCCESS)
				named_main_earlyfatal("IPv4 not supported "
						      "by OS");
			isc_net_disableipv6();
			disable6 = true;
			break;
		case '6':
			if (disable6)
				named_main_earlyfatal("cannot specify "
						      "-4 and -6");
			if (isc_net_probeipv6() != ISC_R_SUCCESS)
				named_main_earlyfatal("IPv6 not supported "
						      "by OS");
			isc_net_disableipv4();
			disable4 = true;
			break;
		case 'A':
			parse_fuzz_arg();
			break;
		case 'c':
			named_g_conffile = isc_commandline_argument;
			named_g_conffileset = true;
			break;
		case 'd':
			named_g_debuglevel = parse_int(isc_commandline_argument,
						       "debug level");
			break;
		case 'D':
			/* Descriptive comment for 'ps'. */
			break;
		case 'E':
			named_g_engine = isc_commandline_argument;
			break;
		case 'f':
			named_g_foreground = true;
			break;
		case 'g':
			named_g_foreground = true;
			named_g_logstderr = true;
			break;
		case 'L':
			named_g_logfile = isc_commandline_argument;
			break;
		case 'M':
			set_flags(isc_commandline_argument, mem_context_flags,
				  &isc_mem_defaultflags);
			break;
		case 'm':
			set_flags(isc_commandline_argument, mem_debug_flags,
				  &isc_mem_debugging);
			break;
		case 'N': /* Deprecated. */
		case 'n':
			named_g_cpus = parse_int(isc_commandline_argument,
					      "number of cpus");
			if (named_g_cpus == 0)
				named_g_cpus = 1;
			break;
		case 'p':
			port = parse_int(isc_commandline_argument, "port");
			if (port < 1 || port > 65535)
				named_main_earlyfatal("port '%s' out of range",
						   isc_commandline_argument);
			named_g_port = port;
			break;
		case 's':
			/* XXXRTH temporary syntax */
			want_stats = true;
			break;
		case 'S':
			maxsocks = parse_int(isc_commandline_argument,
					     "max number of sockets");
			break;
		case 't':
			/* XXXJAB should we make a copy? */
			named_g_chrootdir = isc_commandline_argument;
			break;
		case 'T':	/* NOT DOCUMENTED */
			parse_T_opt(isc_commandline_argument);
			break;
		case 'U':
			named_g_udpdisp = parse_int(isc_commandline_argument,
						    "number of UDP listeners "
						    "per interface");
			break;
		case 'u':
			named_g_username = isc_commandline_argument;
			break;
		case 'v':
			printversion(false);
			exit(0);
		case 'V':
			printversion(true);
			exit(0);
		case 'x':
			/* Obsolete. No longer in use. Ignore. */
			break;
		case 'X':
			named_g_forcelock = true;
			if (strcasecmp(isc_commandline_argument, "none") != 0)
				named_g_defaultlockfile =
					isc_commandline_argument;
			else
				named_g_defaultlockfile = NULL;
			break;
		case 'F':
			/* Reserved for FIPS mode */
			/* FALLTHROUGH */
		case '?':
			usage();
			if (isc_commandline_option == '?')
				exit(0);
			p = strchr(NAMED_MAIN_ARGS, isc_commandline_option);
			if (p == NULL || *++p != ':')
				named_main_earlyfatal("unknown option '-%c'",
						   isc_commandline_option);
			else
				named_main_earlyfatal("option '-%c' requires "
						   "an argument",
						   isc_commandline_option);
			/* FALLTHROUGH */
		default:
			named_main_earlyfatal("parsing options returned %d",
					      ch);
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;
	POST(argv);

	if (argc > 0) {
		usage();
		named_main_earlyfatal("extra command line arguments");
	}
}

static isc_result_t
create_managers(void) {
	isc_result_t result;
	unsigned int socks;

	INSIST(named_g_cpus_detected > 0);

	if (named_g_cpus == 0)
		named_g_cpus = named_g_cpus_detected;
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "found %u CPU%s, using %u worker thread%s",
		      named_g_cpus_detected,
		      named_g_cpus_detected == 1 ? "" : "s",
		      named_g_cpus, named_g_cpus == 1 ? "" : "s");
#ifdef WIN32
	named_g_udpdisp = 1;
#else
	if (named_g_udpdisp == 0) {
		named_g_udpdisp = named_g_cpus_detected;
	}
	if (named_g_udpdisp > named_g_cpus)
		named_g_udpdisp = named_g_cpus;
#endif
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "using %u UDP listener%s per interface",
		      named_g_udpdisp, named_g_udpdisp == 1 ? "" : "s");

	result = isc_taskmgr_create(named_g_mctx, named_g_cpus, 0,
				    &named_g_taskmgr);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_taskmgr_create() failed: %s",
				 isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}

	result = isc_timermgr_create(named_g_mctx, &named_g_timermgr);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_timermgr_create() failed: %s",
				 isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}

	result = isc_socketmgr_create2(named_g_mctx, &named_g_socketmgr,
				       maxsocks, named_g_cpus);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_socketmgr_create() failed: %s",
				 isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	isc_socketmgr_maxudp(named_g_socketmgr, maxudp);
	result = isc_socketmgr_getmaxsockets(named_g_socketmgr, &socks);
	if (result == ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER,
			      ISC_LOG_INFO, "using up to %u sockets", socks);
	}

	return (ISC_R_SUCCESS);
}

static void
destroy_managers(void) {
	/*
	 * isc_taskmgr_destroy() will block until all tasks have exited,
	 */
	isc_taskmgr_destroy(&named_g_taskmgr);
	isc_timermgr_destroy(&named_g_timermgr);
	isc_socketmgr_destroy(&named_g_socketmgr);
}

static void
dump_symboltable(void) {
	int i;
	isc_result_t result;
	const char *fname;
	const void *addr;

	if (isc__backtrace_nsymbols == 0)
		return;

	if (!isc_log_wouldlog(named_g_lctx, ISC_LOG_DEBUG(99)))
		return;

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_DEBUG(99),
		      "Symbol table:");

	for (i = 0, result = ISC_R_SUCCESS; result == ISC_R_SUCCESS; i++) {
		addr = NULL;
		fname = NULL;
		result = isc_backtrace_getsymbolfromindex(i, &addr, &fname);
		if (result == ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_MAIN, ISC_LOG_DEBUG(99),
				      "[%d] %p %s", i, addr, fname);
		}
	}
}

static void
setup(void) {
	isc_result_t result;
	isc_resourcevalue_t old_openfiles;
	ns_server_t *sctx;
#ifdef HAVE_LIBSCF
	char *instance = NULL;
#endif

	/*
	 * Get the user and group information before changing the root
	 * directory, so the administrator does not need to keep a copy
	 * of the user and group databases in the chroot'ed environment.
	 */
	named_os_inituserinfo(named_g_username);

	/*
	 * Initialize time conversion information
	 */
	named_os_tzset();

	named_os_opendevnull();

#ifdef HAVE_LIBSCF
	/* Check if named is under smf control, before chroot. */
	result = named_smf_get_instance(&instance, 0, named_g_mctx);
	/* We don't care about instance, just check if we got one. */
	if (result == ISC_R_SUCCESS)
		named_smf_got_instance = 1;
	else
		named_smf_got_instance = 0;
	if (instance != NULL)
		isc_mem_free(named_g_mctx, instance);
#endif /* HAVE_LIBSCF */

	/*
	 * Check for the number of cpu's before named_os_chroot().
	 */
	named_g_cpus_detected = isc_os_ncpus();

	named_os_chroot(named_g_chrootdir);

	/*
	 * For operating systems which have a capability mechanism, now
	 * is the time to switch to minimal privs and change our user id.
	 * On traditional UNIX systems, this call will be a no-op, and we
	 * will change the user ID after reading the config file the first
	 * time.  (We need to read the config file to know which possibly
	 * privileged ports to bind() to.)
	 */
	named_os_minprivs();

	result = named_log_init(named_g_username != NULL);
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("named_log_init() failed: %s",
				   isc_result_totext(result));

	/*
	 * Now is the time to daemonize (if we're not running in the
	 * foreground).  We waited until now because we wanted to get
	 * a valid logging context setup.  We cannot daemonize any later,
	 * because calling create_managers() will create threads, which
	 * would be lost after fork().
	 */
	if (!named_g_foreground)
		named_os_daemonize();

	/*
	 * We call isc_app_start() here as some versions of FreeBSD's fork()
	 * destroys all the signal handling it sets up.
	 */
	result = isc_app_start();
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("isc_app_start() failed: %s",
				   isc_result_totext(result));

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "starting %s %s%s%s <id:%s>",
		      named_g_product, named_g_version,
		      *named_g_description ? " " : "", named_g_description,
		      named_g_srcid);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "running on %s", named_os_uname());

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "built with %s", named_g_configargs);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "running as: %s%s",
		      program_name, saved_command_line);
#ifdef __clang__
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled by CLANG %s", __VERSION__);
#else
#if defined(__ICC) || defined(__INTEL_COMPILER)
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled by ICC %s", __VERSION__);
#else
#ifdef __GNUC__
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled by GCC %s", __VERSION__);
#endif
#endif
#endif
#ifdef _MSC_VER
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled by MSVC %d", _MSC_VER);
#endif
#ifdef __SUNPRO_C
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled by Solaris Studio %x", __SUNPRO_C);
#endif
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled with OpenSSL version: %s",
		      OPENSSL_VERSION_TEXT);
#if !defined(LIBRESSL_VERSION_NUMBER) && \
    OPENSSL_VERSION_NUMBER >= 0x10100000L /* 1.1.0 or higher */
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "linked to OpenSSL version: %s",
		      OpenSSL_version(OPENSSL_VERSION));
#else
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "linked to OpenSSL version: %s",
		      SSLeay_version(SSLEAY_VERSION));
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
#ifdef HAVE_LIBXML2
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled with libxml2 version: %s",
		      LIBXML_DOTTED_VERSION);
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "linked to libxml2 version: %s", xmlParserVersion);
#endif
#if defined(HAVE_JSON) && defined(JSON_C_VERSION)
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled with libjson-c version: %s", JSON_C_VERSION);
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "linked to libjson-c version: %s", json_c_version());
#endif
#if defined(HAVE_ZLIB) && defined(ZLIB_VERSION)
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "compiled with zlib version: %s", ZLIB_VERSION);
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "linked to zlib version: %s", zlibVersion());
#endif
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "----------------------------------------------------");
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "BIND 9 is maintained by Internet Systems Consortium,");
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "Inc. (ISC), a non-profit 501(c)(3) public-benefit ");
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "corporation.  Support and training for BIND 9 are ");
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "available at https://www.isc.org/support");
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
		      "----------------------------------------------------");

	dump_symboltable();

	/*
	 * Get the initial resource limits.
	 */
#ifndef WIN32
	RUNTIME_CHECK(isc_resource_getlimit(isc_resource_stacksize,
					    &named_g_initstacksize)
		      == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_resource_getlimit(isc_resource_datasize,
					    &named_g_initdatasize)
		      == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_resource_getlimit(isc_resource_coresize,
					    &named_g_initcoresize)
		      == ISC_R_SUCCESS);
#endif
	RUNTIME_CHECK(isc_resource_getlimit(isc_resource_openfiles,
					    &named_g_initopenfiles)
		      == ISC_R_SUCCESS);

	/*
	 * System resources cannot effectively be tuned on some systems.
	 * Raise the limit in such cases for safety.
	 */
	old_openfiles = named_g_initopenfiles;
	named_os_adjustnofile();
	RUNTIME_CHECK(isc_resource_getlimit(isc_resource_openfiles,
					    &named_g_initopenfiles)
		      == ISC_R_SUCCESS);
	if (old_openfiles != named_g_initopenfiles) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_MAIN, ISC_LOG_NOTICE,
			      "adjusted limit on open files from "
			      "%" PRIu64 " to "
			      "%" PRIu64,
			      old_openfiles, named_g_initopenfiles);
	}

	/*
	 * If the named configuration filename is relative, prepend the current
	 * directory's name before possibly changing to another directory.
	 */
	if (! isc_file_isabsolute(named_g_conffile)) {
		result = isc_file_absolutepath(named_g_conffile,
					       absolute_conffile,
					       sizeof(absolute_conffile));
		if (result != ISC_R_SUCCESS)
			named_main_earlyfatal("could not construct "
					      "absolute path "
					      "of configuration file: %s",
					      isc_result_totext(result));
		named_g_conffile = absolute_conffile;
	}

	/*
	 * Record the server's startup time.
	 */
	result = isc_time_now(&named_g_boottime);
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("isc_time_now() failed: %s",
				   isc_result_totext(result));

	result = create_managers();
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("create_managers() failed: %s",
				   isc_result_totext(result));

	named_builtin_init();

	/*
	 * Add calls to register sdb drivers here.
	 */
	/* xxdb_init(); */

#ifdef ISC_DLZ_DLOPEN
	/*
	 * Register the DLZ "dlopen" driver.
	 */
	result = dlz_dlopen_init(named_g_mctx);
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("dlz_dlopen_init() failed: %s",
				   isc_result_totext(result));
#endif

#if CONTRIB_DLZ
	/*
	 * Register any other contributed DLZ drivers.
	 */
	result = dlz_drivers_init();
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("dlz_drivers_init() failed: %s",
				   isc_result_totext(result));
#endif

	named_server_create(named_g_mctx, &named_g_server);
	sctx = named_g_server->sctx;

	/*
	 * Modify server context according to command line options
	 */
	if (clienttest)
		ns_server_setoption(sctx, NS_SERVER_CLIENTTEST, true);
	if (disable4)
		ns_server_setoption(sctx, NS_SERVER_DISABLE4, true);
	if (disable6)
		ns_server_setoption(sctx, NS_SERVER_DISABLE6, true);
	if (dropedns)
		ns_server_setoption(sctx, NS_SERVER_DROPEDNS, true);
	if (ednsformerr)	/* STD13 server */
		ns_server_setoption(sctx, NS_SERVER_EDNSFORMERR, true);
	if (ednsnotimp)
		ns_server_setoption(sctx, NS_SERVER_EDNSNOTIMP, true);
	if (ednsrefused)
		ns_server_setoption(sctx, NS_SERVER_EDNSREFUSED, true);
	if (fixedlocal)
		ns_server_setoption(sctx, NS_SERVER_FIXEDLOCAL, true);
	if (noaa)
		ns_server_setoption(sctx, NS_SERVER_NOAA, true);
	if (noedns)
		ns_server_setoption(sctx, NS_SERVER_NOEDNS, true);
	if (nonearest)
		ns_server_setoption(sctx, NS_SERVER_NONEAREST, true);
	if (nosoa)
		ns_server_setoption(sctx, NS_SERVER_NOSOA, true);
	if (notcp)
		ns_server_setoption(sctx, NS_SERVER_NOTCP, true);
	if (sigvalinsecs)
		ns_server_setoption(sctx, NS_SERVER_SIGVALINSECS, true);

	named_g_server->sctx->delay = delay;
}

static void
cleanup(void) {
	destroy_managers();

	if (named_g_mapped != NULL)
		dns_acl_detach(&named_g_mapped);

	named_server_destroy(&named_g_server);

	named_builtin_deinit();

	/*
	 * Add calls to unregister sdb drivers here.
	 */
	/* xxdb_clear(); */

#ifdef CONTRIB_DLZ
	/*
	 * Unregister contributed DLZ drivers.
	 */
	dlz_drivers_clear();
#endif
#ifdef ISC_DLZ_DLOPEN
	/*
	 * Unregister "dlopen" DLZ driver.
	 */
	dlz_dlopen_clear();
#endif

	dns_name_destroy();

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_MAIN,
		      ISC_LOG_NOTICE, "exiting");
	named_log_shutdown();
}

static char *memstats = NULL;

void
named_main_setmemstats(const char *filename) {
	/*
	 * Caller has to ensure locking.
	 */

	if (memstats != NULL) {
		free(memstats);
		memstats = NULL;
	}

	if (filename == NULL)
		return;

	memstats = strdup(filename);
}

#ifdef HAVE_LIBSCF
/*
 * Get FMRI for the named process.
 */
isc_result_t
named_smf_get_instance(char **ins_name, int debug, isc_mem_t *mctx) {
	scf_handle_t *h = NULL;
	int namelen;
	char *instance;

	REQUIRE(ins_name != NULL && *ins_name == NULL);

	if ((h = scf_handle_create(SCF_VERSION)) == NULL) {
		if (debug)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "scf_handle_create() failed: %s",
					 scf_strerror(scf_error()));
		return (ISC_R_FAILURE);
	}

	if (scf_handle_bind(h) == -1) {
		if (debug)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "scf_handle_bind() failed: %s",
					 scf_strerror(scf_error()));
		scf_handle_destroy(h);
		return (ISC_R_FAILURE);
	}

	if ((namelen = scf_myname(h, NULL, 0)) == -1) {
		if (debug)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "scf_myname() failed: %s",
					 scf_strerror(scf_error()));
		scf_handle_destroy(h);
		return (ISC_R_FAILURE);
	}

	if ((instance = isc_mem_allocate(mctx, namelen + 1)) == NULL) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "named_smf_get_instance memory "
				 "allocation failed: %s",
				 isc_result_totext(ISC_R_NOMEMORY));
		scf_handle_destroy(h);
		return (ISC_R_FAILURE);
	}

	if (scf_myname(h, instance, namelen + 1) == -1) {
		if (debug)
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "scf_myname() failed: %s",
					 scf_strerror(scf_error()));
		scf_handle_destroy(h);
		isc_mem_free(mctx, instance);
		return (ISC_R_FAILURE);
	}

	scf_handle_destroy(h);
	*ins_name = instance;
	return (ISC_R_SUCCESS);
}
#endif /* HAVE_LIBSCF */

/* main entry point, possibly hooked */

int
main(int argc, char *argv[]) {
	isc_result_t result;
#ifdef HAVE_LIBSCF
	char *instance = NULL;
#endif

#ifdef HAVE_GPERFTOOLS_PROFILER
	(void) ProfilerStart(NULL);
#endif

#ifdef WIN32
	/*
	 * Prevent unbuffered I/O from crippling named performance on Windows
	 * when it is logging to stderr (e.g. in system tests).  Use full
	 * buffering (_IOFBF) as line buffering (_IOLBF) is unavailable on
	 * Windows and fflush() is called anyway after each log message gets
	 * written to the default stderr logging channels created by libisc.
	*/
	setvbuf(stderr, NULL, _IOFBF, BUFSIZ);
#endif

	/*
	 * Record version in core image.
	 * strings named.core | grep "named version:"
	 */
	strlcat(version,
#if defined(NO_VERSION_DATE) || !defined(__DATE__)
		"named version: BIND " VERSION " <" SRCID ">",
#else
		"named version: BIND " VERSION " <" SRCID "> (" __DATE__ ")",
#endif
		sizeof(version));
	result = isc_file_progname(*argv, program_name, sizeof(program_name));
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("program name too long");

	isc_assertion_setcallback(assertion_failed);
	isc_error_setfatal(library_fatal_error);
	isc_error_setunexpected(library_unexpected_error);

	named_os_init(program_name);

	dns_result_register();
	dst_result_register();
	isccc_result_register();
#if USE_PKCS11
	pk11_result_register();
#endif

#if !ISC_MEM_DEFAULTFILL
	/*
	 * Update the default flags to remove ISC_MEMFLAG_FILL
	 * before we parse the command line. If disabled here,
	 * it can be turned back on with -M fill.
	 */
	isc_mem_defaultflags &= ~ISC_MEMFLAG_FILL;
#endif

	parse_command_line(argc, argv);

	pfilter_enable();

#ifdef ENABLE_AFL
	if (named_g_fuzz_type != isc_fuzz_none) {
		named_fuzz_setup();
	}

	if (named_g_fuzz_type == isc_fuzz_resolver) {
		dns_resolver_setfuzzing();
	} else if (named_g_fuzz_type == isc_fuzz_http) {
		isc_httpd_setfinishhook(named_fuzz_notify);
	}
#endif
	/*
	 * Warn about common configuration error.
	 */
	if (named_g_chrootdir != NULL) {
		int len = strlen(named_g_chrootdir);
		if (strncmp(named_g_chrootdir, named_g_conffile, len) == 0 &&
		    (named_g_conffile[len] == '/' ||
		     named_g_conffile[len] == '\\'))
			named_main_earlywarning("config filename (-c %s) "
						"contains chroot path (-t %s)",
						named_g_conffile,
						named_g_chrootdir);
	}

	result = isc_mem_create(0, 0, &named_g_mctx);
	if (result != ISC_R_SUCCESS)
		named_main_earlyfatal("isc_mem_create() failed: %s",
				   isc_result_totext(result));
	isc_mem_setname(named_g_mctx, "main", NULL);

	setup();

	/*
	 * Start things running and then wait for a shutdown request
	 * or reload.
	 */
	do {
		result = isc_app_run();

		if (result == ISC_R_RELOAD) {
			named_server_reloadwanted(named_g_server);
		} else if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_run(): %s",
					 isc_result_totext(result));
			/*
			 * Force exit.
			 */
			result = ISC_R_SUCCESS;
		}
	} while (result != ISC_R_SUCCESS);

#ifdef HAVE_LIBSCF
	if (named_smf_want_disable == 1) {
		result = named_smf_get_instance(&instance, 1, named_g_mctx);
		if (result == ISC_R_SUCCESS && instance != NULL) {
			if (smf_disable_instance(instance, 0) != 0)
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "smf_disable_instance() "
						 "failed for %s : %s",
						 instance,
						 scf_strerror(scf_error()));
		}
		if (instance != NULL)
			isc_mem_free(named_g_mctx, instance);
	}
#endif /* HAVE_LIBSCF */

	cleanup();

	if (want_stats) {
		isc_mem_stats(named_g_mctx, stdout);
		isc_mutex_stats(stdout);
	}

	if (named_g_memstatistics && memstats != NULL) {
		FILE *fp = NULL;
		result = isc_stdio_open(memstats, "w", &fp);
		if (result == ISC_R_SUCCESS) {
			isc_mem_stats(named_g_mctx, fp);
			isc_mutex_stats(fp);
			(void) isc_stdio_close(fp);
		}
	}
	isc_mem_destroy(&named_g_mctx);
	isc_mem_checkdestroyed(stderr);

	named_main_setmemstats(NULL);

	isc_app_finish();

	named_os_closedevnull();

	named_os_shutdown();

#ifdef HAVE_GPERFTOOLS_PROFILER
	ProfilerStop();
#endif

	return (0);
}
