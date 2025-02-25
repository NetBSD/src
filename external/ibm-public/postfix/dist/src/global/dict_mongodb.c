/*	$NetBSD: dict_mongodb.c,v 1.2 2025/02/25 19:15:45 christos Exp $	*/

/*++
/* NAME
/*	dict_mongodb 3
/* SUMMARY
/*	dictionary interface to mongodb, compatible with libmongoc-1.0
/* SYNOPSIS
/*	#include <dict_mongodb.h>
/*
/*	DICT *dict_mongodb_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_mongodb_open() opens a MongoDB database, providing a
/*	dictionary interface for Postfix mappings. The result is a
/*	pointer to the installed dictionary.
/*
/*	Configuration parameters are described in mongodb_table(5).
/*
/*	Arguments:
/* .IP name
/*	Either the path to the MongoDB configuration file (if it
/*	starts with '/' or '.'), or the prefix which will be used
/*	to obtain main.cf configuration parameters for this search.
/*
/*	In the first case, configuration parameters are specified
/*	in the file as \fIname\fR=\fIvalue\fR pairs.
/*
/*	In the second case, the configuration parameters are prefixed
/*	with the value of \fIname\fR and an underscore, and they
/*	are specified in main.cf. For example, if this value is
/*	\fImongodbconf\fR, the parameters would look like
/*	\fImongodbconf_uri\fR, \fImongodbconf_collection\fR, and
/*	so on.
/* .IP open_flags
/*	Must be O_RDONLY
/* .IP dict_flags
/*	See dict_open(3).
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* HISTORY
/* .ad
/* .fi
/*	MongoDB support was added in Postfix 3.9.
/* AUTHOR(S)
/*	Hamid Maadani (hamid@dexo.tech)
/*	Dextrous Technologies, LLC
/*
/*	Edited by:
/*	Wietse Venema
/*	porcupine.org
/*
/*	Based on prior work by:
/*	Stephan Ferraro
/*	Aionda GmbH
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#ifdef HAS_MONGODB
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>			/* C99 PRId64 */

#include <bson/bson.h>
#include <mongoc/mongoc.h>

 /*
  * Utility library.
  */
#include <dict.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <auto_clnt.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <cfg_parser.h>
#include <db_common.h>

 /*
  * Application-specific.
  */
#include <dict_mongodb.h>

 /*
  * Initial size for dynamically-allocated buffers.
  */
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#define INIT_VSTR(buf, len) do { \
	if (buf == 0) \
		buf = vstring_alloc(len); \
	VSTRING_RESET(buf); \
	VSTRING_TERMINATE(buf); \
    } while (0)

/* Structure of one mongodb dictionary handle. */
typedef struct {
    /* Initialized by dict_mongodb_open(). */
    DICT    dict;			/* Parent class */
    CFG_PARSER *parser;			/* Configuration file parser */
    mongoc_client_t *client;		/* Mongo C client handle */
    /* Initialized by mongodb_parse_config(). */
    char   *uri;			/* mongodb+srv:/*localhost:27017 */
    char   *dbname;			/* Database name */
    char   *collection;			/* Collection name */
    char   *query_filter;		/* db_common_expand() query template */
    char   *projection;			/* Advanced MongoDB projection */
    char   *result_attribute;		/* The key(s) to return the data for */
    char   *result_format;		/* db_common_expand() result_template */
    int     expansion_limit;		/* Result expansion limit */
    void   *ctx;			/* db_common handle */
} DICT_MONGODB;

/* Per-process initialization. */
static bool init_done = false;

/* itoa - int64_t to string */

static char *itoa(int64_t val)
{
    static char buf[21] = {0};
    int     ret;

    /*
     * XXX(Wietse) replaced custom code with standard library calls that
     * handle zero, and negative values.
     */
#define PRId64_FORMAT "%" PRId64

    ret = snprintf(buf, sizeof(buf), PRId64_FORMAT, val);
    if (ret < 0)
	msg_panic("itoa: output error for '%s'", PRId64_FORMAT);
    if (ret >= sizeof(buf))
	msg_panic("itoa: output for '%s' exceeds space %ld",
		  PRId64_FORMAT, sizeof(buf));
    return (buf);
}

/* mongodb_parse_config - parse mongodb configuration file */

static void mongodb_parse_config(DICT_MONGODB *dict_mongodb,
				         const char *mongodbcf)
{
    CFG_PARSER *p = dict_mongodb->parser;

    /*
     * Parse the configuration file.
     */
    dict_mongodb->uri = cfg_get_str(p, "uri", NULL, 1, 0);
    dict_mongodb->dbname = cfg_get_str(p, "dbname", NULL, 1, 0);
    dict_mongodb->collection = cfg_get_str(p, "collection", NULL, 1, 0);
    dict_mongodb->query_filter = cfg_get_str(p, "query_filter", NULL, 1, 0);

    /*
     * One of projection and result_attribute must be specified. That is
     * enforced in the caller.
     */
    dict_mongodb->projection = cfg_get_str(p, "projection", NULL, 0, 0);
    dict_mongodb->result_attribute
	= cfg_get_str(p, "result_attribute", NULL, 0, 0);
    dict_mongodb->result_format
	= cfg_get_str(dict_mongodb->parser, "result_format", "%s", 1, 0);
    dict_mongodb->expansion_limit
	= cfg_get_int(dict_mongodb->parser, "expansion_limit", 10, 0, 100);

    /*
     * db_common query parsing and domain pattern lookup.
     */
    dict_mongodb->ctx = 0;
    (void) db_common_parse(&dict_mongodb->dict, &dict_mongodb->ctx,
			   dict_mongodb->query_filter, 1);
    db_common_parse_domain(dict_mongodb->parser, dict_mongodb->ctx);
}

/* expand_value - expand lookup result value */

static bool expand_value(DICT_MONGODB *dict_mongodb, const char *p,
			         const char *lookup_name,
			         VSTRING *resultString,
			         int *expansion, const char *key)
{

    /*
     * If a lookup result cannot be processed due to an expansion limit
     * error, return a DICT_ERR_RETRY error code and a 'false' result value.
     * As documented for many dict_xxx() implementations, and expansion limit
     * error is considered a temporary error.
     */
    if (dict_mongodb->expansion_limit > 0
	&& ++(*expansion) > dict_mongodb->expansion_limit) {
	msg_warn("%s:%s: expansion limit exceeded for key: '%s'",
		 dict_mongodb->dict.type, dict_mongodb->dict.name, key);
	dict_mongodb->dict.error = DICT_ERR_RETRY;
	return (false);
    }

    /*
     * XXX(Wietse) Added the dict_mongodb_lookup() lookup_name argument,
     * because it selects code paths inside db_common_expand() that are
     * specifically for lookup results instead of lookup keys, including
     * %[SUD] substitution.
     */
    db_common_expand(dict_mongodb->ctx, dict_mongodb->result_format, p,
		     lookup_name, resultString, 0);
    return (true);
}

/* get_result_string - convert lookup result to string, or set dict.error */

static char *get_result_string(DICT_MONGODB *dict_mongodb,
			               VSTRING *resultString,
			               bson_iter_t *iter,
			               const char *lookup_name,
			               int *expansion,
			               const char *key)
{
    char   *p = NULL;
    bool    got_one_result = false;

    /*
     * If a lookup result cannot be processed due to an error, return a
     * non-zero error code and a NULL result value.
     */
    INIT_VSTR(resultString, BUFFER_SIZE);
    while (dict_mongodb->dict.error == DICT_ERR_NONE && bson_iter_next(iter)) {
	switch (bson_iter_type(iter)) {
	case BSON_TYPE_UTF8:
	    p = (char *) bson_iter_utf8(iter, NULL);
	    if (!bson_utf8_validate(p, strlen(p), true)) {
		msg_warn("%s:%s: invalid UTF-8 in lookup result '%s'",
		       dict_mongodb->dict.type, dict_mongodb->dict.name, p);
		dict_mongodb->dict.error = DICT_ERR_RETRY;
		break;
	    }
	    got_one_result |= expand_value(dict_mongodb, p, lookup_name,
					   resultString, expansion, key);
	    break;
	case BSON_TYPE_INT64:
	case BSON_TYPE_INT32:
	    p = itoa(bson_iter_as_int64(iter));
	    got_one_result |= expand_value(dict_mongodb, p, lookup_name,
					   resultString, expansion, key);
	    break;
	case BSON_TYPE_ARRAY:
	    ;					/* For pre-C23 Clang. */
	    const uint8_t *dataBuffer = NULL;
	    unsigned int len = 0;
	    bson_iter_t dataIter;
	    bson_t *data = NULL;

	    /*
	     * XXX(Wietse) are there any non-error cases, such as a valid but
	     * empty array, where bson_new_from_data() or bson_iter_init()
	     * would return null or false? If there are no such cases then we
	     * must handle null/false as an error.
	     */
	    bson_iter_array(iter, &len, &dataBuffer);
	    if ((data = bson_new_from_data(dataBuffer, len)) != 0
		&& bson_iter_init(&dataIter, data)) {
		VSTRING *iterResult = vstring_alloc(BUFFER_SIZE);

		if ((p = get_result_string(dict_mongodb, iterResult, &dataIter,
				       lookup_name, expansion, key)) != 0) {
		    vstring_sprintf_append(resultString, (got_one_result) ?
					   ",%s" : "%s", p);
		    got_one_result |= true;
		}
		vstring_free(iterResult);
	    }
	    bson_destroy(data);
	    break;
	default:
	    /* Unexpected field type. As documented, warn and ignore. */
	    msg_warn("%s:%s: failed to retrieve value of '%s', "
		     "Unknown result type %d.", dict_mongodb->dict.type,
		     dict_mongodb->dict.name, bson_iter_key(iter),
		     bson_iter_type(iter));
	    break;
	}
    }
    if (dict_mongodb->dict.error != DICT_ERR_NONE || !got_one_result)
	return (0);
    return (vstring_str(resultString));
}

/* dict_mongdb_quote - quote json string */

static void dict_mongdb_quote(DICT *dict, const char *name, VSTRING *result)
{
    /* quote_for_json_append() will resize the result buffer as needed. */
    (void) quote_for_json_append(result, name, -1);
}

/* dict_mongdb_append_result_attributes - projection builder */

static int dict_mongdb_append_result_attribute(bson_t * projection,
				               const char *result_attribute)
{
    char   *ra = mystrdup(result_attribute);
    char   *pp = ra;
    char   *cp;
    int     ok = 1;

    while (ok && (cp = mystrtok(&pp, CHARS_COMMA_SP)) != 0)
	ok = BSON_APPEND_INT32(projection, cp, 1);
    myfree(ra);
    return (ok);
}

/* dict_mongodb_lookup - find database entry using mongo query language */

static const char *dict_mongodb_lookup(DICT *dict, const char *name)
{
    DICT_MONGODB *dict_mongodb = (DICT_MONGODB *) dict;
    mongoc_collection_t *coll = NULL;
    mongoc_cursor_t *cursor = NULL;
    bson_iter_t iter;
    const bson_t *doc = NULL;
    bson_t *query = NULL;
    bson_t *options = NULL;
    bson_t *projection = NULL;
    bson_error_t error;
    char   *result = NULL;
    static VSTRING *queryString = NULL;
    static VSTRING *resultString = NULL;
    int     domain_rc;
    int     expansion = 0;

    dict_mongodb->dict.error = DICT_ERR_NONE;

    /*
     * If they specified a domain list for this map, then only search for
     * addresses in domains on the list. This can significantly reduce the
     * load on the database.
     */
    if ((domain_rc = db_common_check_domain(dict_mongodb->ctx, name)) == 0) {
	if (msg_verbose)
	    msg_info("%s:%s: skipping lookup of '%s': domain mismatch",
		     dict_mongodb->dict.type, dict_mongodb->dict.name, name);
	return (0);
    } else if (domain_rc < 0) {
	DICT_ERR_VAL_RETURN(dict, domain_rc, (char *) 0);
    }

    /*
     * Ugly macros to make error and non-error handling code more readable.
     * If code size is a concern, them an optimizing compiler can eliminate
     * dead code or duplicated code.
     */

    /* Set an error code, and return null. */
#define DICT_MONGODB_LOOKUP_ERR_RETURN(err) do { \
	dict_mongodb->dict.error = (err); \
	DICT_MONGODB_LOOKUP_RETURN((char *) 0); \
} while (0);

    /* Pass through any error, and return the specified value. */
#define DICT_MONGODB_LOOKUP_RETURN(val) do { \
	if (coll) mongoc_collection_destroy(coll); \
	if (cursor) mongoc_cursor_destroy(cursor); \
	if (query) bson_destroy(query); \
	if (options) bson_destroy(options); \
	if (projection) bson_destroy(projection); \
	return (val); \
    } while (0)

    coll = mongoc_client_get_collection(dict_mongodb->client,
					dict_mongodb->dbname,
					dict_mongodb->collection);
    if (!coll) {
	msg_warn("%s:%s: failed to get collection [%s] from [%s]",
		 dict_mongodb->dict.type, dict_mongodb->dict.name,
		 dict_mongodb->collection, dict_mongodb->dbname);
	DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
    }

    /*
     * Use the specified result projection, or craft one from the
     * result_attribute. Exclude the _id field from the result.
     */
    options = bson_new();
    if (dict_mongodb->projection) {
	projection = bson_new_from_json((uint8_t *) dict_mongodb->projection,
					-1, &error);
	if (!projection) {
	    msg_warn("%s:%s: failed to create a projection from '%s': %s",
		     dict_mongodb->dict.type, dict_mongodb->dict.name,
		     dict_mongodb->projection, error.message);
	    DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
	}
	if (!BSON_APPEND_INT32(projection, "_id", 0)
	    || !BSON_APPEND_DOCUMENT(options, "projection", projection)) {
	    msg_warn("%s:%s: failed to append a projection from '%s'",
		     dict_mongodb->dict.type, dict_mongodb->dict.name,
		     dict_mongodb->projection);
	    DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
	}
    } else if (dict_mongodb->result_attribute) {
	bson_t  res_attr;

	if (!BSON_APPEND_DOCUMENT_BEGIN(options, "projection", &res_attr)
	    || !BSON_APPEND_INT32(&res_attr, "_id", 0)
	    || !dict_mongdb_append_result_attribute(&res_attr,
					     dict_mongodb->result_attribute)
	    || !bson_append_document_end(options, &res_attr)) {
	    msg_warn("%s:%s: failed to append a projection from '%s'",
		     dict_mongodb->dict.type, dict_mongodb->dict.name,
		     dict_mongodb->result_attribute);
	    DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
	}
    } else {
	/* Can't happen. The configuration parser should reject this. */
	msg_panic("%s:%s: empty 'projection' and 'result_attribute'",
		  dict_mongodb->dict.type, dict_mongodb->dict.name);
    }

    /*
     * Expand filter template. This uses a quoting function to prevent
     * metacharacter injection with parts from a crafted email address.
     */
    INIT_VSTR(queryString, BUFFER_SIZE);
    if (!db_common_expand(dict_mongodb->ctx, dict_mongodb->query_filter,
			  name, 0, queryString, dict_mongdb_quote))
	/* Suppress the actual lookup if the expansion is empty. */
	DICT_MONGODB_LOOKUP_RETURN(0);

    /* Create the query from the expanded query template. */
    query = bson_new_from_json((uint8_t *) vstring_str(queryString),
			       -1, &error);
    if (!query) {
	msg_warn("%s:%s: failed to create a query from '%s': %s",
		 dict_mongodb->dict.type, dict_mongodb->dict.name,
		 vstring_str(queryString), error.message);
	DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
    }
    /* Run the query. */
    cursor = mongoc_collection_find_with_opts(coll, query, options, NULL);
    if (mongoc_cursor_error(cursor, &error)) {
	msg_warn("%s:%s: cursor error for '%s': %s",
		 dict_mongodb->dict.type, dict_mongodb->dict.name,
		 vstring_str(queryString), error.message);
	DICT_MONGODB_LOOKUP_ERR_RETURN(DICT_ERR_RETRY);
    }
    /* Convert the lookup result to C string. */
    INIT_VSTR(resultString, BUFFER_SIZE);
    while (mongoc_cursor_next(cursor, &doc)) {
	if (bson_iter_init(&iter, doc)) {
	    result = get_result_string(dict_mongodb, resultString, &iter,
				       name, &expansion, name);
	}
    }
    DICT_MONGODB_LOOKUP_RETURN(result);
}

/* dict_mongodb_close - close MongoDB database */

static void dict_mongodb_close(DICT *dict)
{
    DICT_MONGODB *dict_mongodb = (DICT_MONGODB *) dict;

    cfg_parser_free(dict_mongodb->parser);
    if (dict_mongodb->ctx) {
	db_common_free_ctx(dict_mongodb->ctx);
    }
    myfree(dict_mongodb->uri);
    myfree(dict_mongodb->dbname);
    myfree(dict_mongodb->collection);
    myfree(dict_mongodb->query_filter);

    if (dict_mongodb->result_attribute) {
	myfree(dict_mongodb->result_attribute);
    }
    if (dict_mongodb->result_format) {
	myfree(dict_mongodb->result_format);
    }
    if (dict_mongodb->projection) {
	myfree(dict_mongodb->projection);
    }
    if (dict_mongodb->client) {
	mongoc_client_destroy(dict_mongodb->client);
    }
    dict_free(dict);
}

/* dict_mongodb_open - open MongoDB database connection */

DICT   *dict_mongodb_open(const char *name, int open_flags, int dict_flags)
{
    DICT_MONGODB *dict_mongodb;
    CFG_PARSER *parser;
    mongoc_uri_t *uri = 0;
    bson_error_t error;

    /* Sanity checks. */
    if (open_flags != O_RDONLY) {
	return (dict_surrogate(DICT_TYPE_MONGODB, name, open_flags, dict_flags,
			       "%s:%s: map requires O_RDONLY access mode",
			       DICT_TYPE_MONGODB, name));
    }
    /* Open the configuration file. */
    if ((parser = cfg_parser_alloc(name)) == 0) {
	return (dict_surrogate(DICT_TYPE_MONGODB, name, open_flags, dict_flags,
			       "open %s: %m", name));
    }
    /* Create the dictionary object. */
    dict_mongodb = (DICT_MONGODB *) dict_alloc(DICT_TYPE_MONGODB, name,
					       sizeof(*dict_mongodb));
    dict_mongodb->dict.lookup = dict_mongodb_lookup;
    dict_mongodb->dict.close = dict_mongodb_close;
    dict_mongodb->dict.flags = dict_flags;
    dict_mongodb->parser = parser;
    dict_mongodb->dict.owner = cfg_get_owner(dict_mongodb->parser);
    dict_mongodb->client = NULL;

    /* Parse config. */
    mongodb_parse_config(dict_mongodb, name);
    if (!dict_mongodb->projection == !dict_mongodb->result_attribute) {
	dict_mongodb_close(&dict_mongodb->dict);
	return (dict_surrogate(DICT_TYPE_MONGODB, name, open_flags, dict_flags,
	 "%s:%s: specify exactly one of 'projection' or 'result_attribute'",
			       DICT_TYPE_MONGODB, name));
    }
    /* One-time initialization of libmongoc 's internals. */
    if (!init_done) {
	mongoc_init();
	init_done = true;
    }
#define DICT_MONGODB_OPEN_ERR_RETURN(d) do { \
	DICT   *_d = (d); \
	if (uri) mongoc_uri_destroy(uri); \
	dict_mongodb_close(&dict_mongodb->dict); \
	return (_d); \
    } while (0);

    uri = mongoc_uri_new_with_error(dict_mongodb->uri, &error);
    if (!uri)
	DICT_MONGODB_OPEN_ERR_RETURN(dict_surrogate(DICT_TYPE_MONGODB, name,
						    open_flags, dict_flags,
				      "%s:%s: failed to parse URI '%s': %s",
						    DICT_TYPE_MONGODB, name,
					 dict_mongodb->uri, error.message));

    dict_mongodb->client = mongoc_client_new_from_uri_with_error(uri, &error);
    if (!dict_mongodb->client)
	DICT_MONGODB_OPEN_ERR_RETURN(dict_surrogate(DICT_TYPE_MONGODB, name,
						    open_flags, dict_flags,
			      "%s:%s: failed to create client for '%s': %s",
						    DICT_TYPE_MONGODB, name,
						    dict_mongodb->uri,
						    error.message));

    mongoc_uri_destroy(uri);
    mongoc_client_set_error_api(dict_mongodb->client, MONGOC_ERROR_API_VERSION_2);
    return (DICT_DEBUG (&dict_mongodb->dict));
}

#endif
