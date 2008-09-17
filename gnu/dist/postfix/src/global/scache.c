/*++
/* NAME
/*	scache 3
/* SUMMARY
/*	generic session cache API
/* SYNOPSIS
/*	#include <scache.h>
/* DESCRIPTION
/*	typedef struct {
/* .in +4
/*		int	dest_count;
/*		int	endp_count;
/*		int	sess_count;
/* .in -4
/*	} SCACHE_SIZE;
/*
/*	unsigned scache_size(scache, size)
/*	SCACHE	*scache;
/*	SCACHE_SIZE *size;
/*
/*	void	scache_free(scache)
/*	SCACHE	*scache;
/*
/*	void    scache_save_endp(scache, endp_ttl, endp_label, endp_prop, fd)
/*	SCACHE	*scache;
/*	int	endp_ttl;
/*	const char *endp_label;
/*	const char *endp_prop;
/*	int	fd;
/*
/*	int     scache_find_endp(scache, endp_label, endp_prop)
/*	SCACHE	*scache;
/*	const char *endp_label;
/*	VSTRING	*endp_prop;
/*
/*	void	scache_save_dest(scache, dest_ttl, dest_label,
/*				dest_prop, endp_label)
/*	SCACHE	*scache;
/*	int	dest_ttl;
/*	const char *dest_label;
/*	const char *dest_prop;
/*	const char *endp_label;
/*
/*	int	scache_find_dest(dest_label, dest_prop, endp_prop)
/*	SCACHE	*scache;
/*	const char *dest_label;
/*	VSTRING	*dest_prop;
/*	VSTRING	*endp_prop;
/* DESCRIPTION
/*	This module implements a generic session cache interface.
/*	Specific cache types are described in scache_single(3),
/*	scache_clnt(3) and scache_multi(3). These documents also
/*	describe now to instantiate a specific session cache type.
/*
/*	The code maintains two types of association: a) physical
/*	endpoint to file descriptor, and b) logical endpoint
/*	to physical endpoint. Physical endpoints are stored and
/*	looked up under their low-level session details such as
/*	numerical addresses, while logical endpoints are stored
/*	and looked up by the domain name that humans use. One logical
/*	endpoint can refer to multiple physical endpoints, one
/*	physical endpoint may be referenced by multiple logical
/*	endpoints, and one physical endpoint may have multiple
/*	sessions.
/*
/*	scache_size() returns the number of logical destination
/*	names, physical endpoint addresses, and cached sessions.
/*
/*	scache_free() destroys the specified session cache.
/*
/*	scache_save_endp() stores an open session under the specified
/*	physical endpoint name.
/*
/*	scache_find_endp() looks up a saved session under the
/*	specified physical endpoint name.
/*
/*	scache_save_dest() associates the specified physical endpoint
/*	with the specified logical endpoint name.
/*
/*	scache_find_dest() looks up a saved session under the
/*	specified physical endpoint name.
/*
/*	Arguments:
/* .IP endp_ttl
/*	How long the session should be cached.  When information
/*	expires it is purged automatically.
/* .IP endp_label
/*      The transport name and the physical endpoint name under
/*      which the session is stored and looked up.
/*
/*	In the case of SMTP, the physical endpoint includes the numerical
/*	IP address, address family information, and the numerical TCP port.
/* .IP endp_prop
/*	Application-specific data with endpoint attributes.  It is up to
/*	the application to passivate (flatten) and re-activate this content
/*	upon storage and retrieval, respectively.
/* .sp
/*	In the case of SMTP, the endpoint attributes specify the
/*	server hostname, IP address, numerical TCP port, as well
/*	as ESMTP features advertised by the server, and when information
/*	expires. All this in some application-specific format that is easy
/*	to unravel when re-activating a cached session.
/* .IP dest_ttl
/*	How long the destination-to-endpoint binding should be
/*	cached. When information expires it is purged automatically.
/* .IP dest_label
/*	The transport name and the logical destination under which the
/*	destination-to-endpoint binding is stored and looked up.
/*
/*	In the case of SMTP, the logical destination is the DNS
/*	host or domain name with or without [], plus the numerical TCP port.
/* .IP dest_prop
/*	Application-specific attributes that describe features of
/*	this logical to physical binding. It is up to the application
/*	to passivate (flatten) and re-activate this content.
/*	upon storage and retrieval, respectively
/* .sp
/*	In case the of an SMTP logical destination to physical
/*	endpoint binding, the attributes specify the logical
/*	destination name, numerical port, whether the physical
/*	endpoint is best mx host with respect to a logical or
/*	fall-back destination, and when information expires.
/* .IP fd
/*	File descriptor with session to be cached.
/* DIAGNOSTICS
/*	scache_find_endp() and scache_find_dest() return -1 when
/*	the lookup fails, and a file descriptor upon success.
/*
/*	Other diagnostics: fatal error: memory allocation problem;
/*	panic: internal consistency failure.
/* SEE ALSO
/*	scache_single(3), single-session, in-memory cache
/*	scache_clnt(3), session cache client
/*	scache_multi(3), multi-session, in-memory cache
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <argv.h>
#include <events.h>

/* Global library. */

#include <scache.h>

#ifdef TEST

 /*
  * Driver program for cache regression tests. Although all variants are
  * relatively simple to verify by hand for single session storage, more
  * sophisticated instrumentation is needed to demonstrate that the
  * multi-session cache manager properly handles collisions in the time
  * domain and in all the name space domains.
  */
static SCACHE *scache;
static VSTRING *endp_prop;
static VSTRING *dest_prop;
static int verbose_level = 3;

 /*
  * Cache mode lookup table. We don't do the client-server variant because
  * that drags in a ton of configuration junk; the client-server protocol is
  * relatively easy to verify manually.
  */
struct cache_type {
    char   *mode;
    SCACHE *(*create) (void);
};

static struct cache_type cache_types[] = {
    "single", scache_single_create,
    "multi", scache_multi_create,
    0,
};

#define STR(x) vstring_str(x)

/* cache_type - select session cache type */

static void cache_type(ARGV *argv)
{
    struct cache_type *cp;

    if (argv->argc != 2) {
	msg_error("usage: %s mode", argv->argv[0]);
	return;
    }
    if (scache != 0)
	scache_free(scache);
    for (cp = cache_types; cp->mode != 0; cp++) {
	if (strcmp(cp->mode, argv->argv[1]) == 0) {
	    scache = cp->create();
	    return;
	}
    }
    msg_error("unknown cache type: %s", argv->argv[1]);
}

/* handle_events - handle events while time advances */

static void handle_events(ARGV *argv)
{
    int     delay;
    time_t  before;
    time_t  after;

    if (argv->argc != 2 || (delay = atoi(argv->argv[1])) <= 0) {
	msg_error("usage: %s time", argv->argv[0]);
	return;
    }
    before = event_time();
    event_drain(delay);
    after = event_time();
    if (after < before + delay)
	sleep(before + delay - after);
}

/* save_endp - save endpoint->session binding */

static void save_endp(ARGV *argv)
{
    int     ttl;
    int     fd;

    if (argv->argc != 5
	|| (ttl = atoi(argv->argv[1])) <= 0
	|| (fd = atoi(argv->argv[4])) <= 0) {
	msg_error("usage: save_endp ttl endpoint endp_props fd");
	return;
    }
    if (DUP2(0, fd) < 0)
	msg_fatal("dup2(0, %d): %m", fd);
    scache_save_endp(scache, ttl, argv->argv[2], argv->argv[3], fd);
}

/* find_endp - find endpoint->session binding */

static void find_endp(ARGV *argv)
{
    int     fd;

    if (argv->argc != 2) {
	msg_error("usage: find_endp endpoint");
	return;
    }
    if ((fd = scache_find_endp(scache, argv->argv[1], endp_prop)) >= 0)
	close(fd);
}

/* save_dest - save destination->endpoint binding */

static void save_dest(ARGV *argv)
{
    int     ttl;

    if (argv->argc != 5 || (ttl = atoi(argv->argv[1])) <= 0) {
	msg_error("usage: save_dest ttl destination dest_props endpoint");
	return;
    }
    scache_save_dest(scache, ttl, argv->argv[2], argv->argv[3], argv->argv[4]);
}

/* find_dest - find destination->endpoint->session binding */

static void find_dest(ARGV *argv)
{
    int     fd;

    if (argv->argc != 2) {
	msg_error("usage: find_dest destination");
	return;
    }
    if ((fd = scache_find_dest(scache, argv->argv[1], dest_prop, endp_prop)) >= 0)
	close(fd);
}

/* verbose - adjust noise level during cache manipulation */

static void verbose(ARGV *argv)
{
    int     level;

    if (argv->argc != 2 || (level = atoi(argv->argv[1])) < 0) {
	msg_error("usage: verbose level");
	return;
    }
    verbose_level = level;
}

 /*
  * The command lookup table.
  */
struct action {
    char   *command;
    void    (*action) (ARGV *);
    int     flags;
};

#define FLAG_NEED_CACHE	(1<<0)

static void help(ARGV *);

static struct action actions[] = {
    "cache_type", cache_type, 0,
    "save_endp", save_endp, FLAG_NEED_CACHE,
    "find_endp", find_endp, FLAG_NEED_CACHE,
    "save_dest", save_dest, FLAG_NEED_CACHE,
    "find_dest", find_dest, FLAG_NEED_CACHE,
    "sleep", handle_events, 0,
    "verbose", verbose, 0,
    "?", help, 0,
    0,
};

/* help - list commands */

static void help(ARGV *argv)
{
    struct action *ap;

    vstream_printf("commands:");
    for (ap = actions; ap->command != 0; ap++)
	vstream_printf(" %s", ap->command);
    vstream_printf("\n");
    vstream_fflush(VSTREAM_OUT);
}

/* get_buffer - prompt for input or log input */

static int get_buffer(VSTRING *buf, VSTREAM *fp, int interactive)
{
    int     status;

    if (interactive) {
	vstream_printf("> ");
	vstream_fflush(VSTREAM_OUT);
    }
    if ((status = vstring_get_nonl(buf, fp)) != VSTREAM_EOF) {
	if (!interactive) {
	    vstream_printf(">>> %s\n", STR(buf));
	    vstream_fflush(VSTREAM_OUT);
	}
    }
    return (status);
}

/* at last, the main program */

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(1);
    ARGV   *argv;
    struct action *ap;
    int     interactive = isatty(0);

    endp_prop = vstring_alloc(1);
    dest_prop = vstring_alloc(1);

    vstream_fileno(VSTREAM_ERR) = 1;

    while (get_buffer(buf, VSTREAM_IN, interactive) != VSTREAM_EOF) {
	argv = argv_split(STR(buf), " \t\r\n");
	if (argv->argc > 0 && argv->argv[0][0] != '#') {
	    msg_verbose = verbose_level;
	    for (ap = actions; ap->command != 0; ap++) {
		if (strcmp(ap->command, argv->argv[0]) == 0) {
		    if ((ap->flags & FLAG_NEED_CACHE) != 0 && scache == 0)
			msg_error("no session cache");
		    else
			ap->action(argv);
		    break;
		}
	    }
	    msg_verbose = 0;
	    if (ap->command == 0)
		msg_error("bad command: %s", argv->argv[0]);
	}
	argv_free(argv);
    }
    scache_free(scache);
    vstring_free(endp_prop);
    vstring_free(dest_prop);
    vstring_free(buf);
    exit(0);
}

#endif
