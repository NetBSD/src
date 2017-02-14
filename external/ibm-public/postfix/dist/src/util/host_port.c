/*	$NetBSD: host_port.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	host_port 3
/* SUMMARY
/*	split string into host and port, destroy string
/* SYNOPSIS
/*	#include <host_port.h>
/*
/*	const char *host_port(string, host, def_host, port, def_service)
/*	char	*string;
/*	char	**host;
/*	char	*def_host;
/*	char	**port;
/*	char	*def_service;
/* DESCRIPTION
/*	host_port() splits a string into substrings with the host
/*	name or address, and the service name or port number.
/*	The input string is modified.
/*
/*	Host/domain names are validated with valid_utf8_hostname(),
/*	and host addresses are validated with valid_hostaddr().
/*
/*	The following input formats are understood (null means
/*	a null pointer argument):
/*
/*	When def_service is not null, and def_host is null:
/*
/*		[host]:port, [host]:, [host]
/*
/*		host:port, host:, host
/*
/*	When def_host is not null, and def_service is null:
/*
/*		:port, port
/*
/*	Other combinations of def_service and def_host are
/*	not supported and produce undefined results.
/* DIAGNOSTICS
/*	The result is a null pointer in case of success.
/*	In case of problems the result is a string pointer with
/*	the problem type.
/* CLIENT EXAMPLE
/* .ad
/* .fi
/*	Typical client usage allows the user to omit the service port,
/*	in which case the client connects to a pre-determined default
/*	port:
/* .nf
/* .na
/*
/*	buf = mystrdup(endpoint);
/*	if ((parse_error = host_port(buf, &host, NULL, &port, defport)) != 0)
/*	    msg_fatal("%s in \"%s\"", parse_error, endpoint);
/*	if ((aierr = hostname_to_sockaddr(host, port, SOCK_STREAM, &res)) != 0)
/*	    msg_fatal("%s: %s", endpoint, MAI_STRERROR(aierr));
/*	myfree(buf);
/* SERVER EXAMPLE
/* .ad
/* .fi
/*	Typical server usage allows the user to omit the host, meaning
/*	listen on all available network addresses:
/* .nf
/* .na
/*
/*	buf = mystrdup(endpoint);
/*	if ((parse_error = host_port(buf, &host, "", &port, NULL)) != 0)
/*	    msg_fatal("%s in \"%s\"", parse_error, endpoint);
/*	if (*host == 0)
/*	    host = 0;
/*	if ((aierr = hostname_to_sockaddr(host, port, SOCK_STREAM, &res)) != 0)
/*	    msg_fatal("%s: %s", endpoint, MAI_STRERROR(aierr));
/*	myfree(buf);
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <split_at.h>
#include <stringops.h>			/* XXX util_utf8_enable */
#include <valid_utf8_hostname.h>

/* Global library. */

#include <host_port.h>

 /*
  * Point-fix workaround. The libutil library should be email agnostic, but
  * we can't rip up the library APIs in the stable releases.
  */
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif
#define IPV6_COL           "IPv6:"	/* RFC 2821 */
#define IPV6_COL_LEN       (sizeof(IPV6_COL) - 1)
#define HAS_IPV6_COL(str)  (strncasecmp((str), IPV6_COL, IPV6_COL_LEN) == 0)

/* host_port - parse string into host and port, destroy string */

const char *host_port(char *buf, char **host, char *def_host,
		              char **port, char *def_service)
{
    char   *cp = buf;
    int     ipv6 = 0;

    /*-
     * [host]:port, [host]:, [host].
     * [ipv6:ipv6addr]:port, [ipv6:ipv6addr]:, [ipv6:ipv6addr].
     */
    if (*cp == '[') {
	++cp;
	if ((ipv6 = HAS_IPV6_COL(cp)) != 0)
	    cp += IPV6_COL_LEN;
	*host = cp;
	if ((cp = split_at(cp, ']')) == 0)
	    return ("missing \"]\"");
	if (*cp && *cp++ != ':')
	    return ("garbage after \"]\"");
	if (ipv6 && !valid_ipv6_hostaddr(*host, DONT_GRIPE))
	    return ("malformed IPv6 address");
	*port = *cp ? cp : def_service;
    }

    /*
     * host:port, host:, host, :port, port.
     */
    else {
	if ((cp = split_at_right(buf, ':')) != 0) {
	    *host = *buf ? buf : def_host;
	    *port = *cp ? cp : def_service;
	} else {
	    *host = def_host ? def_host : (*buf ? buf : 0);
	    *port = def_service ? def_service : (*buf ? buf : 0);
	}
    }
    if (*host == 0)
	return ("missing host information");
    if (*port == 0)
	return ("missing service information");

    /*
     * Final sanity checks. We're still sloppy, allowing bare numerical
     * network addresses instead of requiring proper [ipaddress] forms.
     */
    if (*host != def_host 
	&& !valid_utf8_hostname(util_utf8_enable, *host, DONT_GRIPE)
	&& !valid_hostaddr(*host, DONT_GRIPE))
	return ("valid hostname or network address required");
    if (*port != def_service && ISDIGIT(**port) && !alldig(*port))
	return ("garbage after numerical service");
    return (0);
}

#ifdef TEST

#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

#define STR(x) vstring_str(x)

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *in_buf = vstring_alloc(10);
    VSTRING *parse_buf = vstring_alloc(10);
    char   *host;
    char   *port;
    const char *err;

    while (vstring_fgets_nonl(in_buf, VSTREAM_IN)) {
	vstream_printf(">> %s\n", STR(in_buf));
	vstream_fflush(VSTREAM_OUT);
	if (*STR(in_buf) == '#')
	    continue;
	vstring_strcpy(parse_buf, STR(in_buf));
	if ((err = host_port(STR(parse_buf), &host, (char *) 0, &port, "default-service")) != 0) {
	    msg_warn("%s in %s", err, STR(in_buf));
	} else {
	    vstream_printf("host %s port %s\n", host, port);
	    vstream_fflush(VSTREAM_OUT);
	}
    }
    vstring_free(in_buf);
    vstring_free(parse_buf);
    return (0);
}

#endif
