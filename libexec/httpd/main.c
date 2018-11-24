/*	$NetBSD: main.c,v 1.16.6.1 2018/11/24 17:13:51 martin Exp $	*/

/*	$eterna: main.c,v 1.6 2011/11/18 09:21:15 mrg Exp $	*/
/* from: eterna: bozohttpd.c,v 1.159 2009/05/23 02:14:30 mrg Exp 	*/

/*
 * Copyright (c) 1997-2018 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
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
 *
 */

/* this program is dedicated to the Great God of Processed Cheese */

/*
 * main.c:  C front end to bozohttpd
 */

#include <sys/types.h>
#include <sys/param.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "bozohttpd.h"

/* variables and functions */
#ifndef LOG_FTP
#define LOG_FTP LOG_DAEMON
#endif

/* print a usage message, and then exit */
BOZO_DEAD static void
usage(bozohttpd_t *httpd, char *progname)
{
	bozowarn(httpd, "usage: %s [options] slashdir [virtualhostname]",
			progname);
	bozowarn(httpd, "options:");

	if (have_daemon_mode)
		bozowarn(httpd, "   -b\t\t\tbackground and go into daemon mode");
	if (have_cgibin &&
	    have_dynamic_content)
		bozowarn(httpd, "   -C arg prog\t\tadd this CGI handler");
	if (have_cgibin)
		bozowarn(httpd, "   -c cgibin\t\tenable cgi-bin support in "
				"this directory");
	if (have_debug)
		bozowarn(httpd, "   -d\t\t\tenable debug support");
	if (have_cgibin)
		bozowarn(httpd, "   -E\t\t\tenable CGI support for user dirs");
	if (have_user &&
	    have_cgibin)
		bozowarn(httpd, "   -e\t\t\tdon't clean the environment "
				"(-t and -U only)");
	if (have_daemon_mode)
		bozowarn(httpd, "   -f\t\t\tforeground in daemon mode");
	if (have_all)
		bozowarn(httpd, "   -G print version number and exit");
	if (have_dirindex)
		bozowarn(httpd, "   -H\t\t\thide files starting with a period "
				"(.) in index mode");
	if (have_all)
		bozowarn(httpd, "   -I port\t\tbind or use on this port");
	if (have_daemon_mode)
		bozowarn(httpd, "   -i address\t\tbind on this address "
				"(daemon mode only)");
	if (have_lua)
		bozowarn(httpd, "   -L arg script\tadd this Lua script");
	if (have_dynamic_content)
		bozowarn(httpd, "   -M arg t c c11\tadd this mime extenstion");
	if (have_daemon_mode)
		bozowarn(httpd, "   -P pidfile\t\tpid file path");
	if (have_user)
		bozowarn(httpd, "   -p dir\t\t\"public_html\" directory name");

	if (have_all) {
		bozowarn(httpd, "   -S version\t\tset server version string");
		bozowarn(httpd, "   -s\t\t\talways log to stderr");
		bozowarn(httpd, "   -T type timeout\tset `type' timeout");
		bozowarn(httpd, "   -t dir\t\tchroot to `dir'");
		bozowarn(httpd, "   -U username\t\tchange user to `user'");
	}
	if (have_user)
		bozowarn(httpd, "   -u\t\t\tenable ~user/public_html support");

	if (have_all) {
		bozowarn(httpd, "   -V\t\t\tUnknown virtual hosts go to "
				"`slashdir'");
		bozowarn(httpd, "   -v virtualroot\tenable virtual host "
				"support in this directory");
	}

	if (have_dirindex)
		bozowarn(httpd, "   -X\t\t\tdirectory index support");
	if (have_all)
		bozowarn(httpd, "   -x index\t\tdefault \"index.html\" "
				"file name");

	if (have_ssl) {
		bozowarn(httpd, "   -Z cert privkey\tspecify path to server "
				"certificate and private key file\n"
				"\t\t\tin pem format and enable bozohttpd in "
				"SSL mode");
		bozowarn(httpd, "   -z ciphers\t\tspecify SSL ciphers");
	}

	bozoerr(httpd, 1, "%s failed to start", progname);
}

int
main(int argc, char **argv)
{
	bozo_httpreq_t	*request;
	bozohttpd_t	 httpd;
	bozoprefs_t	 prefs;
	char		*progname;
	const char	*val;
	int		 c;

	(void) memset(&httpd, 0x0, sizeof(httpd));
	(void) memset(&prefs, 0x0, sizeof(prefs));

	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		progname++;

	openlog(progname, LOG_PID|LOG_NDELAY, LOG_FTP);

	bozo_set_defaults(&httpd, &prefs);

	/*
	 * -r option was removed, do not reuse it for a while
	 */

	while ((c = getopt(argc, argv,
	    "C:EGHI:L:M:P:S:T:U:VXZ:bc:defhi:np:st:uv:x:z:")) != -1) {
		switch (c) {

		case 'b':
			if (!have_daemon_mode)
 no_daemon_mode:
				bozoerr(&httpd, 1, "Daemon mode not enabled");

			/*
			 * test suite support - undocumented
			 * background == 2 (aka, -b -b) means to
			 * only process 1 per kid
			 */
			val = bozo_get_pref(&prefs, "background") == NULL ?
			    "1" : "2";
			bozo_set_pref(&httpd, &prefs, "background", val);
			break;

		case 'c':
			if (!have_cgibin)
				bozoerr(&httpd, 1, "CGI not enabled");

			bozo_cgi_setbin(&httpd, optarg);
			break;

		case 'C':
			if (!have_dynamic_content && !have_cgibin)
				bozoerr(&httpd, 1,
				    "dynamic CGI handler support not enabled");

			/* make sure there's two arguments */
			if (argc - optind < 1)
				usage(&httpd, progname);
			bozo_add_content_map_cgi(&httpd, optarg,
					argv[optind++]);
			break;

		case 'd':
			if (!have_debug)
				bozowarn(&httpd, "Debugging not enabled");
			httpd.debug++;
			break;

		case 'E':
			if (have_user &&
			    have_cgibin)
				bozoerr(&httpd, 1, "CGI not enabled");

			bozo_set_pref(&httpd, &prefs, "enable user cgibin",
				      "true");
			break;

		case 'e':
			if (!have_daemon_mode)
				goto no_daemon_mode;

			bozo_set_pref(&httpd, &prefs, "dirty environment",
				      "true");
			break;

		case 'f':
			if (!have_daemon_mode)
				goto no_daemon_mode;

			bozo_set_pref(&httpd, &prefs, "foreground", "true");
			break;

		case 'G':
			{
				char	version[128];

				bozo_get_version(version, sizeof(version));
				printf("bozohttpd version %s\n", version);
			}
			return 0;

		case 'H':
			if (!have_dirindex)
 no_dirindex_support:
				bozoerr(&httpd, 1,
					"directory indexing not enabled");

			bozo_set_pref(&httpd, &prefs, "hide dots", "true");
			break;

		case 'I':
			bozo_set_pref(&httpd, &prefs, "port number", optarg);
			break;

		case 'i':
			if (!have_daemon_mode)
				goto no_daemon_mode;

			bozo_set_pref(&httpd, &prefs, "bind address", optarg);
			break;

		case 'L':
			if (!have_lua)
				bozoerr(&httpd, 1, "Lua support not enabled");

			/* make sure there's two argument */
			if (argc - optind < 1)
				usage(&httpd, progname);
			bozo_add_lua_map(&httpd, optarg, argv[optind]);
			optind++;
			break;

		case 'M':
			if (!have_dynamic_content)
				bozoerr(&httpd, 1,
				    "dynamic mime content support not enabled");

			/* make sure there're four arguments */
			if (argc - optind < 3)
				usage(&httpd, progname);
			bozo_add_content_map_mime(&httpd, optarg, argv[optind],
			    argv[optind+1], argv[optind+2]);
			optind += 3;
			break;

		case 'n':
			bozo_set_pref(&httpd, &prefs, "numeric", "true");
			break;

		case 'P':
			if (!have_daemon_mode)
				goto no_daemon_mode;

			bozo_set_pref(&httpd, &prefs, "pid file", optarg);
			break;

		case 'p':
			if (!have_user)
 no_user_support:
				bozoerr(&httpd, 1, "User support not enabled");

			bozo_set_pref(&httpd, &prefs, "public_html", optarg);
			break;

		case 'S':
			bozo_set_pref(&httpd, &prefs, "server software",
				      optarg);
			break;

		case 's':
			bozo_set_pref(&httpd, &prefs, "log to stderr", "true");
			break;

		case 'T':
			/* make sure there're two arguments */
			if (argc - optind < 1)
				usage(&httpd, progname);
			if (bozo_set_timeout(&httpd, &prefs,
					     optarg, argv[optind])) {
				bozoerr(&httpd, 1,
					"invalid type '%s'", optarg);
				/* NOTREACHED */
			}
			optind++;
			break;

		case 't':
			bozo_set_pref(&httpd, &prefs, "chroot dir", optarg);
			break;

		case 'U':
			bozo_set_pref(&httpd, &prefs, "username", optarg);
			break;

		case 'u':
			if (!have_user)
				goto no_user_support;

			bozo_set_pref(&httpd, &prefs, "enable users", "true");
			break;

			bozo_set_pref(&httpd, &prefs, "directory indexing",
				      "true");
			break;

		case 'V':
			bozo_set_pref(&httpd, &prefs, "unknown slash", "true");
			break;

		case 'v':
			bozo_set_pref(&httpd, &prefs, "virtual base", optarg);
			break;

		case 'X':
			if (!have_dirindex)
				goto no_dirindex_support;

		case 'x':
			bozo_set_pref(&httpd, &prefs, "index.html", optarg);
			break;

		case 'Z':
			if (!have_ssl)
 no_ssl:
				bozoerr(&httpd, 1, "ssl support not enabled");

			/* make sure there's two arguments */
			if (argc - optind < 1)
				usage(&httpd, progname);
			bozo_ssl_set_opts(&httpd, optarg, argv[optind++]);
			break;

		case 'z':
			if (!have_ssl)
				goto no_ssl;

			bozo_ssl_set_ciphers(&httpd, optarg);
			break;

		default:
			usage(&httpd, progname);
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0 || argc > 2) {
		usage(&httpd, progname);
	}

	/* virtual host, and root of tree to serve */
	bozo_setup(&httpd, &prefs, argv[1], argv[0]);

	/*
	 * read and process the HTTP request.
	 */
	do {
		if ((request = bozo_read_request(&httpd)) != NULL) {
			bozo_process_request(request);
			bozo_clean_request(request);
		}
	} while (httpd.background);

	return (0);
}
