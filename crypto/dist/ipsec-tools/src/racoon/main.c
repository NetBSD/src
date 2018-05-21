/*	$NetBSD: main.c,v 1.13.26.1 2018/05/21 04:35:49 pgoyette Exp $	*/

/* Id: main.c,v 1.25 2006/06/20 20:31:34 manubsd Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <paths.h>
#include <err.h>

/*
 * If we're using a debugging malloc library, this may define our
 * wrapper stubs.
 */
#define	RACOON_MAIN_PROGRAM
#include "gcmalloc.h"

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "cfparse_proto.h"
#include "isakmp_var.h"
#include "remoteconf.h"
#include "localconf.h"
#include "session.h"
#include "oakley.h"
#include "pfkey.h"
#include "policy.h"
#include "crypto_openssl.h"
#include "backupsa.h"
#include "vendorid.h"

#include "package_version.h"

int dump_config = 0;	/* dump parsed config file. */
int f_local = 0;	/* local test mode.  behave like a wall. */
int vflag = 1;		/* for print-isakmp.c */
static int loading_sa = 0;	/* install sa when racoon boots up. */

#ifdef TOP_PACKAGE
static char version[] = "@(#)" TOP_PACKAGE_STRING " (" TOP_PACKAGE_URL ")";
#else
static char version[] = "@(#) racoon / IPsec-tools";
#endif

static void
print_version(void)
{
	printf("%s\n"
	       "\n"
	       "Compiled with:\n"
	       "- %s (http://www.openssl.org/)\n"
#ifdef INET6
	       "- IPv6 support\n"
#endif
#ifdef ENABLE_DPD
	       "- Dead Peer Detection\n"
#endif
#ifdef ENABLE_FRAG
	       "- IKE fragmentation\n"
#endif
#ifdef ENABLE_HYBRID
	       "- Hybrid authentication\n"
#endif
#ifdef ENABLE_GSSAPI
	       "- GSS-API authentication\n"
#endif
#ifdef ENABLE_NATT
	       "- NAT Traversal\n"
#endif
#ifdef ENABLE_STATS
	       "- Timing statistics\n"
#endif
#ifdef ENABLE_ADMINPORT
	       "- Admin port\n"
#endif
#ifdef HAVE_CLOCK_MONOTONIC
	       "- Monotonic clock\n"
#endif
#ifdef HAVE_SECCTX
	       "- Security context\n"
#endif
	       "\n",
	       version,
	       eay_version());
	exit(0);
}

static void
usage(void)
{
	printf("usage: racoon [-BdFv"
#ifdef INET6
		"46"
#endif
		"] [-f (file)] [-l (file)] [-p (port)] [-P (natt port)]\n"
		"   -B: install SA to the kernel from the file "
		"specified by the configuration file.\n"
		"   -d: debug level, more -d will generate more debug message.\n"
		"   -C: dump parsed config file.\n"
		"   -L: include location in debug messages\n"
		"   -F: run in foreground, do not become daemon.\n"
		"   -v: be more verbose\n"
		"   -V: print version and exit\n"
#ifdef INET6
		"   -4: IPv4 mode.\n"
		"   -6: IPv6 mode.\n"
#endif
		"   -f: pathname for configuration file.\n"
		"   -l: pathname for log file.\n"
		"   -p: port number for isakmp (default: %d).\n"
		"   -P: port number for NAT-T (default: %d).\n"
		"\n",
		PORT_ISAKMP, PORT_ISAKMP_NATT);
	exit(1);
}

static void
parse(int ac, char **av)
{
	extern char *optarg;
	extern int optind;
	int c;
#ifdef YYDEBUG
	extern int yydebug;
#endif

	pname = strrchr(*av, '/');
	if (pname)
		pname++;
	else
		pname = *av;

	while ((c = getopt(ac, av, "dLFp:P:f:l:vVZBC"
#ifdef YYDEBUG
			"y"
#endif
#ifdef INET6
			"46"
#endif
			)) != -1) {
		switch (c) {
		case 'd':
			loglevel++;
			break;
		case 'L':
			print_location = 1;
			break;
		case 'F':
			printf("Foreground mode.\n");
			f_foreground = 1;
			break;
		case 'p':
			lcconf->port_isakmp = atoi(optarg);
			break;
		case 'P':
			lcconf->port_isakmp_natt = atoi(optarg);
			break;
		case 'f':
			lcconf->racoon_conf = optarg;
			break;
		case 'l':
			plogset(optarg);
			break;
		case 'v':
			vflag++;
			break;
		case 'V':
			print_version();
			break;
		case 'Z':
			/*
			 * only local test.
			 * To specify -Z option and to choice a appropriate
			 * port number for ISAKMP, you can launch some racoons
			 * on the local host for debug.
			 * pk_sendadd() on initiator side is always failed
			 * even if this flag is used.  Because there is same
			 * spi in the SAD which is inserted by pk_sendgetspi()
			 * on responder side.
			 */
			printf("Local test mode.\n");
			f_local = 1;
			break;
#ifdef YYDEBUG
		case 'y':
			yydebug = 1;
			break;
#endif
#ifdef INET6
		case '4':
			lcconf->default_af = AF_INET;
			break;
		case '6':
			lcconf->default_af = AF_INET6;
			break;
#endif
		case 'B':
			loading_sa++;
			break;
		case 'C':
			dump_config++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	ac -= optind;
	av += optind;

	if (ac != 0) {
		usage();
		/* NOTREACHED */
	}
}

int
main(int ac, char **av)
{
	initlcconf();
	parse(ac, av);

	if (geteuid() != 0) {
		errx(1, "must be root to invoke this program.");
		/* NOTREACHED*/
	}

	/*
	 * Don't let anyone read files I write.  Although some files (such as
	 * the PID file) can be other readable, we dare to use the global mask,
	 * because racoon uses fopen(3), which can't specify the permission
	 * at the creation time.
	 */
	umask(077);
	if (umask(077) != 077) {
		errx(1, "could not set umask");
		/* NOTREACHED*/
	}

	ploginit();

#ifdef DEBUG_RECORD_MALLOCATION
	DRM_init();
#endif

#ifdef HAVE_SECCTX
	init_avc();
#endif
	eay_init();
	initrmconf();
	oakley_dhinit();
	compute_vendorids();

	plog(LLV_INFO, LOCATION, NULL, "%s\n", version);
	plog(LLV_INFO, LOCATION, NULL, "@(#)"
	    "This product linked %s (http://www.openssl.org/)"
	    "\n", eay_version());
	plog(LLV_INFO, LOCATION, NULL, "Reading configuration from \"%s\"\n",
	    lcconf->racoon_conf);

	/*
	 * install SAs from the specified file.  If the file is not specified
	 * by the configuration file, racoon will exit.
	 */
	if (loading_sa && !f_local) {
		if (backupsa_from_file() != 0)
			errx(1, "something error happened "
				"SA recovering.");
	}

	if (f_foreground)
		close(0);
	else {
		if (daemon(0, 0) < 0) {
			errx(1, "failed to be daemon. (%s)",
				strerror(errno));
		}
#ifndef __linux__
		/*
		 * In case somebody has started inetd manually, we need to
		 * clear the logname, so that old servers run as root do not
		 * get the user's logname..
		 */
		if (setlogin("") < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"cannot clear logname: %s\n", strerror(errno));
			/* no big deal if it fails.. */
		}
#endif
	}

	session();

	return 0;
}
