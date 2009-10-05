/*	$NetBSD: nss_mdns.c,v 1.1 2009/10/05 03:54:17 tsarna Exp $	*/

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

#include <nsswitch.h>
#include <stdarg.h>  
#include <stdlib.h>  
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <dns_sd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>

#ifndef lint
#define UNUSED(a)       (void)&a
#else
#define UNUSED(a)       a = a
#endif

#if defined(__NetBSD__) && (__NetBSD_Version__ < 599002000)
static struct addrinfo *
allocaddrinfo(socklen_t addrlen)
{
    struct addrinfo *ai;
    
    ai = calloc(sizeof(struct addrinfo) + addrlen, 1);
    if (ai) {
        ai->ai_addrlen = addrlen;
        ai->ai_addr = (void *)(ai+1);
    }
    
    return ai;
}
#endif

#define MAXALIASES      35 
#define MAXADDRS        35

typedef struct callback_ctx {
    int done;
} callback_ctx;

typedef struct hostent_ctx {
    callback_ctx cb_ctx;    /* must come first */
    struct hostent host;
    char *h_addr_ptrs[MAXADDRS + 1];
    char *host_aliases[MAXALIASES];
    char addrs[MAXADDRS * 16];
    char buf[8192], *next;
    int done, naliases, naddrs;
} hostent_ctx;

typedef struct addrinfo_ctx {
    callback_ctx cb_ctx;    /* must come first */
    struct addrinfo start, *last;
} addrinfo_ctx;

#define HCTX_BUFLEFT(c) (sizeof((c)->buf) - ((c)->next - (c)->buf))

static hostent_ctx h_ctx;


static int _mdns_getaddrinfo(void *cbrv, void *cbdata, va_list ap);
static int _mdns_gethtbyaddr(void *cbrv, void *cbdata, va_list ap);
static int _mdns_gethtbyname(void *cbrv, void *cbdata, va_list ap);

static void _mdns_addrinfo_init(addrinfo_ctx *ctx, const struct addrinfo *ai);
static void _mdns_addrinfo_add_ai(addrinfo_ctx *ctx, struct addrinfo *ai);
static struct addrinfo *_mdns_addrinfo_done(addrinfo_ctx *ctx);

static void _mdns_hostent_init(hostent_ctx *ctx, int af, int addrlen);
static void _mdns_hostent_add_host(hostent_ctx *ctx, const char *name);
static void _mdns_hostent_add_addr(hostent_ctx *ctx, const void *addr, uint16_t len);
static struct hostent *_mdns_hostent_done(hostent_ctx *ctx);

static void _mdns_addrinfo_cb(DNSServiceRef sdRef, DNSServiceFlags flags,
    uint32_t interfaceIndex, DNSServiceErrorType errorCode,
    const char *hostname, const struct sockaddr *address, uint32_t ttl,
    void *context);
static void _mdns_hostent_cb(DNSServiceRef sdRef, DNSServiceFlags flags,
    uint32_t interfaceIndex, DNSServiceErrorType errorCode,
    const char *fullname, uint16_t rrtype, uint16_t rrclass,
    uint16_t rdlen, const void *rdata, uint32_t ttl, void *context);
static void _mdns_eventloop(DNSServiceRef sdRef, callback_ctx *ctx);

static char *_mdns_rdata2name(const unsigned char *rdata, uint16_t rdlen,
    char *buf, size_t buflen);


static ns_mtab mtab[] = {
    { NSDB_HOSTS, "getaddrinfo",    _mdns_getaddrinfo, NULL },
    { NSDB_HOSTS, "gethostbyaddr",  _mdns_gethtbyaddr, NULL },
    { NSDB_HOSTS, "gethostbyname",  _mdns_gethtbyname, NULL },
};



ns_mtab *
nss_module_register(const char *source, u_int *nelems,
                    nss_module_unregister_fn *unreg)
{
    UNUSED(source);
    UNUSED(unreg);
    
    *nelems = sizeof(mtab) / sizeof(mtab[0]);
    *unreg = NULL;

    return mtab;
}



static int
_mdns_getaddrinfo(void *cbrv, void *cbdata, va_list ap)
{
    const struct addrinfo *pai;
    const char *name;
    addrinfo_ctx ctx;
    DNSServiceProtocol proto;
    DNSServiceErrorType err;
    DNSServiceRef sdRef;
    
    UNUSED(cbdata);
    
    name = va_arg(ap, char *);
    pai = va_arg(ap, struct addrinfo *);
    
    //XXX check valid name space
    
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
        return NS_UNAVAIL;
    }

    _mdns_addrinfo_init(&ctx, pai);

    err = DNSServiceGetAddrInfo(
        &sdRef,
        kDNSServiceFlagsForceMulticast,
        kDNSServiceInterfaceIndexAny,
        proto,
        name,
        _mdns_addrinfo_cb,
        &ctx
    );

    if (err) {
        h_errno = NETDB_INTERNAL;
        return NS_UNAVAIL;
    }
    
    _mdns_eventloop(sdRef, (void *)&h_ctx);

    DNSServiceRefDeallocate(sdRef);

    if (ctx.start.ai_next) {
        *(struct addrinfo **)cbrv = _mdns_addrinfo_done(&ctx);
    
        return NS_SUCCESS;
    } else {
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

    //XXX Check if in valid address space
    
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
        h_errno = NETDB_INTERNAL;
        return NS_NOTFOUND;
    }

    _mdns_hostent_init(&h_ctx, af, addrlen);
    _mdns_hostent_add_addr(&h_ctx, addr, addrlen);

    err = DNSServiceQueryRecord(
        &sdRef,
        kDNSServiceFlagsForceMulticast,
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
        return NS_NOTFOUND;
    }
}



static int
_mdns_gethtbyname(void *cbrv, void *cbdata, va_list ap)
{
    const char *name;
    int namelen, af, addrlen, rrtype;
    DNSServiceErrorType err;
    DNSServiceRef sdRef;

    UNUSED(cbdata);
    
    name = va_arg(ap, char *);
    namelen = va_arg(ap, int);
    af = va_arg(ap, int);

    UNUSED(namelen);
    
    //XXX Check if in valid name space
    
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

    _mdns_hostent_init(&h_ctx, af, addrlen);
    _mdns_hostent_add_host(&h_ctx, name);

    err = DNSServiceQueryRecord(
        &sdRef,
        kDNSServiceFlagsForceMulticast,
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
        *(struct hostent **)cbrv = _mdns_hostent_done(&h_ctx);
    
        return NS_SUCCESS;
    } else {
        return NS_NOTFOUND;
    }
}



static void
_mdns_addrinfo_init(addrinfo_ctx *ctx, const struct addrinfo *ai)
{
    ctx->cb_ctx.done = 0;
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
    p = &(ctx->start);

    while (p->ai_next) {
        if (p->ai_next->ai_family == AF_INET6) {
            t->ai_next = p->ai_next;
            t = t->ai_next;
            p->ai_next = p->ai_next->ai_next;
        } else {
            p = p->ai_next;
        }
    }

    /* add rest of list and reset start to the new list*/
    t->ai_next = ctx->start.ai_next;
    ctx->start.ai_next = head.ai_next;
    
    return ctx->start.ai_next;
}



static void
_mdns_hostent_init(hostent_ctx *ctx, int af, int addrlen)
{
    int i;
    
    ctx->cb_ctx.done = ctx->naliases = ctx->naddrs = 0;
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
    size_t len = strlen(name);
    
    if (name && (HCTX_BUFLEFT(ctx) > len) && (ctx->naliases < MAXALIASES)) {
        /* skip dupe names */
        if ((ctx->host.h_name) && !strcmp(ctx->host.h_name, name)) {
            return;
        }
    
        strcpy(ctx->next, name);
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
            ctx->cb_ctx.done = 1;
        }

        ai = allocaddrinfo((socklen_t)(address->sa_len));
        if (ai) {
            ai->ai_flags = ctx->start.ai_flags;
            ai->ai_family = address->sa_family;
            ai->ai_socktype = ctx->start.ai_socktype;
            ai->ai_protocol = ctx->start.ai_protocol;
            memcpy(ai->ai_addr, address, (size_t)(address->sa_len));
            
            //XXX canonname?
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
        ctx->cb_ctx.done = 1;
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
        ret = poll(&fds, 1, 500);
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
