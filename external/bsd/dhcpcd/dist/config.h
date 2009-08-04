/*
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CONFIG_H
#define CONFIG_H

#define PACKAGE			"dhcpcd"
#define VERSION			"4.0.14"

/*
 * By default we don't add a local link route if we got a routeable address.
 * This is because dhcpcd can't really decide which interface should allow
 * link local routing when we have more than one interface.
 * Ideally the host network scripts should add the link local route for us.
 * If not, you can define this to get dhcpcd to always add the link local route.
 */
// #define IPV4LL_ALWAYSROUTE 

/* Some systems do not have a working fork. */
/* #define THERE_IS_NO_FORK */

/* Paths to things */
#ifndef SYSCONFDIR
# define SYSCONFDIR		"/etc"
#endif
#ifndef LIBEXECDIR
# define LIBEXECDIR		"/libexec"
#endif
#ifndef RUNDIR
# define RUNDIR			"/var/run"
#endif
#ifndef DBDIR
# define DBDIR			"/var/db"
#endif

#ifndef CONFIG
# define CONFIG			SYSCONFDIR "/" PACKAGE ".conf"
#endif
#ifndef SCRIPT
# define SCRIPT			LIBEXECDIR "/" PACKAGE "-run-hooks"
#endif
#ifndef DUID
# define DUID			SYSCONFDIR "/" PACKAGE ".duid"
#endif
#ifndef LEASEFILE
# define LEASEFILE		DBDIR "/" PACKAGE "-%s.lease"
#endif
#ifndef PIDFILE
# define PIDFILE		RUNDIR "/" PACKAGE "-%s.pid"
#endif

#endif
