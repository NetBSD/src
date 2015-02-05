#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv6.c,v 1.1.1.11.2.2 2015/02/05 15:13:12 martin Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#ifndef __linux__
#  ifndef __QNX__
#    include <sys/endian.h>
#  endif
#  include <net/if.h>
#  ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#    include <net/if_var.h>
#  endif
#  ifndef __sun
#    include <netinet6/in6_var.h>
#  endif
#endif

#include <errno.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define ELOOP_QUEUE 7
#include "common.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "eloop.h"
#include "if.h"
#include "ipv6.h"
#include "ipv6nd.h"

#ifdef HAVE_MD5_H
#  ifndef DEPGEN
#    include <md5.h>
#  endif
#else
#  include "md5.h"
#endif

#ifdef SHA2_H
#  include SHA2_H
#else
#  include "sha256.h"
#endif

#ifndef SHA256_DIGEST_LENGTH
#  define SHA256_DIGEST_LENGTH		32
#endif

#ifdef IPV6_POLLADDRFLAG
#  warning kernel does not report IPv6 address flag changes
#  warning polling tentative address flags periodically instead
#endif

#ifdef __linux__
   /* Match Linux defines to BSD */
#  define IN6_IFF_TEMPORARY IFA_F_TEMPORARY
#  ifdef IFA_F_OPTIMISTIC
#    define IN6_IFF_TENTATIVE	(IFA_F_TENTATIVE | IFA_F_OPTIMISTIC)
#  else
#    define IN6_IFF_TENTATIVE   (IFA_F_TENTATIVE | 0x04)
#  endif
#  ifdef IF_F_DADFAILED
#    define IN6_IFF_DUPLICATED	IFA_F_DADFAILED
#  else
#    define IN6_IFF_DUPLICATED	0x08
#  endif
#  define IN6_IFF_DETACHED	0
#endif

#define IN6_IFF_NOTUSEABLE \
	(IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED | IN6_IFF_DETACHED)

/* Hackery at it's finest. */
#ifndef s6_addr32
#  ifdef __sun
#    define s6_addr32	_S6_un._S6_u32
#  else
#    define s6_addr32	__u6_addr.__u6_addr32
#  endif
#endif


#ifdef IPV6_MANAGETEMPADDR
static void ipv6_regentempifid(void *);
static void ipv6_regentempaddr(void *);
#else
#define ipv6_regentempifid(a) {}
#endif

struct ipv6_ctx *
ipv6_init(struct dhcpcd_ctx *dhcpcd_ctx)
{
	struct ipv6_ctx *ctx;

	if (dhcpcd_ctx->ipv6)
		return dhcpcd_ctx->ipv6;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return NULL;

	ctx->routes = malloc(sizeof(*ctx->routes));
	if (ctx->routes == NULL) {
		free(ctx);
		return NULL;
	}
	TAILQ_INIT(ctx->routes);

	ctx->ra_routers = malloc(sizeof(*ctx->ra_routers));
	if (ctx->ra_routers == NULL) {
		free(ctx->routes);
		free(ctx);
		return NULL;
	}
	TAILQ_INIT(ctx->ra_routers);

	ctx->sndhdr.msg_namelen = sizeof(struct sockaddr_in6);
	ctx->sndhdr.msg_iov = ctx->sndiov;
	ctx->sndhdr.msg_iovlen = 1;
	ctx->sndhdr.msg_control = ctx->sndbuf;
	ctx->sndhdr.msg_controllen = sizeof(ctx->sndbuf);
	ctx->rcvhdr.msg_name = &ctx->from;
	ctx->rcvhdr.msg_namelen = sizeof(ctx->from);
	ctx->rcvhdr.msg_iov = ctx->rcviov;
	ctx->rcvhdr.msg_iovlen = 1;
	ctx->rcvhdr.msg_control = ctx->rcvbuf;
	// controllen is set at recieve
	//ctx->rcvhdr.msg_controllen = sizeof(ctx->rcvbuf);
	ctx->rcviov[0].iov_base = ctx->ansbuf;
	ctx->rcviov[0].iov_len = sizeof(ctx->ansbuf);

	ctx->nd_fd = -1;
	ctx->dhcp_fd = -1;

	dhcpcd_ctx->ipv6 = ctx;

	return ctx;
}

ssize_t
ipv6_printaddr(char *s, size_t sl, const uint8_t *d, const char *ifname)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	size_t l;

	p = inet_ntop(AF_INET6, d, buf, sizeof(buf));
	if (p == NULL)
		return -1;

	l = strlen(p);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80)
		l += 1 + strlen(ifname);

	if (s == NULL)
		return (ssize_t)l;

	if (sl < l) {
		errno = ENOMEM;
		return -1;
	}

	s += strlcpy(s, p, sl);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80) {
		*s++ = '%';
		s += strlcpy(s, ifname, sl);
	}
	*s = '\0';
	return (ssize_t)l;
}

static ssize_t
ipv6_readsecret(struct dhcpcd_ctx *ctx)
{
	FILE *fp;
	char line[1024];
	unsigned char *p;
	size_t len;
	uint32_t r;
	int x;

	if ((fp = fopen(SECRET, "r"))) {
		len = 0;
		while (fgets(line, sizeof(line), fp)) {
			len = strlen(line);
			if (len) {
				if (line[len - 1] == '\n')
					line[len - 1] = '\0';
			}
			len = hwaddr_aton(NULL, line);
			if (len) {
				ctx->secret_len = hwaddr_aton(ctx->secret,
				    line);
				break;
			}
			len = 0;
		}
		fclose(fp);
		if (len)
			return (ssize_t)len;
	} else {
		if (errno != ENOENT)
			syslog(LOG_ERR, "error reading secret: %s: %m", SECRET);
	}

	/* Chaining arc4random should be good enough.
	 * RFC7217 section 5.1 states the key SHOULD be at least 128 bits.
	 * To attempt and future proof ourselves, we'll generate a key of
	 * 512 bits (64 bytes). */
	p = ctx->secret;
	ctx->secret_len = 0;
	for (len = 0; len < 512 / NBBY; len += sizeof(r)) {
		r = arc4random();
		memcpy(p, &r, sizeof(r));
		p += sizeof(r);
		ctx->secret_len += sizeof(r);

	}

	/* Ensure that only the dhcpcd user can read the secret.
	 * Write permission is also denied as chaning it would remove
	 * it's stability. */
	if ((fp = fopen(SECRET, "w")) == NULL ||
	    chmod(SECRET, S_IRUSR) == -1)
		goto eexit;
	x = fprintf(fp, "%s\n",
	    hwaddr_ntoa(ctx->secret, ctx->secret_len, line, sizeof(line)));
	fclose(fp);
	if (x > 0)
		return (ssize_t)ctx->secret_len;

eexit:
	syslog(LOG_ERR, "error writing secret: %s: %m", SECRET);
	unlink(SECRET);
	ctx->secret_len = 0;
	return -1;
}

/* http://www.iana.org/assignments/ipv6-interface-ids/ipv6-interface-ids.xhtml
 * RFC5453 */
static const struct reslowhigh {
	const uint8_t high[8];
	const uint8_t low[8];
} reslowhigh[] = {
	/* RFC4291 + RFC6543 */
	{ { 0x02, 0x00, 0x5e, 0xff, 0xfe, 0x00, 0x00, 0x00 },
	  { 0x02, 0x00, 0x5e, 0xff, 0xfe, 0xff, 0xff, 0xff } },
	/* RFC2526 */
	{ { 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80 },
	  { 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } }
};

static int
ipv6_reserved(const struct in6_addr *addr)
{
	uint64_t id, low, high;
	size_t i;
	const struct reslowhigh *r;

	id = be64dec(addr->s6_addr + sizeof(id));
	if (id == 0) /* RFC4291 */
		return 1;
	for (i = 0; i < sizeof(reslowhigh) / sizeof(reslowhigh[0]); i++) {
		r = &reslowhigh[i];
		low = be64dec(r->low);
		high = be64dec(r->high);
		if (id >= low && id <= high)
			return 1;
	}
	return 0;
}

/* RFC7217 */
static int
ipv6_makestableprivate1(struct in6_addr *addr,
    const struct in6_addr *prefix, int prefix_len,
    const unsigned char *netiface, size_t netiface_len,
    const unsigned char *netid, size_t netid_len,
    uint32_t *dad_counter,
    const unsigned char *secret, size_t secret_len)
{
	unsigned char buf[2048], *p, digest[SHA256_DIGEST_LENGTH];
	size_t len, l;
	SHA256_CTX ctx;

	if (prefix_len < 0 || prefix_len > 120) {
		errno = EINVAL;
		return -1;
	}

	l = (size_t)(ROUNDUP8(prefix_len) / NBBY);
	len = l + netiface_len + netid_len + sizeof(*dad_counter) + secret_len;
	if (len > sizeof(buf)) {
		errno = ENOBUFS;
		return -1;
	}

	for (;; (*dad_counter)++) {
		/* Combine all parameters into one buffer */
		p = buf;
		memcpy(p, prefix, l);
		p += l;
		memcpy(p, netiface, netiface_len);
		p += netiface_len;
		memcpy(p, netid, netid_len);
		p += netid_len;
		memcpy(p, dad_counter, sizeof(*dad_counter));
		p += sizeof(*dad_counter);
		memcpy(p, secret, secret_len);

		/* Make an address using the digest of the above.
		 * RFC7217 Section 5.1 states that we shouldn't use MD5.
		 * Pity as we use that for HMAC-MD5 which is still deemed OK.
		 * SHA-256 is recommended */
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, buf, len);
		SHA256_Final(digest, &ctx);

		p = addr->s6_addr;
		memcpy(p, prefix, l);
		/* RFC7217 section 5.2 says we need to start taking the id from
		 * the least significant bit */
		len = sizeof(addr->s6_addr) - l;
		memcpy(p + l, digest + (sizeof(digest) - len), len);

		/* Ensure that the Interface ID does not match a reserved one,
		 * if it does then treat it as a DAD failure.
		 * RFC7217 section 5.2 */
		if (prefix_len != 64)
			break;
		if (!ipv6_reserved(addr))
			break;
	}

	return 0;
}

int
ipv6_makestableprivate(struct in6_addr *addr,
    const struct in6_addr *prefix, int prefix_len,
    const struct interface *ifp,
    int *dad_counter)
{
	uint32_t dad;
	int r;

	dad = (uint32_t)*dad_counter;

	/* For our implementation, we shall set the hardware address
	 * as the interface identifier */
	r = ipv6_makestableprivate1(addr, prefix, prefix_len,
	    ifp->hwaddr, ifp->hwlen,
	    ifp->ssid, ifp->ssid_len,
	    &dad,
	    ifp->ctx->secret, ifp->ctx->secret_len);

	if (r == 0)
		*dad_counter = (int)dad;
	return r;
}

int
ipv6_makeaddr(struct in6_addr *addr, const struct interface *ifp,
    const struct in6_addr *prefix, int prefix_len)
{
	const struct ipv6_addr *ap;
	int dad;

	if (prefix_len < 0 || prefix_len > 120) {
		errno = EINVAL;
		return -1;
	}

	if (ifp->options->options & DHCPCD_SLAACPRIVATE) {
		if (ifp->ctx->secret_len == 0) {
			if (ipv6_readsecret(ifp->ctx) == -1)
				return -1;
		}
		dad = 0;
		if (ipv6_makestableprivate(addr,
		    prefix, prefix_len, ifp, &dad) == -1)
			return -1;
		return dad;
	}

	if (prefix_len > 64) {
		errno = EINVAL;
		return -1;
	}
	if ((ap = ipv6_linklocal(ifp)) == NULL) {
		/* We delay a few functions until we get a local-link address
		 * so this should never be hit. */
		errno = ENOENT;
		return -1;
	}

	/* Make the address from the first local-link address */
	memcpy(addr, prefix, sizeof(*prefix));
	addr->s6_addr32[2] = ap->addr.s6_addr32[2];
	addr->s6_addr32[3] = ap->addr.s6_addr32[3];
	return 0;
}

int
ipv6_makeprefix(struct in6_addr *prefix, const struct in6_addr *addr, int len)
{
	int bytelen, bitlen;

	if (len < 0 || len > 128) {
		errno = EINVAL;
		return -1;
	}

	bytelen = len / NBBY;
	bitlen = len % NBBY;
	memcpy(&prefix->s6_addr, &addr->s6_addr, (size_t)bytelen);
	if (bitlen != 0)
		prefix->s6_addr[bytelen] >>= NBBY - bitlen;
	memset((char *)prefix->s6_addr + bytelen, 0,
	    sizeof(prefix->s6_addr) - (size_t)bytelen);
	return 0;
}

int
ipv6_mask(struct in6_addr *mask, int len)
{
	static const unsigned char masks[NBBY] =
	    { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
	int bytes, bits, i;

	if (len < 0 || len > 128) {
		errno = EINVAL;
		return -1;
	}

	memset(mask, 0, sizeof(*mask));
	bytes = len / NBBY;
	bits = len % NBBY;
	for (i = 0; i < bytes; i++)
		mask->s6_addr[i] = 0xff;
	if (bits)
		mask->s6_addr[bytes] = masks[bits - 1];
	return 0;
}

uint8_t
ipv6_prefixlen(const struct in6_addr *mask)
{
	int x = 0, y;
	const unsigned char *lim, *p;

	lim = (const unsigned char *)mask + sizeof(*mask);
	for (p = (const unsigned char *)mask; p < lim; x++, p++) {
		if (*p != 0xff)
			break;
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < NBBY; y++) {
			if ((*p & (0x80 >> y)) == 0)
				break;
		}
	}

	/*
	 * when the limit pointer is given, do a stricter check on the
	 * remaining bits.
	 */
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0)
			return 0;
		for (p = p + 1; p < lim; p++)
			if (*p != 0)
				return 0;
	}

	return (uint8_t)(x * NBBY + y);
}

static void
in6_to_h64(uint64_t *vhigh, uint64_t *vlow, const struct in6_addr *addr)
{

	*vhigh = be64dec(addr->s6_addr);
	*vlow = be64dec(addr->s6_addr + 8);
}

static void
h64_to_in6(struct in6_addr *addr, uint64_t vhigh, uint64_t vlow)
{

	be64enc(addr->s6_addr, vhigh);
	be64enc(addr->s6_addr + 8, vlow);
}

int
ipv6_userprefix(
	const struct in6_addr *prefix,	// prefix from router
	short prefix_len,		// length of prefix received
	uint64_t user_number,		// "random" number from user
	struct in6_addr *result,	// resultant prefix
	short result_len)		// desired prefix length
{
	uint64_t vh, vl, user_low, user_high;

	if (prefix_len < 0 || prefix_len > 120 ||
	    result_len < 0 || result_len > 120)
	{
		errno = EINVAL;
		return -1;
	}

	/* Check that the user_number fits inside result_len less prefix_len */
	if (result_len < prefix_len || user_number > INT_MAX ||
	    ffs((int)user_number) > result_len - prefix_len)
	{
	       errno = ERANGE;
	       return -1;
	}

	/* virtually shift user number by dest_len, then split at 64 */
	if (result_len >= 64) {
		user_high = user_number << (result_len - 64);
		user_low = 0;
	} else {
		user_high = user_number >> (64 - result_len);
		user_low = user_number << result_len;
	}

	/* convert to two 64bit host order values */
	in6_to_h64(&vh, &vl, prefix);

	vh |= user_high;
	vl |= user_low;

	/* copy back result */
	h64_to_in6(result, vh, vl);

	return 0;
}

#ifdef IPV6_POLLADDRFLAG
void
ipv6_checkaddrflags(void *arg)
{
	struct ipv6_addr *ap;
	int ifa_flags;

	ap = arg;
	ifa_flags = if_addrflags6(&ap->addr, ap->iface);
	if (ifa_flags == -1)
		syslog(LOG_ERR, "%s: if_addrflags6: %m", ap->iface->name);
	else if (!(ifa_flags & IN6_IFF_TENTATIVE)) {
		ipv6_handleifa(ap->iface->ctx, RTM_NEWADDR,
		    ap->iface->ctx->ifaces, ap->iface->name,
		    &ap->addr, ap->prefix_len, ifa_flags);
	} else {
		struct timeval tv;

		ms_to_tv(&tv, RETRANS_TIMER / 2);
		eloop_timeout_add_tv(ap->iface->ctx->eloop, &tv,
		    ipv6_checkaddrflags, ap);
	}
}
#endif


static void
ipv6_deleteaddr(struct ipv6_addr *addr)
{
	struct ipv6_state *state;
	struct ipv6_addr *ap;

	syslog(LOG_INFO, "%s: deleting address %s",
	    addr->iface->name, addr->saddr);
	if (if_deladdress6(addr) == -1 &&
	    errno != EADDRNOTAVAIL && errno != ENXIO && errno != ENODEV)
		syslog(LOG_ERR, "if_deladdress6: :%m");

	state = IPV6_STATE(addr->iface);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (IN6_ARE_ADDR_EQUAL(&ap->addr, &addr->addr)) {
			TAILQ_REMOVE(&state->addrs, ap, next);
			free(ap);
			break;
		}
	}
}

int
ipv6_addaddr(struct ipv6_addr *ap, const struct timeval *now)
{
	struct interface *ifp;
	struct ipv6_state *state;
	struct ipv6_addr *nap;
	uint32_t pltime, vltime;

	/* Ensure no other interface has this address */
	TAILQ_FOREACH(ifp, ap->iface->ctx->ifaces, next) {
		if (ifp == ap->iface || strcmp(ifp->name, ap->iface->name) == 0)
			continue;
		state = IPV6_STATE(ifp);
		if (state == NULL)
			continue;
		TAILQ_FOREACH(nap, &state->addrs, next) {
			if (IN6_ARE_ADDR_EQUAL(&nap->addr, &ap->addr)) {
				ipv6_deleteaddr(nap);
				break;
			}
		}
	}

	if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
	    ipv6_iffindaddr(ap->iface, &ap->addr))
		ap->flags |= IPV6_AF_DADCOMPLETED;

	syslog(ap->flags & IPV6_AF_NEW ? LOG_INFO : LOG_DEBUG,
	    "%s: adding address %s", ap->iface->name, ap->saddr);
	if (ap->prefix_pltime == ND6_INFINITE_LIFETIME &&
	    ap->prefix_vltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: pltime infinity, vltime infinity",
		    ap->iface->name);
	else if (ap->prefix_pltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: pltime infinity, vltime %"PRIu32" seconds",
		    ap->iface->name, ap->prefix_vltime);
	else if (ap->prefix_vltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: pltime %"PRIu32"seconds, vltime infinity",
		    ap->iface->name, ap->prefix_pltime);
	else
		syslog(LOG_DEBUG,
		    "%s: pltime %"PRIu32" seconds, vltime %"PRIu32" seconds",
		    ap->iface->name, ap->prefix_pltime, ap->prefix_vltime);

	/* Adjust plftime and vltime based on acquired time */
	pltime = ap->prefix_pltime;
	vltime = ap->prefix_vltime;
	if (timerisset(&ap->acquired) &&
	    (ap->prefix_pltime != ND6_INFINITE_LIFETIME ||
	    ap->prefix_vltime != ND6_INFINITE_LIFETIME))
	{
		struct timeval n;

		if (now == NULL) {
			get_monotonic(&n);
			now = &n;
		}
		timersub(now, &ap->acquired, &n);
		if (ap->prefix_pltime != ND6_INFINITE_LIFETIME)
			ap->prefix_pltime -= (uint32_t)n.tv_sec;
		if (ap->prefix_vltime != ND6_INFINITE_LIFETIME)
			ap->prefix_vltime -= (uint32_t)n.tv_sec;
	}

	if (if_addaddress6(ap) == -1) {
		syslog(LOG_ERR, "if_addaddress6: %m");
#if 0
		syslog(LOG_DEBUG,
		    "%s: adj pltime %"PRIu32" seconds, "
		    "vltime %"PRIu32" seconds",
		    ap->iface->name, ap->prefix_pltime, ap->prefix_vltime);
#endif
		/* Restore real pltime and vltime */
		ap->prefix_pltime = pltime;
		ap->prefix_vltime = vltime;
		return -1;
	}

#ifdef IPV6_MANAGETEMPADDR
	/* RFC4941 Section 3.4 */
	if (ap->flags & IPV6_AF_TEMPORARY &&
	    ap->prefix_pltime &&
	    ap->prefix_vltime &&
	    ap->iface->options->options & DHCPCD_IPV6RA_OWN &&
	    ip6_use_tempaddr(ap->iface->name))
		eloop_timeout_add_sec(ap->iface->ctx->eloop,
		    (time_t)ap->prefix_pltime - REGEN_ADVANCE,
		    ipv6_regentempaddr, ap);
#endif

	/* Restore real pltime and vltime */
	ap->prefix_pltime = pltime;
	ap->prefix_vltime = vltime;

	ap->flags &= ~IPV6_AF_NEW;
	ap->flags |= IPV6_AF_ADDED;
	if (ap->delegating_iface)
		ap->flags |= IPV6_AF_DELEGATED;
	if (ap->iface->options->options & DHCPCD_IPV6RA_OWN &&
	    ipv6_removesubnet(ap->iface, ap) == -1)
		syslog(LOG_ERR,"ipv6_removesubnet: %m");

#ifdef IPV6_POLLADDRFLAG
	eloop_timeout_delete(ap->iface->ctx->eloop,
		ipv6_checkaddrflags, ap);
	if (!(ap->flags & IPV6_AF_DADCOMPLETED)) {
		struct timeval tv;

		ms_to_tv(&tv, RETRANS_TIMER / 2);
		eloop_timeout_add_tv(ap->iface->ctx->eloop,
		    &tv, ipv6_checkaddrflags, ap);
	}
#endif

	return 0;
}

struct ipv6_addr *
ipv6_findaddr(struct dhcpcd_ctx *ctx, const struct in6_addr *addr, short flags)
{
	struct ipv6_addr *dap, *nap;

	dap = dhcp6_findaddr(ctx, addr, flags);
	nap = ipv6nd_findaddr(ctx, addr, flags);
	if (!dap && !nap)
		return NULL;
	if (dap && !nap)
		return dap;
	if (nap && !dap)
		return nap;
	if (nap->iface->metric < dap->iface->metric)
		return nap;
	return dap;
}

ssize_t
ipv6_addaddrs(struct ipv6_addrhead *addrs)
{
	struct ipv6_addr *ap, *apn, *apf;
	ssize_t i;
	struct timeval now;

	i = 0;
	timerclear(&now);
	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (ap->prefix_vltime == 0) {
			if (ap->flags & IPV6_AF_ADDED) {
				ipv6_deleteaddr(ap);
				i++;
			}
			eloop_q_timeout_delete(ap->iface->ctx->eloop,
			    0, NULL, ap);
			if (ap->flags & IPV6_AF_REQUEST) {
				ap->flags &= ~IPV6_AF_ADDED;
			} else {
				TAILQ_REMOVE(addrs, ap, next);
				free(ap);
			}
		} else if (!(ap->flags & IPV6_AF_STALE) &&
		    !IN6_IS_ADDR_UNSPECIFIED(&ap->addr))
		{
			apf = ipv6_findaddr(ap->iface->ctx,
			    &ap->addr, IPV6_AF_ADDED);
			if (apf && apf->iface != ap->iface &&
			    strcmp(apf->iface->name, ap->iface->name))
			{
				if (apf->iface->metric <= ap->iface->metric) {
					syslog(LOG_INFO,
					    "%s: preferring %s on %s",
					    ap->iface->name,
					    ap->saddr,
					    apf->iface->name);
					continue;
				}
				syslog(LOG_INFO,
				    "%s: preferring %s on %s",
				    apf->iface->name,
				    ap->saddr,
				    ap->iface->name);
				if (if_deladdress6(apf) == -1 &&
				    errno != EADDRNOTAVAIL && errno != ENXIO)
					syslog(LOG_ERR, "if_deladdress6: %m");
				apf->flags &=
				    ~(IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED);
			} else if (apf)
				apf->flags &= ~IPV6_AF_ADDED;
			if (ap->flags & IPV6_AF_NEW)
				i++;
			if (!timerisset(&now))
				get_monotonic(&now);
			ipv6_addaddr(ap, &now);
		}
	}

	return i;
}

void
ipv6_freedrop_addrs(struct ipv6_addrhead *addrs, int drop,
    const struct interface *ifd)
{
	struct ipv6_addr *ap, *apn, *apf;
	struct timeval now;

	timerclear(&now);
	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (ifd && ap->delegating_iface != ifd)
			continue;
		if (drop != 2)
			TAILQ_REMOVE(addrs, ap, next);
		eloop_q_timeout_delete(ap->iface->ctx->eloop, 0, NULL, ap);
		if (drop && ap->flags & IPV6_AF_ADDED &&
		    (ap->iface->options->options &
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			if (drop == 2)
				TAILQ_REMOVE(addrs, ap, next);
			/* Find the same address somewhere else */
			apf = ipv6_findaddr(ap->iface->ctx, &ap->addr, 0);
			if (apf == NULL ||
			    (apf->iface != ap->iface &&
			    strcmp(apf->iface->name, ap->iface->name)))
				ipv6_deleteaddr(ap);
			if (!(ap->iface->options->options &
			    DHCPCD_EXITING) && apf)
			{
				if (!timerisset(&now))
					get_monotonic(&now);
				ipv6_addaddr(apf, &now);
			}
			if (drop == 2)
				free(ap);
		}
		if (drop != 2)
			free(ap);
	}
}

static struct ipv6_state *
ipv6_getstate(struct interface *ifp)
{
	struct ipv6_state *state;

	state = IPV6_STATE(ifp);
	if (state == NULL) {
	        ifp->if_data[IF_DATA_IPV6] = calloc(1, sizeof(*state));
		state = IPV6_STATE(ifp);
		if (state == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		TAILQ_INIT(&state->addrs);
		TAILQ_INIT(&state->ll_callbacks);

		/* Regenerate new ids */
		if (ifp->options &&
		    ifp->options->options & DHCPCD_IPV6RA_OWN &&
		    ip6_use_tempaddr(ifp->name))
			ipv6_regentempifid(ifp);
	}
	return state;
}

void
ipv6_handleifa(struct dhcpcd_ctx *ctx,
    int cmd, struct if_head *ifs, const char *ifname,
    const struct in6_addr *addr, uint8_t prefix_len, int flags)
{
	struct interface *ifp;
	struct ipv6_state *state;
	struct ipv6_addr *ap;
	struct ll_callback *cb;

#if 0
	char buf[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &addr->s6_addr,
	    buf, INET6_ADDRSTRLEN);
	syslog(LOG_DEBUG, "%s: cmd %d addr %s flags %d",
	    ifname, cmd, buf, flags);
#endif

	if (ifs == NULL)
		ifs = ctx->ifaces;
	if (ifs == NULL) {
		errno = ESRCH;
		return;
	}
	TAILQ_FOREACH(ifp, ifs, next) {
		/* Each psuedo interface also stores addresses */
		if (strcmp(ifp->name, ifname))
			continue;
		state = ipv6_getstate(ifp);
		if (state == NULL)
			continue;

		if (!IN6_IS_ADDR_LINKLOCAL(addr)) {
			ipv6nd_handleifa(ctx, cmd, ifname, addr, flags);
			dhcp6_handleifa(ctx, cmd, ifname, addr, flags);
		}

		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (IN6_ARE_ADDR_EQUAL(&ap->addr, addr))
				break;
		}

		switch (cmd) {
		case RTM_DELADDR:
			if (ap) {
				TAILQ_REMOVE(&state->addrs, ap, next);
				free(ap);
			}
			break;
		case RTM_NEWADDR:
			if (ap == NULL) {
				char buf[INET6_ADDRSTRLEN];
				const char *cbp;

				ap = calloc(1, sizeof(*ap));
				ap->iface = ifp;
				ap->addr = *addr;
				ap->prefix_len = prefix_len;
				ipv6_makeprefix(&ap->prefix, &ap->addr,
				    ap->prefix_len);
				cbp = inet_ntop(AF_INET6, &addr->s6_addr,
				    buf, sizeof(buf));
				if (cbp)
					snprintf(ap->saddr, sizeof(ap->saddr),
					    "%s/%d", cbp, prefix_len);
				if (if_getlifetime6(ap) == -1) {
					/* No support or address vanished.
					 * Either way, just set a deprecated
					 * infinite time lifetime and continue.
					 * This is fine because we only want
					 * to know this when trying to extend
					 * temporary addresses.
					 * As we can't extend infinite, we'll
					 * create a new temporary address. */
					ap->prefix_pltime = 0;
					ap->prefix_vltime =
					    ND6_INFINITE_LIFETIME;
				}
				/* This is a minor regression against RFC 4941
				 * because the kernel only knows when the
				 * lifetimes were last updated, not when the
				 * address was initially created.
				 * Provided dhcpcd is not restarted, this
				 * won't be a problem.
				 * If we don't like it, we can always
				 * pretend lifetimes are infinite and always
				 * generate a new temporary address on
				 * restart. */
				ap->acquired = ap->created;
				TAILQ_INSERT_TAIL(&state->addrs,
				    ap, next);
			}
			ap->addr_flags = flags;
			if (ap->addr_flags & IN6_IFF_TEMPORARY)
				ap->flags |= IPV6_AF_TEMPORARY;
			if (IN6_IS_ADDR_LINKLOCAL(&ap->addr)) {
#ifdef IPV6_POLLADDRFLAG
				if (ap->addr_flags & IN6_IFF_TENTATIVE) {
					struct timeval tv;

					ms_to_tv(&tv, RETRANS_TIMER / 2);
					eloop_timeout_add_tv(
					    ap->iface->ctx->eloop,
					    &tv, ipv6_checkaddrflags, ap);
					break;
				}
#endif

				if (!(ap->addr_flags & IN6_IFF_NOTUSEABLE)) {
					/* Now run any callbacks.
					 * Typically IPv6RS or DHCPv6 */
					while ((cb =
					    TAILQ_FIRST(&state->ll_callbacks)))
					{
						TAILQ_REMOVE(
						    &state->ll_callbacks,
						    cb, next);
						cb->callback(cb->arg);
						free(cb);
					}
				}
			}
			break;
		}
	}
}

const struct ipv6_addr *
ipv6_iffindaddr(const struct interface *ifp, const struct in6_addr *addr)
{
	const struct ipv6_state *state;
	const struct ipv6_addr *ap;

	state = IPV6_CSTATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (addr == NULL) {
				if (IN6_IS_ADDR_LINKLOCAL(&ap->addr) &&
				    !(ap->addr_flags & IN6_IFF_NOTUSEABLE))
					return ap;
			} else {
				if (IN6_ARE_ADDR_EQUAL(&ap->addr, addr) &&
				    !(ap->addr_flags & IN6_IFF_TENTATIVE))
					return ap;
			}
		}
	}
	return NULL;
}

int
ipv6_addlinklocalcallback(struct interface *ifp,
    void (*callback)(void *), void *arg)
{
	struct ipv6_state *state;
	struct ll_callback *cb;

	state = ipv6_getstate(ifp);
	TAILQ_FOREACH(cb, &state->ll_callbacks, next) {
		if (cb->callback == callback && cb->arg == arg)
			break;
	}
	if (cb == NULL) {
		cb = malloc(sizeof(*cb));
		if (cb == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		cb->callback = callback;
		cb->arg = arg;
		TAILQ_INSERT_TAIL(&state->ll_callbacks, cb, next);
	}
	return 0;
}

static struct ipv6_addr *
ipv6_newlinklocal(struct interface *ifp)
{
	struct ipv6_addr *ap;

	ap = calloc(1, sizeof(*ap));
	if (ap != NULL) {
		ap->iface = ifp;
		ap->prefix.s6_addr32[0] = htonl(0xfe800000);
		ap->prefix.s6_addr32[1] = 0;
		ap->prefix_len = 64;
		ap->dadcounter = 0;
		ap->prefix_pltime = ND6_INFINITE_LIFETIME;
		ap->prefix_vltime = ND6_INFINITE_LIFETIME;
		ap->flags = IPV6_AF_NEW;
		ap->addr_flags = IN6_IFF_TENTATIVE;
	}
	return ap;
}

static const uint8_t allzero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t allone[8] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static int
ipv6_addlinklocal(struct interface *ifp)
{
	struct ipv6_state *state;
	struct ipv6_addr *ap, *ap2;
	int dadcounter;

	if (ipv6_linklocal(ifp))
		return 0;

	/* Check sanity before malloc */
	if (!(ifp->options->options & DHCPCD_SLAACPRIVATE)) {
		switch (ifp->family) {
		case ARPHRD_ETHER:
			/* Check for a valid hardware address */
			if (ifp->hwlen != 6 && ifp->hwlen != 8) {
				errno = ENOTSUP;
				return -1;
			}
			if (memcmp(ifp->hwaddr, allzero, ifp->hwlen) == 0 ||
			    memcmp(ifp->hwaddr, allone, ifp->hwlen) == 0)
			{
				errno = EINVAL;
				return -1;
			}
			break;
		default:
			errno = ENOTSUP;
			return -1;
		}
	}

	state = ipv6_getstate(ifp);
	if (state == NULL)
		return -1;

	ap = ipv6_newlinklocal(ifp);
	if (ap == NULL)
		return -1;

	if (ifp->options->options & DHCPCD_SLAACPRIVATE) {
		dadcounter = 0;
nextslaacprivate:
		if (ipv6_makestableprivate(&ap->addr,
			&ap->prefix, ap->prefix_len, ifp, &dadcounter) == -1)
		{
			free(ap);
			return -1;
		}
		ap->dadcounter = dadcounter;
	} else {
		memcpy(ap->addr.s6_addr, ap->prefix.s6_addr, ap->prefix_len);
		switch (ifp->family) {
		case ARPHRD_ETHER:
			if (ifp->hwlen == 6) {
				ap->addr.s6_addr[ 8] = ifp->hwaddr[0];
				ap->addr.s6_addr[ 9] = ifp->hwaddr[1];
				ap->addr.s6_addr[10] = ifp->hwaddr[2];
				ap->addr.s6_addr[11] = 0xff;
				ap->addr.s6_addr[12] = 0xfe;
				ap->addr.s6_addr[13] = ifp->hwaddr[3];
				ap->addr.s6_addr[14] = ifp->hwaddr[4];
				ap->addr.s6_addr[15] = ifp->hwaddr[5];
			} else if (ifp->hwlen == 8)
				memcpy(&ap->addr.s6_addr[8], ifp->hwaddr, 8);
			break;
		}

		/* Sanity check: g bit must not indciate "group" */
		if (EUI64_GROUP(&ap->addr)) {
			free(ap);
			errno = EINVAL;
			return -1;
		}
		EUI64_TO_IFID(&ap->addr);
	}

	/* Do we already have this address? */
	TAILQ_FOREACH(ap2, &state->addrs, next) {
		if (IN6_ARE_ADDR_EQUAL(&ap->addr, &ap2->addr)) {
			if (ap2->addr_flags & IN6_IFF_DUPLICATED) {
				if (ifp->options->options &
				    DHCPCD_SLAACPRIVATE)
				{
					dadcounter++;
					goto nextslaacprivate;
				}
				free(ap);
				errno = EADDRNOTAVAIL;
				return -1;
			}

			syslog(LOG_WARNING, "%s: waiting for %s to complete",
			    ap2->iface->name, ap2->saddr);
			free(ap);
			errno =	EEXIST;
			return 0;
		}
	}

	inet_ntop(AF_INET6, &ap->addr, ap->saddr, sizeof(ap->saddr));
	TAILQ_INSERT_TAIL(&state->addrs, ap, next);
	ipv6_addaddr(ap, NULL);
	return 1;
}

/* Ensure the interface has a link-local address */
int
ipv6_start(struct interface *ifp)
{
	const struct ipv6_state *state;
	const struct ipv6_addr *ap;

	/* We can't assign a link-locak address to this,
	 * the ppp process has to. */
	if (ifp->flags & IFF_POINTOPOINT)
		return 0;

	state = IPV6_CSTATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (IN6_IS_ADDR_LINKLOCAL(&ap->addr) &&
			    !(ap->addr_flags & IN6_IFF_DUPLICATED))
				break;
		}
		/* Regenerate new ids */
		if (ifp->options->options & DHCPCD_IPV6RA_OWN &&
		    ip6_use_tempaddr(ifp->name))
			ipv6_regentempifid(ifp);
	} else
		ap = NULL;

	if (ap == NULL && ipv6_addlinklocal(ifp) == -1)
		return -1;
	return 0;
}

void
ipv6_freedrop(struct interface *ifp, int drop)
{
	struct ipv6_state *state;
	struct ll_callback *cb;

	if (ifp == NULL)
		return;

	if ((state = IPV6_STATE(ifp)) == NULL)
		return;

	ipv6_freedrop_addrs(&state->addrs, drop ? 2 : 0, NULL);

	/* Becuase we need to cache the addresses we don't control,
	 * we only free the state on when NOT dropping addresses. */
	if (drop == 0) {
		while ((cb = TAILQ_FIRST(&state->ll_callbacks))) {
			TAILQ_REMOVE(&state->ll_callbacks, cb, next);
			free(cb);
		}
		free(state);
		ifp->if_data[IF_DATA_IPV6] = NULL;
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	}
}

void
ipv6_ctxfree(struct dhcpcd_ctx *ctx)
{
	struct rt6 *rt;

	if (ctx->ipv6 == NULL)
		return;

	while ((rt = TAILQ_FIRST(ctx->ipv6->routes))) {
		TAILQ_REMOVE(ctx->ipv6->routes, rt, next);
		free(rt);
	}
	free(ctx->ipv6->routes);
	free(ctx->ipv6->ra_routers);
	free(ctx->ipv6);
}

int
ipv6_handleifa_addrs(int cmd,
    struct ipv6_addrhead *addrs, const struct in6_addr *addr, int flags)
{
	struct ipv6_addr *ap, *apn;
	uint8_t found, alldadcompleted;

	alldadcompleted = 1;
	found = 0;
	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (!IN6_ARE_ADDR_EQUAL(addr, &ap->addr)) {
			if (ap->flags & IPV6_AF_ADDED &&
			    !(ap->flags & IPV6_AF_DADCOMPLETED))
				alldadcompleted = 0;
			continue;
		}
		switch (cmd) {
		case RTM_DELADDR:
			if (ap->flags & IPV6_AF_ADDED) {
				syslog(LOG_INFO, "%s: deleted address %s",
				    ap->iface->name, ap->saddr);
				ap->flags &= ~IPV6_AF_ADDED;
			}
			break;
		case RTM_NEWADDR:
			/* Safety - ignore tentative announcements */
			if (flags & (IN6_IFF_DETACHED |IN6_IFF_TENTATIVE))
				break;
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
				found++;
				if (flags & IN6_IFF_DUPLICATED)
					ap->flags |= IPV6_AF_DUPLICATED;
				else
					ap->flags &= ~IPV6_AF_DUPLICATED;
				if (ap->dadcallback)
					ap->dadcallback(ap);
				/* We need to set this here in-case the
				 * dadcallback function checks it */
				ap->flags |= IPV6_AF_DADCOMPLETED;
			}
			break;
		}
	}

	return alldadcompleted ? found : 0;
}

#ifdef IPV6_MANAGETEMPADDR
static const struct ipv6_addr *
ipv6_findaddrid(struct dhcpcd_ctx *ctx, uint8_t *addr)
{
	const struct interface *ifp;
	const struct ipv6_state *state;
	const struct ipv6_addr *ia;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if ((state = IPV6_CSTATE(ifp))) {
			TAILQ_FOREACH(ia, &state->addrs, next) {
				if (memcmp(&ia->addr.s6_addr[8], addr, 8) == 0)
					return ia;
			}
		}
	}
	return NULL;
}

static const uint8_t nullid[8];
static const uint8_t anycastid[8] = {
    0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80 };
static const uint8_t isatapid[4] = { 0x00, 0x00, 0x5e, 0xfe };

static void
ipv6_regen_desync(struct interface *ifp, int force)
{
	struct ipv6_state *state;
	time_t max;

	state = IPV6_STATE(ifp);

	/* RFC4941 Section 5 states that DESYNC_FACTOR must never be
	 * greater than TEMP_VALID_LIFETIME - REGEN_ADVANCE.
	 * I believe this is an error and it should be never be greateter than
	 * TEMP_PREFERRED_LIFETIME - REGEN_ADVANCE. */
	max = ip6_temp_preferred_lifetime(ifp->name) - REGEN_ADVANCE;
	if (state->desync_factor && !force && state->desync_factor < max)
		return;
	if (state->desync_factor == 0)
		state->desync_factor =
		    (time_t)arc4random_uniform(MIN(MAX_DESYNC_FACTOR,
		    (uint32_t)max));
	max = ip6_temp_preferred_lifetime(ifp->name) -
	    state->desync_factor - REGEN_ADVANCE;
	eloop_timeout_add_sec(ifp->ctx->eloop, max, ipv6_regentempifid, ifp);
}

void
ipv6_gentempifid(struct interface *ifp)
{
	struct ipv6_state *state;
	MD5_CTX md5;
	uint8_t seed[16], digest[16];
	int retry;

	state = IPV6_STATE(ifp);
	retry = 0;
	if (memcmp(nullid, state->randomseed0, sizeof(nullid)) == 0) {
		uint32_t r;

		r = arc4random();
		memcpy(seed, &r, sizeof(r));
		r = arc4random();
		memcpy(seed + sizeof(r), &r, sizeof(r));
	} else
		memcpy(seed, state->randomseed0, sizeof(state->randomseed0));

	memcpy(seed + sizeof(state->randomseed0),
	    state->randomseed1, sizeof(state->randomseed1));

again:
	/* RFC4941 Section 3.2.1.1
	 * Take the left-most 64bits and set bit 6 to zero */
	MD5Init(&md5);
	MD5Update(&md5, seed, sizeof(seed));
	MD5Final(digest, &md5);

	/* RFC4941 Section 3.2.1.1
	 * Take the left-most 64bits and set bit 6 to zero */
	memcpy(state->randomid, digest, sizeof(state->randomid));
	state->randomid[0] &= ~EUI64_UBIT;

	/* RFC4941 Section 3.2.1.4
	 * Reject reserved or existing id's */
	if (memcmp(nullid, state->randomid, sizeof(nullid)) == 0 ||
	    (memcmp(anycastid, state->randomid, 7) == 0 &&
	    (anycastid[7] & state->randomid[7]) == anycastid[7]) ||
	    memcmp(isatapid, state->randomid, sizeof(isatapid)) == 0 ||
	    ipv6_findaddrid(ifp->ctx, state->randomid))
	{
		if (++retry < GEN_TEMPID_RETRY_MAX) {
			memcpy(seed, digest + 8, 8);
			goto again;
		}
		memset(state->randomid, 0, sizeof(state->randomid));
	}

	/* RFC4941 Section 3.2.1.6
	 * Save the right-most 64bits of the digest */
	memcpy(state->randomseed0, digest + 8,
	    sizeof(state->randomseed0));
}

/* RFC4941 Section 3.3.7 */
static void
ipv6_tempdadcallback(void *arg)
{
	struct ipv6_addr *ia = arg;

	if (ia->flags & IPV6_AF_DUPLICATED) {
		struct ipv6_addr *ia1;
		struct timeval tv;

		if (++ia->dadcounter == TEMP_IDGEN_RETRIES) {
			syslog(LOG_ERR,
			    "%s: too many duplicate temporary addresses",
			    ia->iface->name);
			return;
		}
		get_monotonic(&tv);
		if ((ia1 = ipv6_createtempaddr(ia, &tv)) == NULL)
			syslog(LOG_ERR, "ipv6_createtempaddr: %m");
		else
			ia1->dadcounter = ia->dadcounter;
		ipv6_deleteaddr(ia);
		if (ia1)
			ipv6_addaddr(ia1, &ia1->acquired);
	}
}

struct ipv6_addr *
ipv6_createtempaddr(struct ipv6_addr *ia0, const struct timeval *now)
{
	struct ipv6_state *state;
	const struct ipv6_state *cstate;
	int genid;
	struct in6_addr addr, mask;
	uint32_t randid[2];
	const struct interface *ifp;
	const struct ipv6_addr *ap;
	struct ipv6_addr *ia;
	uint32_t i, trylimit;
	char buf[INET6_ADDRSTRLEN];
	const char *cbp;

	trylimit = TEMP_IDGEN_RETRIES;
	state = IPV6_STATE(ia0->iface);
	genid = 0;

	addr = ia0->addr;
	ipv6_mask(&mask, ia0->prefix_len);
	/* clear the old ifid */
	for (i = 0; i < 4; i++)
		addr.s6_addr32[i] &= mask.s6_addr32[i];

again:
	if (memcmp(state->randomid, nullid, sizeof(nullid)) == 0)
		genid = 1;
	if (genid) {
		memcpy(state->randomseed1, &ia0->addr.s6_addr[8],
		    sizeof(state->randomseed1));
		ipv6_gentempifid(ia0->iface);
		if (memcmp(state->randomid, nullid, sizeof(nullid)) == 0) {
			errno = EFAULT;
			return NULL;
		}
	}
	memcpy(&randid[0], state->randomid, sizeof(randid[0]));
	memcpy(&randid[1], state->randomid + sizeof(randid[1]),
	    sizeof(randid[2]));
	addr.s6_addr32[2] |= randid[0] & ~mask.s6_addr32[2];
	addr.s6_addr32[3] |= randid[1] & ~mask.s6_addr32[3];

	/* Ensure we don't already have it */
	TAILQ_FOREACH(ifp, ia0->iface->ctx->ifaces, next) {
		cstate = IPV6_CSTATE(ifp);
		if (cstate) {
			TAILQ_FOREACH(ap, &cstate->addrs, next) {
				if (IN6_ARE_ADDR_EQUAL(&ap->addr, &addr)) {
					if (--trylimit == 0) {
						errno = EEXIST;
						return NULL;
					}
					genid = 1;
					goto again;
				}
			}
		}
	}

	if ((ia = calloc(1, sizeof(*ia))) == NULL)
		return NULL;

	ia->iface = ia0->iface;
	ia->addr = addr;
	/* Must be made tentative, for our DaD to work */
	ia->addr_flags = IN6_IFF_TENTATIVE;
	ia->dadcallback = ipv6_tempdadcallback;
	ia->flags = IPV6_AF_NEW | IPV6_AF_AUTOCONF | IPV6_AF_TEMPORARY;
	ia->prefix = ia0->prefix;
	ia->prefix_len = ia0->prefix_len;
	ia->created = ia->acquired = now ? *now : ia0->acquired;

	/* Ensure desync is still valid */
	ipv6_regen_desync(ia->iface, 0);

	/* RFC4941 Section 3.3.4 */
	i = (uint32_t)(ip6_temp_preferred_lifetime(ia0->iface->name) -
	    state->desync_factor);
	ia->prefix_pltime = MIN(ia0->prefix_pltime, i);
	i = (uint32_t)ip6_temp_valid_lifetime(ia0->iface->name);
	ia->prefix_vltime = MIN(ia0->prefix_vltime, i);
	if (ia->prefix_pltime <= REGEN_ADVANCE ||
	    ia->prefix_pltime > ia0->prefix_vltime)
	{
		errno =	EINVAL;
		free(ia);
		return NULL;
	}

	cbp = inet_ntop(AF_INET6, &ia->addr, buf, sizeof(buf));
	if (cbp)
		snprintf(ia->saddr, sizeof(ia->saddr), "%s/%d",
		    cbp, ia->prefix_len);
	else
		ia->saddr[0] = '\0';

	TAILQ_INSERT_TAIL(&state->addrs, ia, next);
	return ia;
}

void
ipv6_settempstale(struct interface *ifp)
{
	struct ipv6_state *state;
	struct ipv6_addr *ia;

	state = IPV6_STATE(ifp);
	TAILQ_FOREACH(ia, &state->addrs, next) {
		if (ia->flags & IPV6_AF_TEMPORARY)
			ia->flags |= IPV6_AF_STALE;
	}
}

struct ipv6_addr *
ipv6_settemptime(struct ipv6_addr *ia, int flags)
{
	struct ipv6_state *state;
	struct ipv6_addr *ap, *first;

	state = IPV6_STATE(ia->iface);
	first = NULL;
	TAILQ_FOREACH_REVERSE(ap, &state->addrs, ipv6_addrhead, next) {
		if (ap->flags & IPV6_AF_TEMPORARY &&
		    ap->prefix_pltime &&
		    IN6_ARE_ADDR_EQUAL(&ia->prefix, &ap->prefix))
		{
			time_t max, ext;

			if (flags == 0) {
				if (ap->prefix_pltime -
				    (uint32_t)(ia->acquired.tv_sec -
				    ap->acquired.tv_sec)
				    < REGEN_ADVANCE)
					continue;

				return ap;
			}

			if (!(ap->flags & IPV6_AF_ADDED))
				ap->flags |= IPV6_AF_NEW | IPV6_AF_AUTOCONF;
			ap->flags &= ~IPV6_AF_STALE;

			/* RFC4941 Section 3.4
			 * Deprecated prefix, deprecate the temporary address */
			if (ia->prefix_pltime == 0) {
				ap->prefix_pltime = 0;
				goto valid;
			}

			/* Ensure desync is still valid */
			ipv6_regen_desync(ap->iface, 0);

			/* RFC4941 Section 3.3.2
			 * Extend temporary times, but ensure that they
			 * never last beyond the system limit. */
			ext = ia->acquired.tv_sec + (time_t)ia->prefix_pltime;
			max = ap->created.tv_sec +
			    ip6_temp_preferred_lifetime(ap->iface->name) -
			    state->desync_factor;
			if (ext < max)
				ap->prefix_pltime = ia->prefix_pltime;
			else
				ap->prefix_pltime =
				    (uint32_t)(max - ia->acquired.tv_sec);

valid:
			ext = ia->acquired.tv_sec + (time_t)ia->prefix_vltime;
			max = ap->created.tv_sec +
			    ip6_temp_valid_lifetime(ap->iface->name);
			if (ext < max)
				ap->prefix_vltime = ia->prefix_vltime;
			else
				ap->prefix_vltime =
				    (uint32_t)(max - ia->acquired.tv_sec);

			/* Just extend the latest matching prefix */
			ap->acquired = ia->acquired;

			/* If extending return the last match as
			 * it's the most current.
			 * If deprecating, deprecate any other addresses we
			 * may have, although this should not be needed */
			if (ia->prefix_pltime)
				return ap;
			if (first == NULL)
				first = ap;
		}
	}
	return first;
}

void
ipv6_addtempaddrs(struct interface *ifp, const struct timeval *now)
{
	struct ipv6_state *state;
	struct ipv6_addr *ia;

	state = IPV6_STATE(ifp);
	TAILQ_FOREACH(ia, &state->addrs, next) {
		if (ia->flags & IPV6_AF_TEMPORARY &&
		    !(ia->flags & IPV6_AF_STALE))
			ipv6_addaddr(ia, now);
	}
}

static void
ipv6_regentempaddr(void *arg)
{
	struct ipv6_addr *ia = arg, *ia1;
	struct timeval tv;

	syslog(LOG_DEBUG, "%s: regen temp addr %s",
	    ia->iface->name, ia->saddr);
	get_monotonic(&tv);
	ia1 = ipv6_createtempaddr(ia, &tv);
	if (ia1)
		ipv6_addaddr(ia1, &tv);
	else
		syslog(LOG_ERR, "ipv6_createtempaddr: %m");
}

static void
ipv6_regentempifid(void *arg)
{
	struct interface *ifp = arg;
	struct ipv6_state *state;

	state = IPV6_STATE(ifp);
	if (memcmp(state->randomid, nullid, sizeof(state->randomid)))
		ipv6_gentempifid(ifp);

	ipv6_regen_desync(ifp, 1);
}
#endif /* IPV6_MANAGETEMPADDR */

static struct rt6 *
find_route6(struct rt6_head *rts, const struct rt6 *r)
{
	struct rt6 *rt;

	TAILQ_FOREACH(rt, rts, next) {
		if (IN6_ARE_ADDR_EQUAL(&rt->dest, &r->dest) &&
#if HAVE_ROUTE_METRIC
		    (r->iface == NULL || rt->iface == NULL ||
		    rt->iface->metric == r->iface->metric) &&
#endif
		    IN6_ARE_ADDR_EQUAL(&rt->net, &r->net))
			return rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt6 *rt)
{
	char destbuf[INET6_ADDRSTRLEN];
	char gatebuf[INET6_ADDRSTRLEN];
	const char *ifname, *dest, *gate;

	ifname = rt->iface ? rt->iface->name : "(no iface)";
	dest = inet_ntop(AF_INET6, &rt->dest, destbuf, INET6_ADDRSTRLEN);
	gate = inet_ntop(AF_INET6, &rt->gate, gatebuf, INET6_ADDRSTRLEN);
	if (IN6_ARE_ADDR_EQUAL(&rt->gate, &in6addr_any))
		syslog(LOG_INFO, "%s: %s route to %s/%d", ifname, cmd,
		    dest, ipv6_prefixlen(&rt->net));
	else if (IN6_ARE_ADDR_EQUAL(&rt->dest, &in6addr_any) &&
	    IN6_ARE_ADDR_EQUAL(&rt->net, &in6addr_any))
		syslog(LOG_INFO, "%s: %s default route via %s", ifname, cmd,
		    gate);
	else
		syslog(LOG_INFO, "%s: %s%s route to %s/%d via %s", ifname, cmd,
		    rt->flags & RTF_REJECT ? " reject" : "",
		    dest, ipv6_prefixlen(&rt->net), gate);
}

/* If something other than dhcpcd removes a route,
 * we need to remove it from our internal table. */
int
ipv6_routedeleted(struct dhcpcd_ctx *ctx, const struct rt6 *rt)
{
	struct rt6 *f;

	if (ctx->ipv6 == NULL)
		return 0;

	f = find_route6(ctx->ipv6->routes, rt);
	if (f == NULL)
		return 0;
	desc_route("removing", f);
	TAILQ_REMOVE(ctx->ipv6->routes, f, next);
	free(f);
	return 1;
}

#define n_route(a)	 nc_route(1, a, a)
#define c_route(a, b)	 nc_route(0, a, b)
static int
nc_route(int add, struct rt6 *ort, struct rt6 *nrt)
{

	/* Don't set default routes if not asked to */
	if (IN6_IS_ADDR_UNSPECIFIED(&nrt->dest) &&
	    IN6_IS_ADDR_UNSPECIFIED(&nrt->net) &&
	    !(nrt->iface->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route(add ? "adding" : "changing", nrt);
	/* We delete and add the route so that we can change metric and
	 * prefer the interface. */
	if (if_delroute6(ort) == -1 && errno != ESRCH)
		syslog(LOG_ERR, "%s: if_delroute6: %m", ort->iface->name);
	if (if_addroute6(nrt) == 0)
		return 0;
	syslog(LOG_ERR, "%s: if_addroute6: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt6 *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = if_delroute6(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: if_delroute6: %m", rt->iface->name);
	return retval;
}

static struct rt6 *
make_route(const struct interface *ifp, const struct ra *rap)
{
	struct rt6 *r;

	r = calloc(1, sizeof(*r));
	if (r == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	r->iface = ifp;
	r->metric = ifp->metric;
	if (rap)
		r->mtu = rap->mtu;
	else
		r->mtu = 0;
	return r;
}

static struct rt6 *
make_prefix(const struct interface * ifp, const struct ra *rap,
    const struct ipv6_addr *addr)
{
	struct rt6 *r;

	if (addr == NULL || addr->prefix_len > 128) {
		errno = EINVAL;
		return NULL;
	}

	/* There is no point in trying to manage a /128 prefix,
	 * ones without a lifetime or ones not on link or delegated */
	if (addr->prefix_len == 128 ||
	    addr->prefix_vltime == 0 ||
	    !(addr->flags & (IPV6_AF_ONLINK | IPV6_AF_DELEGATEDPFX)))
		return NULL;

	/* Don't install a blackhole route when not creating bigger prefixes */
	if (addr->flags & IPV6_AF_DELEGATEDZERO)
		return NULL;

	r = make_route(ifp, rap);
	if (r == NULL)
		return NULL;
	r->dest = addr->prefix;
	ipv6_mask(&r->net, addr->prefix_len);
	if (addr->flags & IPV6_AF_DELEGATEDPFX) {
		r->flags |= RTF_REJECT;
		r->gate = in6addr_loopback;
	} else
		r->gate = in6addr_any;
	return r;
}

static struct rt6 *
make_router(const struct ra *rap)
{
	struct rt6 *r;

	r = make_route(rap->iface, rap);
	if (r == NULL)
		return NULL;
	r->dest = in6addr_any;
	r->net = in6addr_any;
	r->gate = rap->from;
	return r;
}

int
ipv6_removesubnet(struct interface *ifp, struct ipv6_addr *addr)
{
	struct rt6 *rt;
#if HAVE_ROUTE_METRIC
	struct rt6 *ort;
#endif
	int r;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	r = 0;
	rt = make_prefix(ifp, NULL, addr);
	if (rt) {
		rt->iface = ifp;
#ifdef __linux__
		rt->metric = 256;
#else
		rt->metric = 0;
#endif
#if HAVE_ROUTE_METRIC
		/* For some reason, Linux likes to re-add the subnet
		   route under the original metric.
		   I would love to find a way of stopping this! */
		if ((ort = find_route6(ifp->ctx->ipv6->routes, rt)) == NULL ||
		    ort->metric != rt->metric)
#else
		if (!find_route6(ifp->ctx->ipv6->routes, rt))
#endif
		{
			r = if_delroute6(rt);
			if (r == -1 && errno == ESRCH)
				r = 0;
		}
		free(rt);
	}
	return r;
}

#define RT_IS_DEFAULT(rtp) \
	(IN6_ARE_ADDR_EQUAL(&((rtp)->dest), &in6addr_any) &&		      \
	    IN6_ARE_ADDR_EQUAL(&((rtp)->net), &in6addr_any))

static void
ipv6_build_ra_routes(struct ipv6_ctx *ctx, struct rt6_head *dnr, int expired)
{
	struct rt6 *rt;
	const struct ra *rap;
	const struct ipv6_addr *addr;

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (rap->expired != expired)
			continue;
		if (rap->iface->options->options & DHCPCD_IPV6RA_OWN) {
			TAILQ_FOREACH(addr, &rap->addrs, next) {
				rt = make_prefix(rap->iface, rap, addr);
				if (rt)
					TAILQ_INSERT_TAIL(dnr, rt, next);
			}
		}
		if (rap->lifetime && rap->iface->options->options &
		    (DHCPCD_IPV6RA_OWN | DHCPCD_IPV6RA_OWN_DEFAULT))
		{
			rt = make_router(rap);
			if (rt)
				TAILQ_INSERT_TAIL(dnr, rt, next);
		}
	}
}

static void
ipv6_build_dhcp_routes(struct dhcpcd_ctx *ctx,
    struct rt6_head *dnr, enum DH6S dstate)
{
	const struct interface *ifp;
	const struct dhcp6_state *d6_state;
	const struct ipv6_addr *addr;
	struct rt6 *rt;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		d6_state = D6_CSTATE(ifp);
		if (d6_state && d6_state->state == dstate) {
			TAILQ_FOREACH(addr, &d6_state->addrs, next) {
				rt = make_prefix(ifp, NULL, addr);
				if (rt)
					TAILQ_INSERT_TAIL(dnr, rt, next);
			}
		}
	}
}

void
ipv6_buildroutes(struct dhcpcd_ctx *ctx)
{
	struct rt6_head dnr, *nrs;
	struct rt6 *rt, *rtn, *or;
	uint8_t have_default;
	unsigned long long o;

	TAILQ_INIT(&dnr);

	/* First add reachable routers and their prefixes */
	ipv6_build_ra_routes(ctx->ipv6, &dnr, 0);
#if HAVE_ROUTE_METRIC
	have_default = (TAILQ_FIRST(&dnr) != NULL);
#endif

	/* We have no way of knowing if prefixes added by DHCP are reachable
	 * or not, so we have to assume they are.
	 * Add bound before delegated so we can prefer interfaces better */
	ipv6_build_dhcp_routes(ctx, &dnr, DH6S_BOUND);
	ipv6_build_dhcp_routes(ctx, &dnr, DH6S_DELEGATED);

#if HAVE_ROUTE_METRIC
	/* If we have an unreachable router, we really do need to remove the
	 * route to it beause it could be a lower metric than a reachable
	 * router. Of course, we should at least have some routers if all
	 * are unreachable. */
	if (!have_default)
#endif
	/* Add our non-reachable routers and prefixes
	 * Unsure if this is needed, but it's a close match to kernel
	 * behaviour */
	ipv6_build_ra_routes(ctx->ipv6, &dnr, 1);

	nrs = malloc(sizeof(*nrs));
	if (nrs == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return;
	}
	TAILQ_INIT(nrs);
	have_default = 0;
	TAILQ_FOREACH_SAFE(rt, &dnr, next, rtn) {
		/* Is this route already in our table? */
		if (find_route6(nrs, rt) != NULL)
			continue;
		//rt->src.s_addr = ifp->addr.s_addr;
		/* Do we already manage it? */
		if ((or = find_route6(ctx->ipv6->routes, rt))) {
			if (or->iface != rt->iface ||
		//	    or->src.s_addr != ifp->addr.s_addr ||
			    !IN6_ARE_ADDR_EQUAL(&rt->gate, &or->gate) ||
			    rt->metric != or->metric)
			{
				if (c_route(or, rt) != 0)
					continue;
			}
			TAILQ_REMOVE(ctx->ipv6->routes, or, next);
			free(or);
		} else {
			if (n_route(rt) != 0)
				continue;
		}
		if (RT_IS_DEFAULT(rt))
			have_default = 1;
		TAILQ_REMOVE(&dnr, rt, next);
		TAILQ_INSERT_TAIL(nrs, rt, next);
	}

	/* Free any routes we failed to add/change */
	while ((rt = TAILQ_FIRST(&dnr))) {
		TAILQ_REMOVE(&dnr, rt, next);
		free(rt);
	}

	/* Remove old routes we used to manage
	 * If we own the default route, but not RA management itself
	 * then we need to preserve the last best default route we had */
	while ((rt = TAILQ_LAST(ctx->ipv6->routes, rt6_head))) {
		TAILQ_REMOVE(ctx->ipv6->routes, rt, next);
		if (find_route6(nrs, rt) == NULL) {
			o = rt->iface->options->options;
			if (!have_default &&
			    (o & DHCPCD_IPV6RA_OWN_DEFAULT) &&
			    !(o & DHCPCD_IPV6RA_OWN) &&
			    RT_IS_DEFAULT(rt))
				have_default = 1;
				/* no need to add it back to our routing table
				 * as we delete an exiting route when we add
				 * a new one */
			else if ((rt->iface->options->options &
				(DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
				(DHCPCD_EXITING | DHCPCD_PERSISTENT))
				d_route(rt);
		}
		free(rt);
	}

	free(ctx->ipv6->routes);
	ctx->ipv6->routes = nrs;
}
