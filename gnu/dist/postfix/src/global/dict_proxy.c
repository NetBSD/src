/*++
/* NAME
/*	dict_proxy 3
/* SUMMARY
/*	generic dictionary proxy client
/* SYNOPSIS
/*	#include <dict_proxy.h>
/*
/*	DICT	*dict_proxy_open(map, open_flags, dict_flags)
/*	const char *map;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_proxy_open() relays read-only operations through
/*	the Postfix proxymap server.
/*
/*	The \fIopen_flags\fR argument must specify O_RDONLY.
/*
/*	The connection to the Postfix proxymap server is automatically
/*	closed after $ipc_idle seconds of idle time, or after $ipc_ttl
/*	seconds of activity.
/* SECURITY
/*      The proxy map server is not meant to be a trusted process. Proxy
/*	maps must not be used to look up security sensitive information
/*	such as user/group IDs, output files, or external commands.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	clnt_stream(3) client endpoint connection management
/* DIAGNOSTICS
/*	Fatal errors: out of memory, unimplemented operation,
/*	bad request parameter, map not approved for proxy access.
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
#include <errno.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>
#include <vstream.h>
#include <attr.h>
#include <dict.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <clnt_stream.h>
#include <dict_proxy.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    int     in_flags;			/* caller-specified flags */
    VSTRING *result;			/* storage */
} DICT_PROXY;

 /*
  * SLMs.
  */
#define STR(x)		vstring_str(x)
#define VSTREQ(v,s)	(strcmp(STR(v),s) == 0)

 /*
  * All proxied maps within a process share the same query/reply socket.
  */
static CLNT_STREAM *proxy_stream;

/* dict_proxy_lookup - find table entry */

static const char *dict_proxy_lookup(DICT *dict, const char *key)
{
    const char *myname = "dict_proxy_lookup";
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;
    VSTREAM *stream;
    int     status;

    /*
     * The client and server live in separate processes that may start and
     * terminate independently. We cannot rely on a persistent connection,
     * let alone on persistent state (such as a specific open table) that is
     * associated with a specific connection. Each lookup needs to specify
     * the table and the flags that were specified to dict_proxy_open().
     */
    VSTRING_RESET(dict_proxy->result);
    VSTRING_TERMINATE(dict_proxy->result);
    for (;;) {
	stream = clnt_stream_access(proxy_stream);
	errno = 0;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_LOOKUP,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict->name,
		       ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, dict_proxy->in_flags,
		       ATTR_TYPE_STR, MAIL_ATTR_KEY, key,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_STR, MAIL_ATTR_VALUE, dict_proxy->result,
			 ATTR_TYPE_END) != 2) {
	    if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", myname, VSTREAM_PATH(stream));
	} else {
	    if (msg_verbose)
		msg_info("%s: table=%s flags=0%o key=%s -> status=%d result=%s",
			 myname, dict->name, dict_proxy->in_flags, key,
			 status, STR(dict_proxy->result));
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s lookup failed for table \"%s\" key \"%s\": "
			  "invalid request",
			  MAIL_SERVICE_PROXYMAP, dict->name, key);
	    case PROXY_STAT_DENY:
		msg_fatal("%s service is not configured for table \"%s\"",
			  MAIL_SERVICE_PROXYMAP, dict->name);
	    case PROXY_STAT_OK:
		return (STR(dict_proxy->result));
	    case PROXY_STAT_NOKEY:
		dict_errno = 0;
		return (0);
	    case PROXY_STAT_RETRY:
		dict_errno = DICT_ERR_RETRY;
		return (0);
	    default:
		msg_warn("%s lookup failed for table \"%s\" key \"%s\": "
			 "unexpected reply status %d",
			 MAIL_SERVICE_PROXYMAP, dict->name, key, status);
	    }
	}
	clnt_stream_recover(proxy_stream);
	sleep(1);				/* XXX make configurable */
    }
}

/* dict_proxy_close - disconnect */

static void dict_proxy_close(DICT *dict)
{
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;

    vstring_free(dict_proxy->result);
    dict_free(dict);
}

/* dict_proxy_open - open remote map */

DICT   *dict_proxy_open(const char *map, int open_flags, int dict_flags)
{
    const char *myname = "dict_proxy_open";
    DICT_PROXY *dict_proxy;
    VSTREAM *stream;
    int     server_flags;
    int     status;
    char   *kludge = 0;
    char   *prefix;

    /*
     * Sanity checks.
     */
    if (dict_flags & DICT_FLAG_NO_PROXY)
	msg_fatal("%s: %s map is not allowed for security sensitive data",
		  map, DICT_TYPE_PROXY);
    if (open_flags != O_RDONLY)
	msg_fatal("%s: %s map open requires O_RDONLY access mode",
		  map, DICT_TYPE_PROXY);

    /*
     * Local initialization.
     */
    dict_proxy = (DICT_PROXY *)
	dict_alloc(DICT_TYPE_PROXY, map, sizeof(*dict_proxy));
    dict_proxy->dict.lookup = dict_proxy_lookup;
    dict_proxy->dict.close = dict_proxy_close;
    dict_proxy->in_flags = dict_flags;
    dict_proxy->result = vstring_alloc(10);

    /*
     * Use a shared stream for all proxied table lookups.
     * 
     * XXX Use absolute pathname to make this work from non-daemon processes.
     */
    if (proxy_stream == 0) {
	if (access(MAIL_CLASS_PRIVATE "/" MAIL_SERVICE_PROXYMAP, F_OK) == 0)
	    prefix = MAIL_CLASS_PRIVATE;
	else
	    prefix = kludge = concatenate(var_queue_dir, "/",
					  MAIL_CLASS_PRIVATE, (char *) 0);
	proxy_stream = clnt_stream_create(prefix,
					  MAIL_SERVICE_PROXYMAP,
					  var_ipc_idle_limit,
					  var_ipc_ttl_limit);
	if (kludge)
	    myfree(kludge);
    }

    /*
     * Establish initial contact and get the map type specific flags.
     * 
     * XXX Should retrieve flags from local instance.
     */
    for (;;) {
	stream = clnt_stream_access(proxy_stream);
	errno = 0;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_OPEN,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict_proxy->dict.name,
		       ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, dict_proxy->in_flags,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, &server_flags,
			 ATTR_TYPE_END) != 2) {
	    if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", VSTREAM_PATH(stream), myname);
	} else {
	    if (msg_verbose)
		msg_info("%s: connect to map=%s status=%d server_flags=0%o",
		       myname, dict_proxy->dict.name, status, server_flags);
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s open failed for table \"%s\": invalid request",
			  MAIL_SERVICE_PROXYMAP, dict_proxy->dict.name);
	    case PROXY_STAT_DENY:
		msg_fatal("%s service is not configured for table \"%s\"",
			  MAIL_SERVICE_PROXYMAP, dict_proxy->dict.name);
	    case PROXY_STAT_OK:
		dict_proxy->dict.flags = dict_proxy->in_flags | server_flags;
		return (DICT_DEBUG (&dict_proxy->dict));
	    default:
		msg_warn("%s open failed for table \"%s\": unexpected status %d",
		      MAIL_SERVICE_PROXYMAP, dict_proxy->dict.name, status);
	    }
	}
	clnt_stream_recover(proxy_stream);
	sleep(1);				/* XXX make configurable */
    }
}
