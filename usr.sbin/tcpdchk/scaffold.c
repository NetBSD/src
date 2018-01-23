/*	$NetBSD: scaffold.c,v 1.12 2018/01/23 21:06:26 sevan Exp $	*/

 /*
  * Routines for testing only. Not really industrial strength.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccs_id[] = "@(#) scaffold.c 1.6 97/03/21 19:27:24";
#else
__RCSID("$NetBSD: scaffold.c,v 1.12 2018/01/23 21:06:26 sevan Exp $");
#endif
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#ifndef INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

/* Application-specific. */

#include "tcpd.h"
#include "scaffold.h"

 /*
  * These are referenced by the options module and by rfc931.c.
  */
int     allow_severity = SEVERITY;
int     deny_severity = LOG_WARNING;
extern int rfc931_timeout; /* = RFC931_TIMEOUT; */

/* find_inet_addr - find all addresses for this host, result to free() */

struct addrinfo *find_inet_addr(char *host, int flags)
{
    struct addrinfo hints, *res;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_CANONNAME | flags;
    error = getaddrinfo(host, "0", &hints, &res);
    if (error) {
	tcpd_warn("%s: %s", host, gai_strerror(error));
	return (0);
    }

    if (res->ai_canonname && STR_NE(host, res->ai_canonname)) {
	tcpd_warn("%s: hostname alias", host);
	tcpd_warn("(official name: %.*s)", STRING_LENGTH, res->ai_canonname);
    }
    return (res);
}

/* check_dns - give each address thorough workout, return address count */

int check_dns(char *host)
{
    struct request_info request;
    struct sockaddr_storage ss;
    struct addrinfo *res0, *res;
    int     count;

    if ((res0 = find_inet_addr(host, 0)) == NULL)
	return (0);
    memset(&ss, 0, sizeof(ss));
    request_init(&request, RQ_CLIENT_SIN, &ss, 0);
    sock_methods(&request);

    count = 0;
    for (res = res0; res; res = res->ai_next) {
	count++;
	if (res->ai_addrlen > sizeof(ss))
	    continue;
	memcpy(&ss, res->ai_addr, res->ai_addrlen);

	/*
	 * Force host name and address conversions. Use the request structure
	 * as a cache. Detect hostname lookup problems. Any name/name or
	 * name/address conflicts will be reported while eval_hostname() does
	 * its job.
	 */
	request_set(&request, RQ_CLIENT_ADDR, "", RQ_CLIENT_NAME, "", 0);
	if (STR_EQ(eval_hostname(request.client), unknown))
	    tcpd_warn("host address %s->name lookup failed",
		      eval_hostaddr(request.client));
    }
    freeaddrinfo(res0);
    return (count);
}

/* dummy function to intercept the real shell_cmd() */

/* ARGSUSED */

void shell_cmd(char *command)
{
    if (hosts_access_verbose)
	printf("command: %s", command);
}

/* dummy function  to intercept the real clean_exit() */

/* ARGSUSED */

void clean_exit(struct request_info *request)
{
    exit(0);
}

#if 0
/* dummy function  to intercept the real rfc931() */

/* ARGSUSED */

void
rfc931(struct request_info *request)
{
    strlcpy(request->user, unknown, sizeof(request->user));
}
#endif

/* check_path - examine accessibility */

int
check_path(const char *path, struct stat *st)
{
    struct stat stbuf;
    char    buf[BUFSIZ];

    if (stat(path, st) < 0)
	return (-1);
#ifdef notdef
    if (st->st_uid != 0)
	tcpd_warn("%s: not owned by root", path);
    if (st->st_mode & 020)
	tcpd_warn("%s: group writable", path);
#endif
    if (st->st_mode & 002)
	tcpd_warn("%s: world writable", path);
    if (path[0] == '/' && path[1] != 0) {
	strlcpy(buf, path, sizeof(buf));
	strrchr(buf, '/')[0] = 0;
	(void) check_path(buf[0] ? buf : "/", &stbuf);
    }
    return (0);
}
