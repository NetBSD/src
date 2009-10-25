/*	$NetBSD: nss_mdnsd.c,v 1.1 2009/10/25 00:17:06 tsarna Exp $	*/

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
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <dns_sd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

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

typedef struct search_domain {
    const char *name;
    bool        enabled;
} search_domain;

typedef struct search_iter {
    const char     *name;
    search_domain  *next_search;
    size_t          baselen;
    bool            abs_first;
    bool            abs_last;
    char            buf[MAXHOSTNAMELEN];
} search_iter;

static hostent_ctx h_ctx;
static DNSServiceFlags svc_flags = 0;
static int ndots = 1, timeout = 1000;
static search_domain *search_domains, *no_search;

ns_mtab *nss_module_register(const char *, u_int *, nss_module_unregister_fn *);
static int load_config(res_state);

static int _mdns_getaddrinfo(void *, void *, va_list);
static int _mdns_gethtbyaddr(void *, void *, va_list);
static int _mdns_gethtbyname(void *, void *, va_list);

static int _mdns_getaddrinfo_abs(const char *, DNSServiceProtocol,
    DNSServiceRef, addrinfo_ctx *);
static void _mdns_addrinfo_init(addrinfo_ctx *, const struct addrinfo *);
static void _mdns_addrinfo_add_ai(addrinfo_ctx *, struct addrinfo *);
static struct addrinfo *_mdns_addrinfo_done(addrinfo_ctx *);

static int _mdns_gethtbyname_abs(const char *, int, DNSServiceRef);
static void _mdns_hostent_init(hostent_ctx *, int, int);
static void _mdns_hostent_add_host(hostent_ctx *, const char *);
static void _mdns_hostent_add_addr(hostent_ctx *, const void *, uint16_t);
static struct hostent *_mdns_hostent_done(hostent_ctx *);

static void _mdns_addrinfo_cb(DNSServiceRef, DNSServiceFlags,
    uint32_t, DNSServiceErrorType, const char *, const struct sockaddr *,
    uint32_t, void *);
static void _mdns_hostent_cb(DNSServiceRef, DNSServiceFlags,
    uint32_t, DNSServiceErrorType, const char *, uint16_t, uint16_t, uint16_t,
    const void *, uint32_t, void *);
static void _mdns_eventloop(DNSServiceRef, callback_ctx *);

static char *_mdns_rdata2name(const unsigned char *, uint16_t,
    char *, size_t);

void search_init(search_iter *, const char *);
const char *search_next(search_iter *);



static ns_mtab mtab[] = {
    { NSDB_HOSTS, "getaddrinfo",    _mdns_getaddrinfo, NULL },
    { NSDB_HOSTS, "gethostbyaddr",  _mdns_gethtbyaddr, NULL },
    { NSDB_HOSTS, "gethostbyname",  _mdns_gethtbyname, NULL },
};



ns_mtab *
nss_module_register(const char *source, u_int *nelems,
                    nss_module_unregister_fn *unreg)
{
    res_state res;

    UNUSED(unreg);
    
    *nelems = sizeof(mtab) / sizeof(mtab[0]);
    *unreg = NULL;

    if (!strcmp(source, "multicast_dns")) {
        svc_flags = kDNSServiceFlagsForceMulticast;
    }

    res = __res_get_state();
    if (res) {
        load_config(res);
        __res_put_state(res);
    }

    return mtab;
}



static int
load_config(res_state res)
{
    int count = 0;
    char **sd;
    
    ndots = res->ndots;
    timeout = res->retrans * 1000; /* retrans in sec to timeout in msec */

    sd = res->dnsrch;
    while (*sd) {
        count++;
        sd++;
    }
    
    search_domains = malloc(sizeof(search_domain) * (count + 1));
    if (!search_domains) {
        return -1;
    }
    
    sd = res->dnsrch;
    no_search = search_domains;
    while (*sd) {
        no_search->name = *sd;
        no_search->enabled =
            (svc_flags & kDNSServiceFlagsForceMulticast) ? true : true;
        no_search++;
        sd++;
    }

    /* terminate list */
    no_search->name = NULL;
}



static int
_mdns_getaddrinfo(void *cbrv, void *cbdata, va_list ap)
{
    const struct addrinfo *pai;
    const char *name, *sname;
    DNSServiceProtocol proto;
    int err = NS_UNAVAIL;
    DNSServiceRef sdRef;
    addrinfo_ctx ctx;
    search_iter iter;
    
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

    /* use one connection for all searches */
    if (DNSServiceCreateConnection(&sdRef)) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_addrinfo_init(&ctx, pai);
    search_init(&iter, name);
    sname = search_next(&iter);
    
    while (sname && (err != NS_SUCCESS)) {
        err = _mdns_getaddrinfo_abs(sname, proto, sdRef, &ctx);
        if (err != NS_SUCCESS) {
            sname = search_next(&iter);
        }
    };

    DNSServiceRefDeallocate(sdRef);

    if (err == NS_SUCCESS) {
        *(struct addrinfo **)cbrv = _mdns_addrinfo_done(&ctx);
    }
    
    return err;
}



static int
_mdns_getaddrinfo_abs(const char *name, DNSServiceProtocol proto,
    DNSServiceRef sdRef, addrinfo_ctx *ctx)
{
    DNSServiceErrorType err;

    err = DNSServiceGetAddrInfo(
        &sdRef,
        svc_flags,
        kDNSServiceInterfaceIndexAny,
        proto,
        name,
        _mdns_addrinfo_cb,
        ctx
    );

    if (err) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_eventloop(sdRef, (void *)ctx);

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

    UNUSED(cbdata);
    
    addr = va_arg(ap, unsigned char *);
    addrlen = va_arg(ap, int);
    af = va_arg(ap, int);

    switch (af) {
    case AF_INET:
        (void)snprintf(qbuf, sizeof(qbuf), "%u.%u.%u.%u.in-addr.arpa",
            (addr[3] & 0xff), (addr[2] & 0xff),
            (addr[1] & 0xff), (addr[0] & 0xff));
        break;
    
    case AF_INET6:
        qp = qbuf;
        ep = qbuf + sizeof(qbuf) - 1;
        for (n = IN6ADDRSZ - 1; n >= 0; n--) {
            advance = snprintf(qp, (size_t)(ep - qp), "%x.%x.",
                addr[n] & 0xf,
                ((unsigned int)addr[n] >> 4) & 0xf);
            if (advance > 0 && qp + advance < ep)
                qp += advance;
            else {
                h_errno = NETDB_INTERNAL;
                return NS_NOTFOUND;
            }
        }
        if (strlcat(qbuf, "ip6.arpa", sizeof(qbuf)) >= sizeof(qbuf)) {
            h_errno = NETDB_INTERNAL;
            return NS_NOTFOUND;
        }
        break;

    default:
        h_errno = NO_RECOVERY;
        return NS_UNAVAIL;
    }

    _mdns_hostent_init(&h_ctx, af, addrlen);
    _mdns_hostent_add_addr(&h_ctx, addr, addrlen);

    err = DNSServiceQueryRecord(
        &sdRef,
        svc_flags,
        kDNSServiceInterfaceIndexAny,
        qbuf,
        kDNSServiceType_PTR,
        kDNSServiceClass_IN,
        _mdns_hostent_cb,
        &h_ctx
    );

    if (err) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_eventloop(sdRef, (void *)&h_ctx);

    DNSServiceRefDeallocate(sdRef);

    if (h_ctx.naliases) {
        *(struct hostent **)cbrv = _mdns_hostent_done(&h_ctx);
    
        return NS_SUCCESS;
    } else {
        h_errno = HOST_NOT_FOUND;
        return NS_NOTFOUND;
    }
}



static int
_mdns_gethtbyname(void *cbrv, void *cbdata, va_list ap)
{
    int namelen, af, addrlen, rrtype, err = NS_UNAVAIL;
    const char *name, *sname;
    DNSServiceRef sdRef;
    search_iter iter;

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
        h_errno = NETDB_INTERNAL;
        return NS_NOTFOUND;
    }

    /* use one connection for all searches */
    if (DNSServiceCreateConnection(&sdRef)) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_hostent_init(&h_ctx, af, addrlen);
    search_init(&iter, name);
    sname = search_next(&iter);
    
    while (sname && (err != NS_SUCCESS)) {
        err = _mdns_gethtbyname_abs(sname, rrtype, sdRef);
        if (err != NS_SUCCESS) {
            sname = search_next(&iter);
        }
    };

    DNSServiceRefDeallocate(sdRef);

    if (err == NS_SUCCESS) {
        _mdns_hostent_add_host(&h_ctx, sname);
        _mdns_hostent_add_host(&h_ctx, name);
        *(struct hostent **)cbrv = _mdns_hostent_done(&h_ctx);
    }
    
    return err;
}



static int
_mdns_gethtbyname_abs(const char *name, int rrtype, DNSServiceRef sdRef)
{
    DNSServiceErrorType err;

    err = DNSServiceQueryRecord(
        &sdRef,
        svc_flags,
        kDNSServiceInterfaceIndexAny,
        name,
        rrtype,
        kDNSServiceClass_IN,
        _mdns_hostent_cb,
        &h_ctx
    );

    if (err) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_eventloop(sdRef, (void *)&h_ctx);

    DNSServiceRefDeallocate(sdRef);

    if (h_ctx.naddrs) {
        return NS_SUCCESS;
    } else {
        h_errno = HOST_NOT_FOUND;
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



static struct hostent *
_mdns_hostent_done(hostent_ctx *ctx)
{
    if (ctx->naliases) {
        /* terminate array */
        ctx->host.h_aliases[ctx->naliases - 1] = NULL;
        ctx->host.h_addr_list[ctx->naddrs] = NULL;
    }
    
    return &(ctx->host);
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
    UNUSED(hostname);
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
    }
}



static void
_mdns_eventloop(DNSServiceRef sdRef, callback_ctx *ctx)
{
    struct pollfd fds;
    int fd, ret;

    fd = DNSServiceRefSockFD(sdRef);
    fds.fd = fd;
    fds.events = POLLRDNORM;

    while (!ctx->done) {
        ret = poll(&fds, 1, timeout);
        if (ret > 0) {
            DNSServiceProcessResult(sdRef);
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



void
search_init(search_iter *iter, const char *name)
{
    const char *c = name;
    bool enddot = false;
    int dots = 0;

    iter->name = name;
    iter->baselen = 0;
        
    while (*c) {
        if (*c == '.') {
            dots++;
            enddot = true;
        } else {
            enddot = false;
        }
        c++;
    } 
    
    if (dots >= ndots) {
        iter->abs_first = true;
        iter->abs_last = false;
    } else {
        iter->abs_first = false;
        iter->abs_last = true;
    }
    
    if (enddot) {
        iter->next_search = no_search;
    } else {
        iter->next_search = search_domains;
    }
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
    
    while (iter->next_search->name) {
        if (iter->next_search->enabled) {
            if (!iter->baselen) {
                iter->baselen = strlcpy(iter->buf, iter->name, sizeof(iter->buf));
                if (iter->baselen >= sizeof(iter->buf) - 1) {
                    /* original is too long, don't try any search domains */
                    iter->next_search = no_search;
                    break;
                }
                
                iter->buf[iter->baselen++] = '.';
            }
            
            len = strlcpy(&(iter->buf[iter->baselen]),
                iter->next_search->name,
                sizeof(iter->buf) - iter->baselen);
                
            iter->next_search++;

            if (len >= sizeof(iter->buf) - iter->baselen) {
                /* result would be too long */
                continue;
            }
            
            return iter->buf;
        } else {
            iter->next_search++;
        }
    }
    
    if (iter->abs_last) {
        iter->abs_last = false;
        return iter->name;
    }
    
    return NULL;
}
