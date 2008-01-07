/*	$NetBSD: kern_uuid.c,v 1.13 2008/01/07 16:13:49 ad Exp $	*/

/*
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: /repoman/r/ncvs/src/sys/kern/kern_uuid.c,v 1.7 2004/01/12 13:34:11 rse Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_uuid.c,v 1.13 2008/01/07 16:13:49 ad Exp $");

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/uuid.h>

/* NetBSD */
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/uio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

/*
 * See also:
 *	http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *	http://www.opengroup.org/onlinepubs/009629399/apdxa.htm
 *
 * Note that the generator state is itself an UUID, but the time and clock
 * sequence fields are written in the native byte order.
 */

/* XXX Do we have a similar ASSERT()? */
#define CTASSERT(x)

CTASSERT(sizeof(struct uuid) == 16);

/* We use an alternative, more convenient representation in the generator. */
struct uuid_private {
	union {
		uint64_t	ll;		/* internal. */
		struct {
			uint32_t	low;
			uint16_t	mid;
			uint16_t	hi;
		} x;
	} time;
	uint16_t	seq;			/* Big-endian. */
	uint16_t	node[UUID_NODE_LEN>>1];
};

CTASSERT(sizeof(struct uuid_private) == 16);

static struct uuid_private uuid_last;

/* "UUID generator mutex lock" */
static kmutex_t uuid_mutex;

void
uuid_init(void)
{

	mutex_init(&uuid_mutex, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Return the first MAC address we encounter or, if none was found,
 * construct a sufficiently random multicast address. We don't try
 * to return the same MAC address as previously returned. We always
 * generate a new multicast address if no MAC address exists in the
 * system.
 * It would be nice to know if 'ifnet' or any of its sub-structures
 * has been changed in any way. If not, we could simply skip the
 * scan and safely return the MAC address we returned before.
 */
static void
uuid_node(uint16_t *node)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	int i, s;

	s = splnet();
	KERNEL_LOCK(1, NULL);
	IFNET_FOREACH(ifp) {
		/* Walk the address list */
		IFADDR_FOREACH(ifa, ifp) {
			sdl = (struct sockaddr_dl*)ifa->ifa_addr;
			if (sdl != NULL && sdl->sdl_family == AF_LINK &&
			    sdl->sdl_type == IFT_ETHER) {
				/* Got a MAC address. */
				memcpy(node, CLLADDR(sdl), UUID_NODE_LEN);
				KERNEL_UNLOCK_ONE(NULL);
				splx(s);
				return;
			}
		}
	}
	KERNEL_UNLOCK_ONE(NULL);
	splx(s);

	for (i = 0; i < (UUID_NODE_LEN>>1); i++)
		node[i] = (uint16_t)arc4random();
	*((uint8_t*)node) |= 0x01;
}

/*
 * Get the current time as a 60 bit count of 100-nanosecond intervals
 * since 00:00:00.00, October 15,1582. We apply a magic offset to convert
 * the Unix time since 00:00:00.00, January 1, 1970 to the date of the
 * Gregorian reform to the Christian calendar.
 */
/*
 * At present, NetBSD has no timespec source, only timeval sources.  So,
 * we use timeval.
 */
static uint64_t
uuid_time(void)
{
	struct timeval tv;
	uint64_t xtime = 0x01B21DD213814000LL;

	microtime(&tv);
	xtime += (uint64_t)tv.tv_sec * 10000000LL;
	xtime += (uint64_t)(10 * tv.tv_usec);
	return (xtime & ((1LL << 60) - 1LL));
}

/*
 * Internal routine to actually generate the UUID.
 */
static void
uuid_generate(struct uuid_private *uuid, uint64_t *timep, int count)
{
	uint64_t xtime;

	mutex_enter(&uuid_mutex);

	uuid_node(uuid->node);
	xtime = uuid_time();
	*timep = xtime;

	if (uuid_last.time.ll == 0LL || uuid_last.node[0] != uuid->node[0] ||
	    uuid_last.node[1] != uuid->node[1] ||
	    uuid_last.node[2] != uuid->node[2])
		uuid->seq = (uint16_t)arc4random() & 0x3fff;
	else if (uuid_last.time.ll >= xtime)
		uuid->seq = (uuid_last.seq + 1) & 0x3fff;
	else
		uuid->seq = uuid_last.seq;

	uuid_last = *uuid;
	uuid_last.time.ll = (xtime + count - 1) & ((1LL << 60) - 1LL);

	mutex_exit(&uuid_mutex);
}

int
sys_uuidgen(struct lwp *l, const struct sys_uuidgen_args *uap, register_t *retval)
{
	struct uuid_private uuid;
	uint64_t xtime;
	int error;
	int i;

	/*
	 * Limit the number of UUIDs that can be created at the same time
	 * to some arbitrary number. This isn't really necessary, but I
	 * like to have some sort of upper-bound that's less than 2G :-)
	 * XXX needs to be tunable.
	 */
	if (SCARG(uap,count) < 1 || SCARG(uap,count) > 2048)
		return (EINVAL);

	/* XXX: pre-validate accessibility to the whole of the UUID store? */

	/* Generate the base UUID. */
	uuid_generate(&uuid, &xtime, SCARG(uap, count));

	/* Set sequence and variant and deal with byte order. */
	uuid.seq = htobe16(uuid.seq | 0x8000);

	/* XXX: this should copyout larger chunks at a time. */
	for (i = 0; i < SCARG(uap, count); xtime++, i++) {
		/* Set time and version (=1) and deal with byte order. */
		uuid.time.x.low = (uint32_t)xtime;
		uuid.time.x.mid = (uint16_t)(xtime >> 32);
		uuid.time.x.hi = ((uint16_t)(xtime >> 48) & 0xfff) | (1 << 12);
		error = copyout(&uuid, SCARG(uap,store) + i, sizeof(uuid));
		if (error != 0)
			return error;
	}

	return 0;
}

int
uuid_snprintf(char *buf, size_t sz, const struct uuid *uuid)
{
	const struct uuid_private *id;
	int cnt;

	id = (const struct uuid_private *)uuid;
	cnt = snprintf(buf, sz, "%08x-%04x-%04x-%04x-%04x%04x%04x",
	    id->time.x.low, id->time.x.mid, id->time.x.hi, be16toh(id->seq),
	    be16toh(id->node[0]), be16toh(id->node[1]), be16toh(id->node[2]));
	return (cnt);
}

int
uuid_printf(const struct uuid *uuid)
{
	char buf[UUID_STR_LEN];

	(void) uuid_snprintf(buf, sizeof(buf), uuid);
	printf("%s", buf);
	return (0);
}

/*
 * Encode/Decode UUID into octet-stream.
 *   http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *
 * 0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          time_low                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       time_mid                |         time_hi_and_version   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         node (2-5)                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

void
uuid_enc_le(void *buf, const struct uuid *uuid)
{
	uint8_t *p = buf;
	int i;

	le32enc(p, uuid->time_low);
	le16enc(p + 4, uuid->time_mid);
	le16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < _UUID_NODE_LEN; i++)
		p[10 + i] = uuid->node[i];
}

void
uuid_dec_le(void const *buf, struct uuid *uuid)
{
	const uint8_t *p = buf;
	int i;

	uuid->time_low = le32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = le16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = p[10 + i];
}

void
uuid_enc_be(void *buf, const struct uuid *uuid)
{
	uint8_t *p = buf;
	int i;

	be32enc(p, uuid->time_low);
	be16enc(p + 4, uuid->time_mid);
	be16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < _UUID_NODE_LEN; i++)
		p[10 + i] = uuid->node[i];
}

void
uuid_dec_be(void const *buf, struct uuid *uuid)
{
	const uint8_t *p = buf;
	int i;

	uuid->time_low = be32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = be16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = p[10 + i];
}
