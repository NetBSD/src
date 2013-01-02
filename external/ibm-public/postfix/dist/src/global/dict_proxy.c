/*	$NetBSD: dict_proxy.c,v 1.1.1.2 2013/01/02 18:58:57 tron Exp $	*/

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
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_proxy_open() relays read-only or read-write operations
/*	through the Postfix proxymap server.
/*
/*	The \fIopen_flags\fR argument must specify O_RDONLY
/*	or O_RDWR. Depending on this, the client
/*	connects to the proxymap multiserver or to the
/*	proxywrite single updater.
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
    CLNT_STREAM *clnt;			/* client handle (shared) */
    const char *service;		/* service name */
    int     inst_flags;			/* saved dict flags */
    VSTRING *reskey;			/* result key storage */
    VSTRING *result;			/* storage */
} DICT_PROXY;

 /*
  * SLMs.
  */
#define STR(x)		vstring_str(x)
#define VSTREQ(v,s)	(strcmp(STR(v),s) == 0)

 /*
  * All proxied maps of the same type share the same query/reply socket.
  */
static CLNT_STREAM *proxymap_stream;	/* read-only maps */
static CLNT_STREAM *proxywrite_stream;	/* read-write maps */

/* dict_proxy_sequence - find first/next entry */

static int dict_proxy_sequence(DICT *dict, int function,
			               const char **key, const char **value)
{
    const char *myname = "dict_proxy_sequence";
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;
    VSTREAM *stream;
    int     status;
    int     count = 0;
    int     request_flags;

    /*
     * The client and server live in separate processes that may start and
     * terminate independently. We cannot rely on a persistent connection,
     * let alone on persistent state (such as a specific open table) that is
     * associated with a specific connection. Each lookup needs to specify
     * the table and the flags that were specified to dict_proxy_open().
     */
    VSTRING_RESET(dict_proxy->reskey);
    VSTRING_TERMINATE(dict_proxy->reskey);
    VSTRING_RESET(dict_proxy->result);
    VSTRING_TERMINATE(dict_proxy->result);
    request_flags = dict_proxy->inst_flags
	| (dict->flags & DICT_FLAG_RQST_MASK);
    for (;;) {
	stream = clnt_stream_access(dict_proxy->clnt);
	errno = 0;
	count += 1;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_SEQUENCE,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict->name,
		       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, request_flags,
		       ATTR_TYPE_INT, MAIL_ATTR_FUNC, function,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_STR, MAIL_ATTR_KEY, dict_proxy->reskey,
			 ATTR_TYPE_STR, MAIL_ATTR_VALUE, dict_proxy->result,
			 ATTR_TYPE_END) != 3) {
	    if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", myname, VSTREAM_PATH(stream));
	} else {
	    if (msg_verbose)
		msg_info("%s: table=%s flags=%s func=%d -> status=%d key=%s val=%s",
			 myname, dict->name, dict_flags_str(request_flags),
			 function, status, STR(dict_proxy->reskey),
			 STR(dict_proxy->result));
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s sequence failed for table \"%s\" function %d: "
			  "invalid request",
			  dict_proxy->service, dict->name, function);
	    case PROXY_STAT_DENY:
		msg_fatal("%s service is not configured for table \"%s\"",
			  dict_proxy->service, dict->name);
	    case PROXY_STAT_OK:
		*key = STR(dict_proxy->reskey);
		*value = STR(dict_proxy->result);
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
	    case PROXY_STAT_NOKEY:
		*key = *value = 0;
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
	    case PROXY_STAT_RETRY:
		*key = *value = 0;
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
	    default:
		msg_warn("%s sequence failed for table \"%s\" function %d: "
			 "unexpected reply status %d",
			 dict_proxy->service, dict->name, function, status);
	    }
	}
	clnt_stream_recover(dict_proxy->clnt);
	sleep(1);				/* XXX make configurable */
    }
}

/* dict_proxy_lookup - find table entry */

static const char *dict_proxy_lookup(DICT *dict, const char *key)
{
    const char *myname = "dict_proxy_lookup";
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;
    VSTREAM *stream;
    int     status;
    int     count = 0;
    int     request_flags;

    /*
     * The client and server live in separate processes that may start and
     * terminate independently. We cannot rely on a persistent connection,
     * let alone on persistent state (such as a specific open table) that is
     * associated with a specific connection. Each lookup needs to specify
     * the table and the flags that were specified to dict_proxy_open().
     */
    VSTRING_RESET(dict_proxy->result);
    VSTRING_TERMINATE(dict_proxy->result);
    request_flags = dict_proxy->inst_flags
	| (dict->flags & DICT_FLAG_RQST_MASK);
    for (;;) {
	stream = clnt_stream_access(dict_proxy->clnt);
	errno = 0;
	count += 1;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_LOOKUP,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict->name,
		       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, request_flags,
		       ATTR_TYPE_STR, MAIL_ATTR_KEY, key,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_STR, MAIL_ATTR_VALUE, dict_proxy->result,
			 ATTR_TYPE_END) != 2) {
	    if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", myname, VSTREAM_PATH(stream));
	} else {
	    if (msg_verbose)
		msg_info("%s: table=%s flags=%s key=%s -> status=%d result=%s",
			 myname, dict->name,
			 dict_flags_str(request_flags), key,
			 status, STR(dict_proxy->result));
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s lookup failed for table \"%s\" key \"%s\": "
			  "invalid request",
			  dict_proxy->service, dict->name, key);
	    case PROXY_STAT_DENY:
		msg_fatal("%s service is not configured for table \"%s\"",
			  dict_proxy->service, dict->name);
	    case PROXY_STAT_OK:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, STR(dict_proxy->result));
	    case PROXY_STAT_NOKEY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, (char *) 0);
	    case PROXY_STAT_RETRY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, (char *) 0);
	    default:
		msg_warn("%s lookup failed for table \"%s\" key \"%s\": "
			 "unexpected reply status %d",
			 dict_proxy->service, dict->name, key, status);
	    }
	}
	clnt_stream_recover(dict_proxy->clnt);
	sleep(1);				/* XXX make configurable */
    }
}

/* dict_proxy_update - update table entry */

static int dict_proxy_update(DICT *dict, const char *key, const char *value)
{
    const char *myname = "dict_proxy_update";
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;
    VSTREAM *stream;
    int     status;
    int     count = 0;
    int     request_flags;

    /*
     * The client and server live in separate processes that may start and
     * terminate independently. We cannot rely on a persistent connection,
     * let alone on persistent state (such as a specific open table) that is
     * associated with a specific connection. Each lookup needs to specify
     * the table and the flags that were specified to dict_proxy_open().
     */
    request_flags = dict_proxy->inst_flags
	| (dict->flags & DICT_FLAG_RQST_MASK);
    for (;;) {
	stream = clnt_stream_access(dict_proxy->clnt);
	errno = 0;
	count += 1;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_UPDATE,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict->name,
		       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, request_flags,
		       ATTR_TYPE_STR, MAIL_ATTR_KEY, key,
		       ATTR_TYPE_STR, MAIL_ATTR_VALUE, value,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_END) != 1) {
	    if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", myname, VSTREAM_PATH(stream));
	} else {
	    if (msg_verbose)
		msg_info("%s: table=%s flags=%s key=%s value=%s -> status=%d",
			 myname, dict->name, dict_flags_str(request_flags),
			 key, value, status);
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s update failed for table \"%s\" key \"%s\": "
			  "invalid request",
			  dict_proxy->service, dict->name, key);
	    case PROXY_STAT_DENY:
		msg_fatal("%s update access is not configured for table \"%s\"",
			  dict_proxy->service, dict->name);
	    case PROXY_STAT_OK:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
	    case PROXY_STAT_NOKEY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
	    case PROXY_STAT_RETRY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
	    default:
		msg_warn("%s update failed for table \"%s\" key \"%s\": "
			 "unexpected reply status %d",
			 dict_proxy->service, dict->name, key, status);
	    }
	}
	clnt_stream_recover(dict_proxy->clnt);
	sleep(1);				/* XXX make configurable */
    }
}

/* dict_proxy_delete - delete table entry */

static int dict_proxy_delete(DICT *dict, const char *key)
{
    const char *myname = "dict_proxy_delete";
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;
    VSTREAM *stream;
    int     status;
    int     count = 0;
    int     request_flags;

    /*
     * The client and server live in separate processes that may start and
     * terminate independently. We cannot rely on a persistent connection,
     * let alone on persistent state (such as a specific open table) that is
     * associated with a specific connection. Each lookup needs to specify
     * the table and the flags that were specified to dict_proxy_open().
     */
    request_flags = dict_proxy->inst_flags
	| (dict->flags & DICT_FLAG_RQST_MASK);
    for (;;) {
	stream = clnt_stream_access(dict_proxy->clnt);
	errno = 0;
	count += 1;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_DELETE,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict->name,
		       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, request_flags,
		       ATTR_TYPE_STR, MAIL_ATTR_KEY, key,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_END) != 1) {
	    if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno !=
					     ENOENT))
		msg_warn("%s: service %s: %m", myname, VSTREAM_PATH(stream));
	} else {
	    if (msg_verbose)
		msg_info("%s: table=%s flags=%s key=%s -> status=%d",
			 myname, dict->name, dict_flags_str(request_flags),
			 key, status);
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s delete failed for table \"%s\" key \"%s\": "
			  "invalid request",
			  dict_proxy->service, dict->name, key);
	    case PROXY_STAT_DENY:
		msg_fatal("%s update access is not configured for table \"%s\"",
			  dict_proxy->service, dict->name);
	    case PROXY_STAT_OK:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
	    case PROXY_STAT_NOKEY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
	    case PROXY_STAT_RETRY:
		DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
	    default:
		msg_warn("%s delete failed for table \"%s\" key \"%s\": "
			 "unexpected reply status %d",
			 dict_proxy->service, dict->name, key, status);
	    }
	}
	clnt_stream_recover(dict_proxy->clnt);
	sleep(1);				/* XXX make configurable */
    }
}

/* dict_proxy_close - disconnect */

static void dict_proxy_close(DICT *dict)
{
    DICT_PROXY *dict_proxy = (DICT_PROXY *) dict;

    vstring_free(dict_proxy->reskey);
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
    const char *service;
    char   *relative_path;
    char   *kludge = 0;
    char   *prefix;
    CLNT_STREAM **pstream;

    /*
     * If this map can't be proxied then we silently do a direct open. This
     * allows sites to benefit from proxying the virtual mailbox maps without
     * unnecessary pain.
     */
    if (dict_flags & DICT_FLAG_NO_PROXY)
	return (dict_open(map, open_flags, dict_flags));

    /*
     * Use a shared stream for proxied table lookups of the same type.
     * 
     * XXX A complete implementation would also allow O_RDWR without O_CREAT.
     * But we must not pass on every possible set of flags to the proxy
     * server; only sets that make sense. For now, the flags are passed
     * implicitly by choosing between the proxymap or proxywrite service.
     * 
     * XXX Use absolute pathname to make this work from non-daemon processes.
     */
    if (open_flags == O_RDONLY) {
	pstream = &proxymap_stream;
	service = var_proxymap_service;
    } else if ((open_flags & O_RDWR) == O_RDWR) {
	pstream = &proxywrite_stream;
	service = var_proxywrite_service;
    } else
	msg_fatal("%s: %s map open requires O_RDONLY or O_RDWR mode",
		  map, DICT_TYPE_PROXY);

    if (*pstream == 0) {
	relative_path = concatenate(MAIL_CLASS_PRIVATE "/",
				    service, (char *) 0);
	if (access(relative_path, F_OK) == 0)
	    prefix = MAIL_CLASS_PRIVATE;
	else
	    prefix = kludge = concatenate(var_queue_dir, "/",
					  MAIL_CLASS_PRIVATE, (char *) 0);
	*pstream = clnt_stream_create(prefix, service, var_ipc_idle_limit,
				      var_ipc_ttl_limit);
	if (kludge)
	    myfree(kludge);
	myfree(relative_path);
    }

    /*
     * Local initialization.
     */
    dict_proxy = (DICT_PROXY *)
	dict_alloc(DICT_TYPE_PROXY, map, sizeof(*dict_proxy));
    dict_proxy->dict.lookup = dict_proxy_lookup;
    dict_proxy->dict.update = dict_proxy_update;
    dict_proxy->dict.delete = dict_proxy_delete;
    dict_proxy->dict.sequence = dict_proxy_sequence;
    dict_proxy->dict.close = dict_proxy_close;
    dict_proxy->inst_flags = (dict_flags & DICT_FLAG_INST_MASK);
    dict_proxy->reskey = vstring_alloc(10);
    dict_proxy->result = vstring_alloc(10);
    dict_proxy->clnt = *pstream;
    dict_proxy->service = service;

    /*
     * Establish initial contact and get the map type specific flags.
     * 
     * XXX Should retrieve flags from local instance.
     */
    for (;;) {
	stream = clnt_stream_access(dict_proxy->clnt);
	errno = 0;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, PROXY_REQ_OPEN,
		       ATTR_TYPE_STR, MAIL_ATTR_TABLE, dict_proxy->dict.name,
		     ATTR_TYPE_INT, MAIL_ATTR_FLAGS, dict_proxy->inst_flags,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &server_flags,
			 ATTR_TYPE_END) != 2) {
	    if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		msg_warn("%s: service %s: %m", VSTREAM_PATH(stream), myname);
	} else {
	    if (msg_verbose)
		msg_info("%s: connect to map=%s status=%d server_flags=%s",
			 myname, dict_proxy->dict.name, status,
			 dict_flags_str(server_flags));
	    switch (status) {
	    case PROXY_STAT_BAD:
		msg_fatal("%s open failed for table \"%s\": invalid request",
			  dict_proxy->service, dict_proxy->dict.name);
	    case PROXY_STAT_DENY:
		msg_fatal("%s service is not configured for table \"%s\"",
			  dict_proxy->service, dict_proxy->dict.name);
	    case PROXY_STAT_OK:
		dict_proxy->dict.flags = (dict_flags & ~DICT_FLAG_IMPL_MASK)
		    | (server_flags & DICT_FLAG_IMPL_MASK);
		return (DICT_DEBUG (&dict_proxy->dict));
	    default:
		msg_warn("%s open failed for table \"%s\": unexpected status %d",
			 dict_proxy->service, dict_proxy->dict.name, status);
	    }
	}
	clnt_stream_recover(dict_proxy->clnt);
	sleep(1);				/* XXX make configurable */
    }
}
