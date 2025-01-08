/*	$NetBSD: pathnames.h,v 1.6 2025/01/08 19:59:39 christos Exp $	*/

/*
 * pathnames.h - define default paths for pppd
 *
 * Copyright (c) 1999-2024 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Paths are controlled by configure. Typically, one would pass the following options
 * to configure:
 *    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
 *
 * Note that if the prefix variable do control all the other directory variables, e.g
 * sysconfdir is ${prefix}/etc. Setting prefix to /usr, you'll have to override
 * sysconfdir with /etc to avoid installing config files into /usr/etc.
 *
 * In addition, there are three explicit variables that has overrides via configure:
 * - PPPD_RUNTIME_DIR, set by --with-runtime-dir=<dir>
 * - PPPD_LOGFILE_DIR, set by --with-logfile-dir=<dir>
 * - PPPD_PLUGIN_DIR, set by --with-plugin-dir=<dir>
 */
#ifndef PPP_PATHNAMES_H
#define PPP_PATHNAMES_H

#include "pppdconf.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif /* HAVE_PATHS_H */

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL           "/dev/null"
#endif

#define PPP_DEVNULL             _PATH_DEVNULL

/*
 * PPPD_RUNTIME_DIR is set by passing --with-runtime-dir=<path> via configure
 *    the default value set by configure when option is absent is ${localstatedir}/run/pppd
 */
#ifdef PPPD_RUNTIME_DIR
#define PPP_PATH_VARRUN         PPPD_RUNTIME_DIR
#else
#define PPP_PATH_VARRUN         _PATH_VARRUN
#endif

/*
 * PPPD_LOGFILE_DIR is set by passing --with-logfile-dir=<path> via configure
 *    the default value set by configure when option is absent is ${localstatedir}/log/ppp
 */
#ifdef PPPD_LOGFILE_DIR
#define PPP_PATH_VARLOG         PPPD_LOGFILE_DIR
#else
#define PPP_PATH_VARLOG         _PATH_VARRUN "/log/ppp"
#endif

/*
 * PPPD_PLUGIN_DIR is set by passing --with-plugin-dir=<path> via configure
 *    the default value set by configure when option is absent is ${libdir}/pppd/${version}
 */
#ifdef PPPD_PLUGIN_DIR
#define PPP_PATH_PLUGIN         PPPD_PLUGIN_DIR
#endif /* PPPD_PLUGIN_DIR */

#define PPP_PATH_CONFDIR        SYSCONFDIR "/ppp"

#define PPP_PATH_UPAPFILE       PPP_PATH_CONFDIR "/pap-secrets"
#define PPP_PATH_CHAPFILE       PPP_PATH_CONFDIR "/chap-secrets"
#define PPP_PATH_SRPFILE        PPP_PATH_CONFDIR "/srp-secrets"

#ifdef PPP_WITH_EAPTLS
#define PPP_PATH_EAPTLSCLIFILE  PPP_PATH_CONFDIR "/eaptls-client"
#define PPP_PATH_EAPTLSSERVFILE PPP_PATH_CONFDIR "/eaptls-server"
#define PPP_PATH_OPENSSLCONFFILE PPP_PATH_CONFDIR "/openssl.cnf"
#endif /* PPP_WITH_EAPTLS */

#define PPP_PATH_SYSOPTIONS     PPP_PATH_CONFDIR "/options"
#define PPP_PATH_IPUP           PPP_PATH_CONFDIR "/ip-up"
#define PPP_PATH_IPDOWN         PPP_PATH_CONFDIR "/ip-down"
#define PPP_PATH_IPPREUP        PPP_PATH_CONFDIR "/ip-pre-up"
#define PPP_PATH_AUTHUP         PPP_PATH_CONFDIR "/auth-up"
#define PPP_PATH_AUTHDOWN       PPP_PATH_CONFDIR "/auth-down"
#define PPP_PATH_TTYOPT         PPP_PATH_CONFDIR "/options."
#define PPP_PATH_PEERFILES      PPP_PATH_CONFDIR "/peers/"
#define PPP_PATH_RESOLV         PPP_PATH_CONFDIR "/resolv.conf"

#define PPP_PATH_NET_INIT	PPP_PATH_CONFDIR "/net-init"
#define PPP_PATH_NET_PREUP	PPP_PATH_CONFDIR "/net-pre-up"
#define PPP_PATH_NET_DOWN	PPP_PATH_CONFDIR "/net-down"

#define PPP_PATH_CONNERRS       PPP_PATH_VARLOG  "/connect-errors"

#define PPP_PATH_USEROPT        ".ppprc"
#define PPP_PATH_PSEUDONYM	    ".ppp_pseudonym"

#ifdef PPP_WITH_IPV6CP
#define PPP_PATH_IPV6UP         PPP_PATH_CONFDIR "/ipv6-up"
#define PPP_PATH_IPV6DOWN       PPP_PATH_CONFDIR "/ipv6-down"
#endif

#define PPP_PATH_PPPDB          PPP_PATH_VARRUN  "/pppd2.tdb"

#ifdef __linux__
#define PPP_PATH_LOCKDIR        "/var/lock"
#else
#ifdef SVR4
#define PPP_PATH_LOCKDIR        "/var/spool/locks"
#else
#define PPP_PATH_LOCKDIR        "/var/spool/lock"
#endif
#endif

#endif /* PPP_PATHNAMES_H */
