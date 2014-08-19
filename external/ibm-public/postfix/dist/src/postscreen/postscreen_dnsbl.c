/*	$NetBSD: postscreen_dnsbl.c,v 1.1.1.2.10.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	postscreen_dnsbl 3
/* SUMMARY
/*	postscreen DNSBL support
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	psc_dnsbl_init(void)
/*
/*	int	psc_dnsbl_request(client_addr, callback, context)
/*	char	*client_addr;
/*	void	(*callback)(int, char *);
/*	char	*context;
/*
/*	int	psc_dnsbl_retrieve(client_addr, dnsbl_name, dnsbl_index)
/*	char	*client_addr;
/*	const char **dnsbl_name;
/*	int	dnsbl_index;
/* DESCRIPTION
/*	This module implements preliminary support for DNSBL lookups.
/*	Multiple requests for the same information are handled with
/*	reference counts.
/*
/*	psc_dnsbl_init() initializes this module, and must be called
/*	once before any of the other functions in this module.
/*
/*	psc_dnsbl_request() requests a blocklist score for the
/*	specified client IP address and increments the reference
/*	count.  The request completes in the background. The client
/*	IP address must be in inet_ntop(3) output format.  The
/*	callback argument specifies a function that is called when
/*	the requested result is available. The context is passed
/*	on to the callback function. The callback should ignore its
/*	first argument (it exists for compatibility with Postfix
/*	generic event infrastructure).
/*	The result value is the index for the psc_dnsbl_retrieve()
/*	call.
/*
/*	psc_dnsbl_retrieve() retrieves the result score requested with
/*	psc_dnsbl_request() and decrements the reference count. It
/*	is an error to retrieve a score without requesting it first.
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
#include <sys/socket.h>			/* AF_INET */
#include <netinet/in.h>			/* inet_pton() */
#include <arpa/inet.h>			/* inet_pton() */
#include <stdio.h>			/* sscanf */

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <argv.h>
#include <htable.h>
#include <events.h>
#include <vstream.h>
#include <connect.h>
#include <split_at.h>
#include <valid_hostname.h>
#include <ip_match.h>
#include <myaddrinfo.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Talking to the DNSBLOG service.
  */
#define DNSBLOG_TIMEOUT			10
static char *psc_dnsbl_service;

 /*
  * Per-DNSBL filters and weights.
  * 
  * The postscreen_dnsbl_sites parameter specifies zero or more DNSBL domains.
  * We provide multiple access methods, one for quick iteration when sending
  * queries to all DNSBL servers, and one for quick location when receiving a
  * reply from one DNSBL server.
  * 
  * Each DNSBL domain can be specified more than once, each time with a
  * different (filter, weight) pair. We group (filter, weight) pairs in a
  * linked list under their DNSBL domain name. The list head has a reference
  * to a "safe name" for the DNSBL, in case the name includes a password.
  */
static HTABLE *dnsbl_site_cache;	/* indexed by DNSBNL domain */
static HTABLE_INFO **dnsbl_site_list;	/* flattened cache */

typedef struct {
    const char *safe_dnsbl;		/* from postscreen_dnsbl_reply_map */
    struct PSC_DNSBL_SITE *first;	/* list of (filter, weight) tuples */
} PSC_DNSBL_HEAD;

typedef struct PSC_DNSBL_SITE {
    char   *filter;			/* printable filter (default: null) */
    char   *byte_codes;			/* encoded filter (default: null) */
    int     weight;			/* reply weight (default: 1) */
    struct PSC_DNSBL_SITE *next;	/* linked list */
} PSC_DNSBL_SITE;

 /*
  * Per-client DNSBL scores.
  * 
  * Some SMTP clients make parallel connections. This can trigger parallel
  * blocklist score requests when the pre-handshake delays of the connections
  * overlap.
  * 
  * We combine requests for the same score under the client IP address in a
  * single reference-counted entry. The reference count goes up with each
  * request for a score, and it goes down with each score retrieval. Each
  * score has one or more requestors that need to be notified when the result
  * is ready, so that postscreen can terminate a pre-handshake delay when all
  * pre-handshake tests are completed.
  */
static HTABLE *dnsbl_score_cache;	/* indexed by client address */

typedef struct {
    void    (*callback) (int, char *);	/* generic call-back routine */
    char   *context;			/* generic call-back argument */
} PSC_CALL_BACK_ENTRY;

typedef struct {
    const char *dnsbl_name;		/* DNSBL with largest contribution */
    int     dnsbl_weight;		/* weight of largest contribution */
    int     total;			/* combined blocklist score */
    int     refcount;			/* score reference count */
    int     pending_lookups;		/* nr of DNS requests in flight */
    int     request_id;			/* duplicate suppression */
    /* Call-back table support. */
    int     index;			/* next table index */
    int     limit;			/* last valid index */
    PSC_CALL_BACK_ENTRY table[1];	/* actually a bunch */
} PSC_DNSBL_SCORE;

#define PSC_CALL_BACK_INIT(sp) do { \
	(sp)->limit = 0; \
	(sp)->index = 0; \
    } while (0)

#define PSC_CALL_BACK_INDEX_OF_LAST(sp) ((sp)->index - 1)

#define PSC_CALL_BACK_CANCEL(sp, idx) do { \
	PSC_CALL_BACK_ENTRY *_cb_; \
	if ((idx) < 0 || (idx) >= (sp)->index) \
	    msg_panic("%s: index %d must be >= 0 and < %d", \
		      myname, (idx), (sp)->index); \
	_cb_ = (sp)->table + (idx); \
	event_cancel_timer(_cb_->callback, _cb_->context); \
	_cb_->callback = 0; \
	_cb_->context = 0; \
    } while (0)

#define PSC_CALL_BACK_EXTEND(hp, sp) do { \
	if ((sp)->index >= (sp)->limit) { \
	    int _count_ = ((sp)->limit ? (sp)->limit * 2 : 5); \
	    (hp)->value = myrealloc((char *) (sp), sizeof(*(sp)) + \
				    _count_ * sizeof((sp)->table)); \
	    (sp) = (PSC_DNSBL_SCORE *) (hp)->value; \
	    (sp)->limit = _count_; \
	} \
    } while (0)

#define PSC_CALL_BACK_ENTER(sp, fn, ctx) do { \
	PSC_CALL_BACK_ENTRY *_cb_ = (sp)->table + (sp)->index++; \
	_cb_->callback = (fn); \
	_cb_->context = (ctx); \
    } while (0)

#define PSC_CALL_BACK_NOTIFY(sp, ev) do { \
	PSC_CALL_BACK_ENTRY *_cb_; \
	for (_cb_ = (sp)->table; _cb_ < (sp)->table + (sp)->index; _cb_++) \
	    if (_cb_->callback != 0) \
		_cb_->callback((ev), _cb_->context); \
    } while (0)

#define PSC_NULL_EVENT	(0)

 /*
  * Per-request state.
  * 
  * This implementation stores the client IP address and DNSBL domain in the
  * DNSBLOG query/reply stream. This simplifies code, and allows the DNSBLOG
  * server to produce more informative logging.
  */
static VSTRING *reply_client;		/* client address in DNSBLOG reply */
static VSTRING *reply_dnsbl;		/* domain in DNSBLOG reply */
static VSTRING *reply_addr;		/* adress list in DNSBLOG reply */

/* psc_dnsbl_add_site - add DNSBL site information */

static void psc_dnsbl_add_site(const char *site)
{
    const char *myname = "psc_dnsbl_add_site";
    char   *saved_site = mystrdup(site);
    VSTRING *byte_codes = 0;
    PSC_DNSBL_HEAD *head;
    PSC_DNSBL_SITE *new_site;
    char    junk;
    const char *weight_text;
    char   *pattern_text;
    int     weight;
    HTABLE_INFO *ht;
    char   *parse_err;

    /*
     * Parse the required DNSBL domain name, the optional reply filter and
     * the optional reply weight factor.
     */
#define DO_GRIPE	1

    /* Negative weight means whitelist. */
    if ((weight_text = split_at(saved_site, '*')) != 0) {
	if (sscanf(weight_text, "%d%c", &weight, &junk) != 1)
	    msg_fatal("bad DNSBL weight factor \"%s\" in \"%s\"",
		      weight_text, site);
    } else {
	weight = 1;
    }
    /* Reply filter. */
    if ((pattern_text = split_at(saved_site, '=')) != 0) {
	byte_codes = vstring_alloc(100);
	if ((parse_err = ip_match_parse(byte_codes, pattern_text)) != 0)
	    msg_fatal("bad DNSBL filter syntax: %s", parse_err);
    }
    if (valid_hostname(saved_site, DO_GRIPE) == 0)
	msg_fatal("bad DNSBL domain name \"%s\" in \"%s\"",
		  saved_site, site);

    if (msg_verbose > 1)
	msg_info("%s: \"%s\" -> domain=\"%s\" pattern=\"%s\" weight=%d",
		 myname, site, saved_site, pattern_text ? pattern_text :
		 "null", weight);

    /*
     * Look up or create the (filter, weight) list head for this DNSBL domain
     * name.
     */
    if ((head = (PSC_DNSBL_HEAD *)
	 htable_find(dnsbl_site_cache, saved_site)) == 0) {
	head = (PSC_DNSBL_HEAD *) mymalloc(sizeof(*head));
	ht = htable_enter(dnsbl_site_cache, saved_site, (char *) head);
	/* Translate the DNSBL name into a safe name if available. */
	if (psc_dnsbl_reply == 0
	 || (head->safe_dnsbl = dict_get(psc_dnsbl_reply, saved_site)) == 0)
	    head->safe_dnsbl = ht->key;
	if (psc_dnsbl_reply && psc_dnsbl_reply->error)
	    msg_fatal("%s:%s lookup error", psc_dnsbl_reply->type,
		      psc_dnsbl_reply->name);
	head->first = 0;
    }

    /*
     * Append the new (filter, weight) node to the list for this DNSBL domain
     * name.
     */
    new_site = (PSC_DNSBL_SITE *) mymalloc(sizeof(*new_site));
    new_site->filter = (pattern_text ? mystrdup(pattern_text) : 0);
    new_site->byte_codes = (byte_codes ? ip_match_save(byte_codes) : 0);
    new_site->weight = weight;
    new_site->next = head->first;
    head->first = new_site;

    myfree(saved_site);
    if (byte_codes)
	vstring_free(byte_codes);
}

/* psc_dnsbl_match - match DNSBL reply filter */

static int psc_dnsbl_match(const char *filter, ARGV *reply)
{
    char    addr_buf[MAI_HOSTADDR_STRSIZE];
    char  **cpp;

    /*
     * Run the replies through the pattern-matching engine.
     */
    for (cpp = reply->argv; *cpp != 0; cpp++) {
	if (inet_pton(AF_INET, *cpp, addr_buf) != 1)
	    msg_warn("address conversion error for %s -- ignoring this reply",
		     *cpp);
	if (ip_match_execute(filter, addr_buf))
	    return (1);
    }
    return (0);
}

/* psc_dnsbl_retrieve - retrieve blocklist score, decrement reference count */

int     psc_dnsbl_retrieve(const char *client_addr, const char **dnsbl_name,
			           int dnsbl_index)
{
    const char *myname = "psc_dnsbl_retrieve";
    PSC_DNSBL_SCORE *score;
    int     result_score;

    /*
     * Sanity check.
     */
    if ((score = (PSC_DNSBL_SCORE *)
	 htable_find(dnsbl_score_cache, client_addr)) == 0)
	msg_panic("%s: no blocklist score for %s", myname, client_addr);

    /*
     * Disable callbacks.
     */
    PSC_CALL_BACK_CANCEL(score, dnsbl_index);

    /*
     * Reads are destructive.
     */
    result_score = score->total;
    *dnsbl_name = score->dnsbl_name;
    score->refcount -= 1;
    if (score->refcount < 1) {
	if (msg_verbose > 1)
	    msg_info("%s: delete blocklist score for %s", myname, client_addr);
	htable_delete(dnsbl_score_cache, client_addr, myfree);
    }
    return (result_score);
}

/* psc_dnsbl_receive - receive DNSBL reply, update blocklist score */

static void psc_dnsbl_receive(int event, char *context)
{
    const char *myname = "psc_dnsbl_receive";
    VSTREAM *stream = (VSTREAM *) context;
    PSC_DNSBL_SCORE *score;
    PSC_DNSBL_HEAD *head;
    PSC_DNSBL_SITE *site;
    ARGV   *reply_argv;
    int     request_id;

    PSC_CLEAR_EVENT_REQUEST(vstream_fileno(stream), psc_dnsbl_receive, context);

    /*
     * Receive the DNSBL lookup result.
     * 
     * This is preliminary code to explore the field. Later, DNSBL lookup will
     * be handled by an UDP-based DNS client that is built directly into some
     * Postfix daemon.
     * 
     * Don't bother looking up the blocklist score when the client IP address is
     * not listed at the DNSBL.
     * 
     * Don't panic when the blocklist score no longer exists. It may be deleted
     * when the client triggers a "drop" action after pregreet, when the
     * client does not pregreet and the DNSBL reply arrives late, or when the
     * client triggers a "drop" action after hanging up.
     */
    if (event == EVENT_READ
	&& attr_scan(stream,
		     ATTR_FLAG_STRICT,
		     ATTR_TYPE_STR, MAIL_ATTR_RBL_DOMAIN, reply_dnsbl,
		     ATTR_TYPE_STR, MAIL_ATTR_ACT_CLIENT_ADDR, reply_client,
		     ATTR_TYPE_INT, MAIL_ATTR_LABEL, &request_id,
		     ATTR_TYPE_STR, MAIL_ATTR_RBL_ADDR, reply_addr,
		     ATTR_TYPE_END) == 4
	&& (score = (PSC_DNSBL_SCORE *)
	    htable_find(dnsbl_score_cache, STR(reply_client))) != 0
	&& score->request_id == request_id) {

	/*
	 * Run this response past all applicable DNSBL filters and update the
	 * blocklist score for this client IP address.
	 * 
	 * Don't panic when the DNSBL domain name is not found. The DNSBLOG
	 * server may be messed up.
	 */
	if (msg_verbose > 1)
	    msg_info("%s: client=\"%s\" score=%d domain=\"%s\" reply=\"%s\"",
		     myname, STR(reply_client), score->total,
		     STR(reply_dnsbl), STR(reply_addr));
	if (*STR(reply_addr) != 0) {
	    head = (PSC_DNSBL_HEAD *)
		htable_find(dnsbl_site_cache, STR(reply_dnsbl));
	    site = (head ? head->first : (PSC_DNSBL_SITE *) 0);
	    for (reply_argv = 0; site != 0; site = site->next) {
		if (site->byte_codes == 0
		    || psc_dnsbl_match(site->byte_codes, reply_argv ? reply_argv :
			 (reply_argv = argv_split(STR(reply_addr), " ")))) {
		    if (score->dnsbl_name == 0
			|| score->dnsbl_weight < site->weight) {
			score->dnsbl_name = head->safe_dnsbl;
			score->dnsbl_weight = site->weight;
		    }
		    score->total += site->weight;
		    if (msg_verbose > 1)
			msg_info("%s: filter=\"%s\" weight=%d score=%d",
			       myname, site->filter ? site->filter : "null",
				 site->weight, score->total);
		}
	    }
	    if (reply_argv != 0)
		argv_free(reply_argv);
	}

	/*
	 * Notify the requestor(s) that the result is ready to be picked up.
	 * If this call isn't made, clients have to sit out the entire
	 * pre-handshake delay.
	 */
	score->pending_lookups -= 1;
	if (score->pending_lookups == 0)
	    PSC_CALL_BACK_NOTIFY(score, PSC_NULL_EVENT);
    } else if (event == EVENT_TIME) {
	msg_warn("dnsblog reply timeout %ds for %s",
		 DNSBLOG_TIMEOUT, (char *) vstream_context(stream));
    }
    /* Here, score may be a null pointer. */
    vstream_fclose(stream);
}

/* psc_dnsbl_request  - send dnsbl query, increment reference count */

int     psc_dnsbl_request(const char *client_addr,
			          void (*callback) (int, char *),
			          char *context)
{
    const char *myname = "psc_dnsbl_request";
    int     fd;
    VSTREAM *stream;
    HTABLE_INFO **ht;
    PSC_DNSBL_SCORE *score;
    HTABLE_INFO *hash_node;
    static int request_count;

    /*
     * Some spambots make several connections at nearly the same time,
     * causing their pregreet delays to overlap. Such connections can share
     * the efforts of DNSBL lookup.
     * 
     * We store a reference-counted DNSBL score under its client IP address. We
     * increment the reference count with each score request, and decrement
     * the reference count with each score retrieval.
     * 
     * Do not notify the requestor NOW when the DNS replies are already in.
     * Reason: we must not make a backwards call while we are still in the
     * middle of executing the corresponding forward call. Instead we create
     * a zero-delay timer request and call the notification function from
     * there.
     * 
     * psc_dnsbl_request() could instead return a result value to indicate that
     * the DNSBL score is already available, but that would complicate the
     * caller with two different notification code paths: one asynchronous
     * code path via the callback invocation, and one synchronous code path
     * via the psc_dnsbl_request() result value. That would be a source of
     * future bugs.
     */
    if ((hash_node = htable_locate(dnsbl_score_cache, client_addr)) != 0) {
	score = (PSC_DNSBL_SCORE *) hash_node->value;
	score->refcount += 1;
	PSC_CALL_BACK_EXTEND(hash_node, score);
	PSC_CALL_BACK_ENTER(score, callback, context);
	if (msg_verbose > 1)
	    msg_info("%s: reuse blocklist score for %s refcount=%d pending=%d",
		     myname, client_addr, score->refcount,
		     score->pending_lookups);
	if (score->pending_lookups == 0)
	    event_request_timer(callback, context, EVENT_NULL_DELAY);
	return (PSC_CALL_BACK_INDEX_OF_LAST(score));
    }
    if (msg_verbose > 1)
	msg_info("%s: create blocklist score for %s", myname, client_addr);
    score = (PSC_DNSBL_SCORE *) mymalloc(sizeof(*score));
    score->request_id = request_count++;
    score->dnsbl_name = 0;
    score->dnsbl_weight = 0;
    score->total = 0;
    score->refcount = 1;
    score->pending_lookups = 0;
    PSC_CALL_BACK_INIT(score);
    PSC_CALL_BACK_ENTER(score, callback, context);
    (void) htable_enter(dnsbl_score_cache, client_addr, (char *) score);

    /*
     * Send a query to all DNSBL servers. Later, DNSBL lookup will be done
     * with an UDP-based DNS client that is built directly into Postfix code.
     * We therefore do not optimize the maximum out of this temporary
     * implementation.
     */
    for (ht = dnsbl_site_list; *ht; ht++) {
	if ((fd = LOCAL_CONNECT(psc_dnsbl_service, NON_BLOCKING, 1)) < 0) {
	    msg_warn("%s: connect to %s service: %m",
		     myname, psc_dnsbl_service);
	    continue;
	}
	stream = vstream_fdopen(fd, O_RDWR);
	vstream_control(stream,
			VSTREAM_CTL_CONTEXT, ht[0]->key,
			VSTREAM_CTL_END);
	attr_print(stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_STR, MAIL_ATTR_RBL_DOMAIN, ht[0]->key,
		   ATTR_TYPE_STR, MAIL_ATTR_ACT_CLIENT_ADDR, client_addr,
		   ATTR_TYPE_INT, MAIL_ATTR_LABEL, score->request_id,
		   ATTR_TYPE_END);
	if (vstream_fflush(stream) != 0) {
	    msg_warn("%s: error sending to %s service: %m",
		     myname, psc_dnsbl_service);
	    vstream_fclose(stream);
	    continue;
	}
	PSC_READ_EVENT_REQUEST(vstream_fileno(stream), psc_dnsbl_receive,
			       (char *) stream, DNSBLOG_TIMEOUT);
	score->pending_lookups += 1;
    }
    return (PSC_CALL_BACK_INDEX_OF_LAST(score));
}

/* psc_dnsbl_init - initialize */

void    psc_dnsbl_init(void)
{
    const char *myname = "psc_dnsbl_init";
    ARGV   *dnsbl_site = argv_split(var_psc_dnsbl_sites, ", \t\r\n");
    char  **cpp;

    /*
     * Sanity check.
     */
    if (dnsbl_site_cache != 0)
	msg_panic("%s: called more than once", myname);

    /*
     * pre-compute the DNSBLOG socket name.
     */
    psc_dnsbl_service = concatenate(MAIL_CLASS_PRIVATE, "/",
				    var_dnsblog_service, (char *) 0);

    /*
     * Prepare for quick iteration when sending out queries to all DNSBL
     * servers, and for quick lookup when a reply arrives from a specific
     * DNSBL server.
     */
    dnsbl_site_cache = htable_create(13);
    for (cpp = dnsbl_site->argv; *cpp; cpp++)
	psc_dnsbl_add_site(*cpp);
    argv_free(dnsbl_site);
    dnsbl_site_list = htable_list(dnsbl_site_cache);

    /*
     * The per-client blocklist score.
     */
    dnsbl_score_cache = htable_create(13);

    /*
     * Space for ad-hoc DNSBLOG server request/reply parameters.
     */
    reply_client = vstring_alloc(100);
    reply_dnsbl = vstring_alloc(100);
    reply_addr = vstring_alloc(100);
}
