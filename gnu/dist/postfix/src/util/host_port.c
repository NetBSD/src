/*++
/* NAME
/*	host_port 3
/* SUMMARY
/*	split string into host and port, destroy string
/* SYNOPSIS
/*	#include <host_port.h>
/*
/*	const char *host_port(string, host, port, def_service)
/*	char	*string;
/*	char	**host;
/*	char	**port;
/*	char	*def_service;
/* DESCRIPTION
/*	host_port() splits a string into substrings with the host
/*	name or address, and the service name or port number.
/*	The input string is modified.
/*
/*	The following input formats are understood:
/*
/*	[host]:port, [host]:, [host].
/*
/*	host:port, host:, host.
/* DIAGNOSTICS
/*	The result is a null pointer in case of success.
/*	In case of problems the result is a string pointer with
/*	the problem type.
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
#include <stringops.h>
#include <valid_hostname.h>

/* Global library. */

#include <host_port.h>

/* host_port - parse string into host and port, destroy string */

const char *host_port(char *buf, char **host, char **port,
		              char *def_service)
{
    char   *cp = buf;

    /*
     * [host]:port, [host]:, [host].
     */
    if (*cp == '[') {
	*host = ++cp;
	if ((cp = split_at(cp, ']')) == 0)
	    return ("missing \"]\"");
	if (*cp && *cp++ != ':')
	    return ("garbage after \"]\"");
	if (*cp == 0)
	    *port = def_service;
	else
	    *port = cp;

	/*
	 * [host:port], [host:]. These conflict with IPV6 address literals.
	 */
#if 1
	if (strchr(*host, ':')) {
	    msg_warn("old-style address form: [%s]", *host);
	    msg_warn("support for [host:port] forms will go away");
	    msg_warn("specify [host]:port instead");
	    return (host_port(buf + 1, host, port, def_service));
	}
#endif
    }

    /*
     * host:port, host:, host.
     */
    else {
	*host = cp;
	if ((cp = split_at_right(cp, ':')) == 0 || *cp == 0)
	    *port = def_service;
	else
	    *port = cp;
    }

    /*
     * Final sanity checks. We're still sloppy, allowing bare numerical
     * network addresses instead of requiring proper [ipaddress] forms.
     */
    if (!valid_hostname(*host, DONT_GRIPE)
	&& !valid_hostaddr(*host, DONT_GRIPE))
	return ("valid hostname or network address required");
    if (ISDIGIT(**port) && !alldig(*port))
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
	vstring_strcpy(parse_buf, STR(in_buf));
	if ((err = host_port(STR(parse_buf), &host, &port, "default-service")) != 0) {
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
