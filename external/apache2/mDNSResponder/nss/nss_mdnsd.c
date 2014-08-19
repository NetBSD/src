/*	$NetBSD: nss_mdnsd.c,v 1.3.12.1 2014/08/19 23:45:51 tls Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Multicast DNS ("Bonjour") hosts name service switch
 *
 * Documentation links:
 *
 * http://developer.apple.com/bonjour/
 * http://www.multicastdns.org/
 * http://www.dns-sd.org/
 */

#include <errno.h>
#include <nsswitch.h>
#include <stdarg.h>  
#include <stdlib.h>  
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <dns_sd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#include "hostent.h"

/*
 * Pool of mdnsd connections
 */
static SLIST_HEAD(, svc_ref) conn_list = LIST_HEAD_INITIALIZER(&conn_list);
static unsigned int conn_count = 0;
static pid_t my_pid;
struct timespec last_config;

typedef struct svc_ref {
    SLIST_ENTRY(svc_ref)    entries;
    DNSServiceRef           sdRef;
    unsigned int            uses;
} svc_ref;

/*
 * There is a large class of programs that do a few lookups at startup
 * and then never again (ping, telnet, etc). Keeping a persistent connection
 * for these would be a waste, so there is a kind of slow start mechanism.
 * The first SLOWSTART_LOOKUPS times, dispose of the connection after use.
 * After that we assume the program is a serious consumer of host lookup
 * services and start keeping connections.
 */
#define SLOWSTART_LOOKUPS 5
static unsigned int svc_puts = 0;

/*
 * Age out connections. Free connection instead of putting on the list
 * if used more than REUSE_TIMES and there are others on the list.
 */
#define REUSE_TIMES          32
 
/* protects above data */
static pthread_mutex_t conn_list_lock = PTHREAD_MUTEX_INITIALIZER;

extern int __isthreaded; /* libc private -- wish there was a better way */

#define LOCK(x) do { if (__isthreaded) pthread_mutex_lock(x); } while (0)
#define UNLOCK(x) do { if (__isthreaded) pthread_mutex_unlock(x); } while (0)


#ifndef lint
#define UNUSED(a)       (void)&a
#else
#define UNUSED(a)       a = a
#endif

#define MAXALIASES      35 
#define MAXADDRS        35

typedef struct callback_ctx {
    bool done;
} callback_ctx;

typedef struct hostent_ctx {
    callback_ctx cb_ctx;    /* must come first */
    struct hostent host;
    char *h_addr_ptrs[MAXADDRS + 1];
    char *host_aliases[MAXALIASES];
    char addrs[MAXADDRS * 16];
    char buf[8192], *next;
    int naliases, naddrs;
} hostent_ctx;

typedef struct addrinfo_ctx {
    callback_ctx cb_ctx;    /* must come first */
    struct addrinfo start, *last;
} addrinfo_ctx;

#define HCTX_BUFLEFT(c) (sizeof((c)->buf) - ((c)->next - (c)->buf))

typedef struct res_conf {
    unsigned int            refcount;
    char                  **search_domains;
    char                  **no_search;
    short                   ndots;
    short                   timeout;
} res_conf;

static res_conf *cur_res_conf;

/* protects above data */
static pthread_mutex_t res_conf_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct search_iter {
    res_conf       *conf;
    const char     *name;
    char          **next_search;    
    size_t          baselen;
    bool            abs_first;
    bool            abs_last;
    char            buf[MAXHOSTNAMELEN];
} search_iter;

static DNSServiceFlags svc_flags = 0;

ns_mtab *nss_module_register(const char *, u_int *, nss_module_unregister_fn *);
static int load_config(res_state);

static int _mdns_getaddrinfo(void *, void *, va_list);
static int _mdns_gethtbyaddr(void *, void *, va_list);
static int _mdns_gethtbyname(void *, void *, va_list);

static int _mdns_getaddrinfo_abs(const char *, DNSServiceProtocol,
    svc_ref **, addrinfo_ctx *, short);
static void _mdns_addrinfo_init(addrinfo_ctx *, const struct addrinfo *);
static void _mdns_addrinfo_add_ai(addrinfo_ctx *, struct addrinfo *);
static struct addrinfo *_mdns_addrinfo_done(addrinfo_ctx *);

static int _mdns_gethtbyname_abs(struct getnamaddr *, struct hostent_ctx *,
    const char *, int, svc_ref **, short);
static void _mdns_hostent_init(hostent_ctx *, int, int);
static void _mdns_hostent_add_host(hostent_ctx *, const char *);
static void _mdns_hostent_add_addr(hostent_ctx *, const void *, uint16_t);
static int _mdns_hostent_done(struct getnamaddr *, hostent_ctx *);

static void _mdns_addrinfo_cb(DNSServiceRef, DNSServiceFlags,
    uint32_t, DNSServiceErrorType, const char *, const struct sockaddr *,
    uint32_t, void *);
static void _mdns_hostent_cb(DNSServiceRef, DNSServiceFlags,
    uint32_t, DNSServiceErrorType, const char *, uint16_t, uint16_t, uint16_t,
    const void *, uint32_t, void *);
static void _mdns_eventloop(svc_ref *, callback_ctx *, short);

static char *_mdns_rdata2name(const unsigned char *, uint16_t,
    char *, size_t);

static int search_init(search_iter *, const char *, const char **);
static void search_done(search_iter *);
static const char *search_next(search_iter *);
static bool searchable_domain(char *);

static void destroy_svc_ref(svc_ref *);
static svc_ref *get_svc_ref(void);
static void put_svc_ref(svc_ref *);
static bool retry_query(svc_ref **, DNSServiceErrorType);

static void decref_res_conf(res_conf *);
static res_conf *get_res_conf(void);
static void put_res_conf(res_conf *);
static res_conf *new_res_conf(res_state);
static short get_timeout(void);

static ns_mtab mtab[] = {
    { NSDB_HOSTS, "getaddrinfo",    _mdns_getaddrinfo, NULL },
    { NSDB_HOSTS, "gethostbyaddr",  _mdns_gethtbyaddr, NULL },
    { NSDB_HOSTS, "gethostbyname",  _mdns_gethtbyname, NULL },
};



ns_mtab *
nss_module_register(const char *source, u_int *nelems,
                    nss_module_unregister_fn *unreg)
{
    *nelems = sizeof(mtab) / sizeof(mtab[0]);
    *unreg = NULL;

    my_pid = getpid();

    if (!strcmp(source, "multicast_dns")) {
        svc_flags = kDNSServiceFlagsForceMulticast;
    }

    return mtab;
}



static int
_mdns_getaddrinfo(void *cbrv, void *cbdata, va_list ap)
{
    const struct addrinfo *pai;
    const char *name, *sname;
    DNSServiceProtocol proto;
    DNSServiceRef sdRef;
    addrinfo_ctx ctx;
    search_iter iter;
    res_conf *rc;
    svc_ref *sr;
    int err;
    
    UNUSED(cbdata);
    
    name = va_arg(ap, char *);
    pai = va_arg(ap, struct addrinfo *);
    
    switch (pai->ai_family) {
    case AF_UNSPEC:
        proto = kDNSServiceProtocol_IPv6 | kDNSServiceProtocol_IPv4;
        break;

    case AF_INET6:
        proto = kDNSServiceProtocol_IPv6;
        break;

    case AF_INET:
        proto = kDNSServiceProtocol_IPv4;
        break;
        
    default:
        h_errno = NO_RECOVERY;
        return NS_UNAVAIL;
    }

    sr = get_svc_ref();
    if (!sr) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    if ((err = search_init(&iter, name, &sname)) != NS_SUCCESS) {
        put_svc_ref(sr);
        return err;
    }
    
    _mdns_addrinfo_init(&ctx, pai);
    
    err = NS_NOTFOUND;
    while (sr && sname && (err != NS_SUCCESS)) {
        err = _mdns_getaddrinfo_abs(sname, proto, &sr, &ctx, iter.conf->timeout);
        if (err != NS_SUCCESS) {
            sname = search_next(&iter);
        }
    };

    search_done(&iter);
    put_svc_ref(sr);

    if (err == NS_SUCCESS) {
        *(struct addrinfo **)cbrv = _mdns_addrinfo_done(&ctx);
    }
    
    return err;
}



static int
_mdns_getaddrinfo_abs(const char *name, DNSServiceProtocol proto,
    svc_ref **sr, addrinfo_ctx *ctx, short timeout)
{
    DNSServiceErrorType err = kDNSServiceErr_ServiceNotRunning;
    DNSServiceRef sdRef;
    bool retry = true;
    
    while (*sr && retry) {
        /* We must always use a copy of the ref when using a shared
           connection, per kDNSServiceFlagsShareConnection docs */

        sdRef = (*sr)->sdRef;

        err = DNSServiceGetAddrInfo(
            &sdRef,
            svc_flags
                | kDNSServiceFlagsShareConnection
                | kDNSServiceFlagsReturnIntermediates,
            kDNSServiceInterfaceIndexAny,
            proto,
            name,
            _mdns_addrinfo_cb,
            ctx
        );
        
        retry = retry_query(sr, err);
    }

    if (err) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_eventloop(*sr, (void *)ctx, timeout);

    DNSServiceRefDeallocate(sdRef);

    if (ctx->start.ai_next) {
        return NS_SUCCESS;
    } else {
        h_errno = HOST_NOT_FOUND;
        return NS_NOTFOUND;
    }
}



static int
_mdns_gethtbyaddr(void *cbrv, void *cbdata, va_list ap)
{
    const unsigned char *addr;
    int addrlen, af;
    char qbuf[NS_MAXDNAME + 1], *qp, *ep;
    int advance, n;
    DNSServiceErrorType err;
    DNSServiceRef sdRef;
    svc_ref *sr;
    bool retry = true;
    struct getnamaddr *info = cbrv;

    UNUSED(cbdata);
    
    addr = va_arg(ap, unsigned char *);
    addrlen = va_arg(ap, int);
    af = va_arg(ap, int);

    switch (af) {
    case AF_INET:
        /* if mcast-only don't bother for non-LinkLocal addrs) */
        if (svc_flags & kDNSServiceFlagsForceMulticast) {
            if ((addr[0] != 169) || (addr[1] != 254)) {
                *info->he = HOST_NOT_FOUND;
                return NS_NOTFOUND;
            }
        }

        (void)snprintf(qbuf, sizeof(qbuf), "%u.%u.%u.%u.in-addr.arpa",
            (addr[3] & 0xff), (addr[2] & 0xff),
            (addr[1] & 0xff), (addr[0] & 0xff));
        break;
    
    case AF_INET6:
        /* if mcast-only don't bother for non-LinkLocal addrs) */
        if (svc_flags & kDNSServiceFlagsForceMulticast) {
            if ((addr[0] != 0xfe) || ((addr[1] & 0xc0) != 0x80)) {
                *info->he = HOST_NOT_FOUND;
                return NS_NOTFOUND;
            }
        }
        
        qp = qbuf;
        ep = qbuf + sizeof(qbuf) - 1;
        for (n = IN6ADDRSZ - 1; n >= 0; n--) {
            advance = snprintf(qp, (size_t)(ep - qp), "%x.%x.",
                addr[n] & 0xf,
                ((unsigned int)addr[n] >> 4) & 0xf);
            if (advance > 0 && qp + advance < ep)
                qp += advance;
            else {
                *info->he = NETDB_INTERNAL;
                return NS_NOTFOUND;
            }
        }
        if (strlcat(qbuf, "ip6.arpa", sizeof(qbuf)) >= sizeof(qbuf)) {
            *info->he = NETDB_INTERNAL;
            return NS_NOTFOUND;
        }
        break;

    default:
        *info->he = NO_RECOVERY;
        return NS_UNAVAIL;
    }

    hostent_ctx h_ctx;
    _mdns_hostent_init(&h_ctx, af, addrlen);
    _mdns_hostent_add_addr(&h_ctx, addr, addrlen);

    sr = get_svc_ref();
    if (!sr) {
        *info->he = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    while (sr && retry) {
        /* We must always use a copy of the ref when using a shared
           connection, per kDNSServiceFlagsShareConnection docs */
        sdRef = sr->sdRef;

        err = DNSServiceQueryRecord(
            &sdRef,
            svc_flags
                | kDNSServiceFlagsShareConnection
                | kDNSServiceFlagsReturnIntermediates,
            kDNSServiceInterfaceIndexAny,
            qbuf,
            kDNSServiceType_PTR,
            kDNSServiceClass_IN,
            _mdns_hostent_cb,
            &h_ctx
        );
        
        retry = retry_query(&sr, err);
    }

    if (err) {
        put_svc_ref(sr);
        *info->he = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }

    _mdns_eventloop(sr, (void *)&h_ctx, get_timeout());

    DNSServiceRefDeallocate(sdRef);
    put_svc_ref(sr);

    if (h_ctx.naliases) {
        return _mdns_hostent_done(info, &h_ctx);
    } else {
        *info->he = HOST_NOT_FOUND;
        return NS_NOTFOUND;
    }
}



static int
_mdns_gethtbyname(void *cbrv, void *cbdata, va_list ap)
{
    int namelen, af, addrlen, rrtype, err;
    const char *name, *sname;
    DNSServiceRef sdRef;
    search_iter iter;
    svc_ref *sr;
    struct getnamaddr *info = cbrv;

    UNUSED(cbdata);
    
    name = va_arg(ap, char *);
    namelen = va_arg(ap, int);
    af = va_arg(ap, int);

    UNUSED(namelen);
    
    switch (af) {
    case AF_INET:
        rrtype = kDNSServiceType_A;
        addrlen = 4;
        break;
    
    case AF_INET6:
        rrtype = kDNSServiceType_AAAA;
        addrlen = 16;
        break;

    default:
        *info->he = NO_RECOVERY;
        return NS_UNAVAIL;
    }

    sr = get_svc_ref();
    if (!sr) {
        *info->he = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    if ((err = search_init(&iter, name, &sname)) != NS_SUCCESS) {
        put_svc_ref(sr);
        return err;
    }

    hostent_ctx h_ctx;
    _mdns_hostent_init(&h_ctx, af, addrlen);

    err = NS_NOTFOUND;
    while (sr && sname && (err != NS_SUCCESS)) {
        err = _mdns_gethtbyname_abs(info, &h_ctx, sname, rrtype, &sr,
	    iter.conf->timeout);
        if (err != NS_SUCCESS) {
            sname = search_next(&iter);
        }
    };

    search_done(&iter);
    put_svc_ref(sr);
    
    if (err != NS_SUCCESS)
	return err;
    _mdns_hostent_add_host(&h_ctx, sname);
    _mdns_hostent_add_host(&h_ctx, name);
    return _mdns_hostent_done(info, &h_ctx);
}



static int
_mdns_gethtbyname_abs(struct getnamaddr *info, struct hostent_ctx *ctx,
    const char *name, int rrtype, svc_ref **sr, short timeout)
{
    DNSServiceErrorType err = kDNSServiceErr_ServiceNotRunning;
    DNSServiceRef sdRef;
    bool retry = true;    

    while (*sr && retry) {
        /* We must always use a copy of the ref when using a shared
           connection, per kDNSServiceFlagsShareConnection docs */
        sdRef = (*sr)->sdRef;

        err = DNSServiceQueryRecord(
            &sdRef,
            svc_flags
                | kDNSServiceFlagsShareConnection
                | kDNSServiceFlagsReturnIntermediates,
            kDNSServiceInterfaceIndexAny,
            name,
            rrtype,
            kDNSServiceClass_IN,
            _mdns_hostent_cb,
            ctx
        );
        
        retry = retry_query(sr, err);
    }

    if (err) {
        *info->he = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }

    _mdns_eventloop(*sr, (void *)ctx, timeout);

    DNSServiceRefDeallocate(sdRef);

    if (ctx->naddrs) {
        return NS_SUCCESS;
    } else {
        *info->he = HOST_NOT_FOUND;
        return NS_NOTFOUND;
    }
}



static void
_mdns_addrinfo_init(addrinfo_ctx *ctx, const struct addrinfo *ai)
{
    ctx->cb_ctx.done = false;
    ctx->start = *ai;
    ctx->start.ai_next = NULL;
    ctx->start.ai_canonname = NULL;
    ctx->last = &(ctx->start);
}



static void
_mdns_addrinfo_add_ai(addrinfo_ctx *ctx, struct addrinfo *ai)
{
    ctx->last->ai_next = ai;
    while (ctx->last->ai_next)
        ctx->last = ctx->last->ai_next;
}



static struct addrinfo *
_mdns_addrinfo_done(addrinfo_ctx *ctx)
{
    struct addrinfo head, *t, *p;
            
    /* sort v6 up */

    t = &head;
    p = ctx->start.ai_next;

    while (p->ai_next) {
        if (p->ai_next->ai_family == AF_INET6) {
            t->ai_next = p->ai_next;
            t = t->ai_next;
            p->ai_next = p->ai_next->ai_next;
        } else {
            p = p->ai_next;
        }
    }

    /* add rest of list and reset start to the new list */

    t->ai_next = ctx->start.ai_next;
    ctx->start.ai_next = head.ai_next;
    
    return ctx->start.ai_next;
}



static void
_mdns_hostent_init(hostent_ctx *ctx, int af, int addrlen)
{
    int i;
    
    ctx->cb_ctx.done = false;
    ctx->naliases = ctx->naddrs = 0;
    ctx->next = ctx->buf;

    ctx->host.h_aliases = ctx->host_aliases;
    ctx->host.h_addr_list = ctx->h_addr_ptrs;
    ctx->host.h_name = ctx->host.h_aliases[0] = NULL;
    ctx->host.h_addrtype = af;
    ctx->host.h_length = addrlen;
    
    for (i = 0; i < MAXADDRS; i++) {
        ctx->host.h_addr_list[i] = &(ctx->addrs[i * 16]);
    }
}



static void
_mdns_hostent_add_host(hostent_ctx *ctx, const char *name)
{
    size_t len;
    int i;
    
    if (name && (len = strlen(name))
    && (HCTX_BUFLEFT(ctx) > len) && (ctx->naliases < MAXALIASES)) {
        if (len && (name[len - 1] == '.')) {
            len--;
        }
        
        /* skip dupe names */

        if ((ctx->host.h_name) && !strncmp(ctx->host.h_name, name, len)
        && (strlen(ctx->host.h_name) == len)) {
            return;
        }

        for (i = 0; i < ctx->naliases - 1; i++) {
            if (!strncmp(ctx->host.h_aliases[i], name, len)
            && (strlen(ctx->host.h_aliases[i]) == len)) {
                return;
            }
        }
    
        strncpy(ctx->next, name, len);
        ctx->next[len] = 0;
        
        if (ctx->naliases == 0) {
            ctx->host.h_name = ctx->next;
        } else {
            ctx->host.h_aliases[ctx->naliases - 1] = ctx->next;
        }
        
        ctx->next += (len + 1);
        ctx->naliases++;
    } /* else silently ignore */
}



static void
_mdns_hostent_add_addr(hostent_ctx *ctx, const void *addr, uint16_t len)
{
    if ((len == ctx->host.h_length) && (ctx->naddrs < MAXADDRS)) {
        memcpy(ctx->host.h_addr_list[ctx->naddrs++], addr, (size_t)len);
    } /* else wrong address type or out of room... silently skip */
}



static int
_mdns_hostent_done(struct getnamaddr *info, hostent_ctx *ctx)
{
    int i;
    char *ptr = info->buf;
    size_t len = info->buflen;
    struct hostent *hp = info->hp;
    struct hostent *chp = &ctx->host;

    hp->h_length = ctx->host.h_length;
    hp->h_addrtype = ctx->host.h_addrtype;
    HENT_ARRAY(hp->h_addr_list, ctx->naddrs, ptr, len);
    HENT_ARRAY(hp->h_aliases, ctx->naliases - 1, ptr, len);

    for (i = 0; i < ctx->naddrs; i++)
	HENT_COPY(hp->h_addr_list[i], chp->h_addr_list[i],
	    hp->h_length, ptr, len);

    hp->h_addr_list[ctx->naddrs] = NULL;

    HENT_SCOPY(hp->h_name, chp->h_name, ptr, len);

    for (i = 0; i < ctx->naliases - 1; i++)
	    HENT_SCOPY(hp->h_aliases[i], chp->h_aliases[i], ptr, len);
    hp->h_aliases[ctx->naliases - 1] = NULL;
    *info->he = 0;
    return NS_SUCCESS;
nospc:
    *info->he = NETDB_INTERNAL;
    errno = ENOSPC;
    return NS_UNAVAIL;
}



static void
_mdns_addrinfo_cb(
    DNSServiceRef           sdRef,
    DNSServiceFlags         flags,
    uint32_t                interfaceIndex,
    DNSServiceErrorType     errorCode,
    const char             *hostname,
    const struct sockaddr  *address,
    uint32_t                ttl,
    void                   *context
) {
    addrinfo_ctx *ctx = context;
    struct addrinfo *ai;

    UNUSED(sdRef);
    UNUSED(interfaceIndex);
    UNUSED(ttl);

    if (errorCode == kDNSServiceErr_NoError) {
        if (! (flags & kDNSServiceFlagsMoreComing)) {
            ctx->cb_ctx.done = true;
        }

        ai = allocaddrinfo((socklen_t)(address->sa_len));
        if (ai) {
            ai->ai_flags = ctx->start.ai_flags;
            ai->ai_family = address->sa_family;
            ai->ai_socktype = ctx->start.ai_socktype;
            ai->ai_protocol = ctx->start.ai_protocol;
            memcpy(ai->ai_addr, address, (size_t)(address->sa_len));
            
            if ((ctx->start.ai_flags & AI_CANONNAME) && hostname) {
                ai->ai_canonname = strdup(hostname);
                if (ai->ai_canonname[strlen(ai->ai_canonname) - 1] == '.') {
                    ai->ai_canonname[strlen(ai->ai_canonname) - 1] = '\0';
                }
            }

            _mdns_addrinfo_add_ai(ctx, ai);
        }
    }
}



static void
_mdns_hostent_cb(
    DNSServiceRef           sdRef,
    DNSServiceFlags         flags,
    uint32_t                interfaceIndex,
    DNSServiceErrorType     errorCode,
    const char             *fullname,
    uint16_t                rrtype,
    uint16_t                rrclass,
    uint16_t                rdlen,
    const void             *rdata,
    uint32_t                ttl,
    void                   *context
) {
    hostent_ctx *ctx = (hostent_ctx *)context;
    char buf[NS_MAXDNAME+1];

    UNUSED(sdRef);
    UNUSED(interfaceIndex);
    UNUSED(rrclass);
    UNUSED(ttl);

    if (! (flags & kDNSServiceFlagsMoreComing)) {
        ctx->cb_ctx.done = true;
    }

    if (errorCode == kDNSServiceErr_NoError) {
        switch (rrtype) {
        case kDNSServiceType_PTR:
            if (!_mdns_rdata2name(rdata, rdlen, buf, sizeof(buf))) {
                /* corrupt response -- skip */
                return;
            }
        
            _mdns_hostent_add_host(ctx, buf);
            break;

        case kDNSServiceType_A:
            if (ctx->host.h_addrtype == AF_INET) {
                _mdns_hostent_add_host(ctx, fullname);
                _mdns_hostent_add_addr(ctx, rdata, rdlen);
            }
            break;

        case kDNSServiceType_AAAA:
            if (ctx->host.h_addrtype == AF_INET6) {
                _mdns_hostent_add_host(ctx, fullname);
                _mdns_hostent_add_addr(ctx, rdata, rdlen);
            }
            break;
        }
    } else if (errorCode == kDNSServiceErr_NoSuchRecord) {
        ctx->cb_ctx.done = true;
    }
}



static void
_mdns_eventloop(svc_ref *sr, callback_ctx *ctx, short timeout)
{
    struct pollfd fds;
    int fd, ret;

    fd = DNSServiceRefSockFD(sr->sdRef);
    fds.fd = fd;
    fds.events = POLLRDNORM;

    while (!ctx->done) {
        ret = poll(&fds, 1, timeout * 1000);
        if (ret > 0) {
            DNSServiceProcessResult(sr->sdRef);
        } else {
            break;
        }
    }
}



static char *
_mdns_rdata2name(const unsigned char *rdata, uint16_t rdlen, char *buf, size_t buflen)
{
    unsigned char l;
    char *r = buf;

    /* illegal 0-size answer or not enough room for even "." */
    if ((!rdlen) || (rdlen < 2)) {
        return NULL;
    }
    
    buflen--; /* reserve space for terminating NUL now */
    
    /* special case empty as "." */
    if ((rdlen == 1) && (!*rdata)) {
        strcpy(buf, ".");
            
        return r;
    }
    
    while (rdlen && *rdata) {
        /* label length byte */
        l = *rdata++; rdlen--;
        
        if (l > 63) {
            /* compression or bitstrings -- shouldn't happen */
            return NULL;
        } else if (l > buflen) {
            /* not enough space */
            return NULL;
        } else if (l > rdlen) {
            /* label shouldn't be longer than remaining rdata */
            return NULL;
        } else if (!l) {
            /* empty label -- should be done */
            if (rdlen) {
                /* but more left!? */
                return NULL;
            } else {
                break;
            }
        }
        
        memcpy(buf, rdata, (size_t)l);
        rdata += l; buf += l;
        rdlen -= l; buflen -= l;
        
        /* Another label to come? add a separator */
        if (rdlen && *rdata) {
            if (!buflen) {
                return NULL;
            }

            *buf++ = '.'; buflen--;
        }
    }

    /* we reserved space above, so we know we have space
       to add this termination */
       
    *buf = '\0';

    return r;
}



int
search_init(search_iter *iter, const char *name, const char **first)
{
    const char *c = name, *cmp;
    int dots = 0, enddot = 0;
    size_t len, cl;
    
    iter->conf = get_res_conf();
    if (!iter->conf) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }

    iter->name = name;
    iter->baselen = 0;
    iter->abs_first = iter->abs_last = false;
    iter->next_search = iter->conf->search_domains;
        
    while (*c) {
        if (*c == '.') {
            dots++;
            enddot = 1;
        } else {
            enddot = 0;
        }
        c++;
    } 
    
    if (svc_flags & kDNSServiceFlagsForceMulticast) {
        if (dots) {
            iter->next_search = iter->conf->no_search;
            if ((dots - enddot) == 1) {
                len = strlen(iter->name);
                cl = strlen(".local") + enddot;
                if (len > cl) {
                    cmp = enddot ? ".local." : ".local";
                    c = iter->name + len - cl;
                    
                    if (!strcasecmp(c, cmp)) {
                        iter->abs_first = true;
                    }
                }
            }
        }
    } else {
        if (dots >= iter->conf->ndots) {
            iter->abs_first = true;
        } else {
            iter->abs_last = true;
        }
    
        if (enddot) {
            /* absolute; don't search */
            iter->next_search = iter->conf->no_search;
        }
    }
    
    *first = search_next(iter);
    if (!first) {
        search_done(iter);
        h_errno = HOST_NOT_FOUND;
        return NS_NOTFOUND;
    }
    
    return NS_SUCCESS;
}



const char *
search_next(search_iter *iter)
{
    const char *a = NULL;
    res_state res;
    size_t len;
    
    if (iter->abs_first) {
        iter->abs_first = false;
        return iter->name;
    }
    
    while (*(iter->next_search)) {
        if (!iter->baselen) {
            iter->baselen = strlcpy(iter->buf, iter->name, sizeof(iter->buf));
            if (iter->baselen >= sizeof(iter->buf) - 1) {
                /* original is too long, don't try any search domains */
                iter->next_search = iter->conf->no_search;
                break;
            }

            iter->buf[iter->baselen++] = '.';
        }

        len = strlcpy(&(iter->buf[iter->baselen]),
            *(iter->next_search),
            sizeof(iter->buf) - iter->baselen);

        iter->next_search++;

        if (len >= sizeof(iter->buf) - iter->baselen) {
            /* result was too long */
            continue;
        }

        return iter->buf;
    }
    
    if (iter->abs_last) {
        iter->abs_last = false;
        return iter->name;
    }
    
    return NULL;
}



void
search_done(search_iter *iter)
{
    if (iter->conf) {
        put_res_conf(iter->conf);
        iter->conf = NULL;
    }
}



/*
 * Is domain appropriate to be in the domain search list?
 * For mdnsd, take everything. For multicast_dns, only "local"
 * if present.
 */
bool
searchable_domain(char *d)
{
    if (!(svc_flags & kDNSServiceFlagsForceMulticast)) {
        return true;
    }
    
    if (!strcasecmp(d, "local") || !strcasecmp(d, "local.")) {
        return true;
    }
    
    return false;
}



static void
destroy_svc_ref(svc_ref *sr)
{
    /* assumes not on conn list */
    
    if (sr) {
        DNSServiceRefDeallocate(sr->sdRef);
        free(sr);
    }
}



static svc_ref *
get_svc_ref(void)
{
    svc_ref *sr;
    
    LOCK(&conn_list_lock);
    
    if (getpid() != my_pid) {
        my_pid = getpid();

        /*
         * We forked and kept running. We don't want to share
         * connections with the parent or we'll garble each others
         * comms, so throw away the parent's list and start over
         */
        while ((sr = SLIST_FIRST(&conn_list))) {
            SLIST_REMOVE_HEAD(&conn_list, entries);
            destroy_svc_ref(sr);
        }

        sr = NULL;
    } else {
        /* try to recycle a connection */
        sr = SLIST_FIRST(&conn_list);
        if (sr) {
            SLIST_REMOVE_HEAD(&conn_list, entries);
            conn_count--;
        }
    }
    
    UNLOCK(&conn_list_lock);
    
    if (!sr) {
        /* none available, we need a new one */

        sr = calloc(sizeof(svc_ref), 1);
        if (sr) {
            if (DNSServiceCreateConnection(&(sr->sdRef))) {
                free(sr);
                return NULL;
            }
            
            if (fcntl(DNSServiceRefSockFD(sr->sdRef), F_SETFD, FD_CLOEXEC) < 0) {
                destroy_svc_ref(sr);
                sr = NULL;
            }
        }
    }

    return sr;
}



static void
put_svc_ref(svc_ref *sr)
{
    if (sr) {
        LOCK(&conn_list_lock);

        sr->uses++;

        /* if slow start or aged out, destroy */
        if ((svc_puts++ < SLOWSTART_LOOKUPS)
        ||  (conn_count && (sr->uses > REUSE_TIMES))) {
            UNLOCK(&conn_list_lock);
            destroy_svc_ref(sr);
            return;
        }

        conn_count++;
        
        SLIST_INSERT_HEAD(&conn_list, sr, entries);
        
        UNLOCK(&conn_list_lock);
    }
}



/*
 * determine if this is a call we should retry with a fresh
 * connection, for example if mdnsd went away and came back.
 */
static bool
retry_query(svc_ref **sr, DNSServiceErrorType err)
{
    if ((err == kDNSServiceErr_Unknown)
    ||  (err == kDNSServiceErr_ServiceNotRunning)) {
        /* these errors might indicate a stale socket */
        if ((*sr)->uses) {
            /* this was an old socket, so kill it and get another */
            destroy_svc_ref(*sr);
            *sr = get_svc_ref();
            if (*sr) {
                /* we can retry */
                return true;
            }
        }
    }
    
    return false;
}



static void
decref_res_conf(res_conf *rc)
{
    rc->refcount--;
    
    if (rc->refcount < 1) {
        if ((rc->no_search = rc->search_domains)) {
            for (; *(rc->no_search); rc->no_search++) {
                free(*(rc->no_search));
            }
        
            free(rc->search_domains);
        }
        
        free(rc);
    }
}



res_conf *
get_res_conf(void)
{
    struct timespec last_change;
    res_state res;
    res_conf *rc;

    LOCK(&res_conf_lock);
    
    /* check if resolver config changed */

    res = __res_get_state();
    if (res) {
        res_check(res, &last_change);
        if (timespeccmp(&last_config, &last_change, <)) {
            if (cur_res_conf) {
                decref_res_conf(cur_res_conf);
            }
            cur_res_conf = new_res_conf(res);
            if (cur_res_conf) {
                last_config = last_change;
            }
        }
        __res_put_state(res);
    }

    rc = cur_res_conf;
    if (rc) {
        rc->refcount++;
    }

    UNLOCK(&res_conf_lock);
    
    return rc;
}



static void
put_res_conf(res_conf *rc)
{
    LOCK(&res_conf_lock);
    
    decref_res_conf(rc);
    
    UNLOCK(&res_conf_lock);
}



static res_conf *
new_res_conf(res_state res)
{
    res_conf *rc;
    int count = 0;
    char **sd, *p;
    
    rc = calloc(sizeof(res_conf), 1);
    if (rc) {
        rc->refcount = 1;
        
        sd = res->dnsrch;
        while (*sd) {
            if (searchable_domain(*sd)) {
                count++;
            }
            sd++;
        }
    
        rc->search_domains = calloc(sizeof(char *), count + 1);
        if (!(rc->search_domains)) {
            decref_res_conf(rc);
            return NULL;
        }
    
        sd = res->dnsrch;
        rc->no_search = rc->search_domains;
        while (*sd) {
            if (searchable_domain(*sd)) {
                *(rc->no_search) = p = strdup(*sd);
                if (!p) {
                    decref_res_conf(rc);
                    return NULL;
                }
                    
                rc->no_search++;
            }
            sd++;
        }

        rc->timeout = res->retrans;

        if (svc_flags & kDNSServiceFlagsForceMulticast) {
            rc->ndots = 1;
            if (rc->timeout > 2) {
                rc->timeout = 2;
            }
        } else {
            rc->ndots = res->ndots;
        }
    }
    
    return rc;
}



static short
get_timeout(void)
{
    short timeout = 5;
    res_conf *rc;
    
    rc = get_res_conf();
    if (rc) {
        timeout = rc->timeout;
        put_res_conf(rc);
    }

    return timeout;
}
