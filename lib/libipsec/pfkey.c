/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("@(#) pfkey.c $Revision: 1.2 $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netkey/keyv2.h>
#include <netkey/key_var.h>
#include <netinet/in.h>
#include <netinet6/in6.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ipsec_strerror.h"

#define CALLOC(size, cast) (cast)calloc(1, (size))

static int pfkey_send_x1( int so, u_int type, u_int satype,
	struct sockaddr *src, u_int prefs,
	struct sockaddr *dst, u_int prefd, u_int proto,
	struct sockaddr *proxy,
	u_int32_t spi, caddr_t keymat,
	u_int e_type, u_int e_keylen, u_int a_type, u_int a_keylen,
	u_int flags,
	u_int32_t l_alloc, u_int32_t l_bytes,
	u_int32_t l_addtime, u_int32_t l_usetime, u_int32_t seq);
static int pfkey_send_x2( int so, u_int type, u_int satype,
	struct sockaddr *src, u_int prefs,
	struct sockaddr *dst, u_int prefd, u_int proto,
	u_int32_t spi);

static int pfkey_send_x3(int so, u_int type, u_int satype);

static caddr_t pfkey_setsadbaddr(caddr_t buf, u_int type,
			u_int family, caddr_t addr,
			u_int pref, u_int proto, u_int port);
static caddr_t pfkey_setsadbkey(caddr_t buf, u_int type,
			caddr_t key, u_int keylen);
static caddr_t pfkey_setsadbsa(caddr_t buf, caddr_t spi,
			u_int a_type, u_int e_type, u_int flags);
static caddr_t pfkey_setsadblifetime(caddr_t buf, u_int type,
		u_int32_t l_alloc, u_int32_t l_bytes,
		u_int32_t l_addtime, u_int32_t l_usetime);

/*
 * check key length against algorithm specified.
 * supported is either SADB_EXT_SUPPORTED_ENCRYPT or SADB_EXT_SUPPORTED_AUTH.
 * Refer to keyv2.h to get more info.
 * keylen is the unit of bit.
 * OUT:
 *	-1: invalid.
 *	 0: valid.
 */
static struct sadb_msg *ipsec_supported = NULL;

int ipsec_check_keylen(u_int supported, u_int alg_id, u_int keylen)
{
	u_int tlen;
	caddr_t p;
	struct sadb_supported *sup;
	struct sadb_alg *alg;

	/* validity check */
	if (ipsec_supported == NULL) {
		ipsec_errcode = EIPSEC_DO_GET_SUPP_LIST;
		return -1;
	}
	switch (supported) {
	case SADB_EXT_SUPPORTED_AUTH:
	case SADB_EXT_SUPPORTED_ENCRYPT:
		break;
	default:
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}

	tlen = ipsec_supported->sadb_msg_len - sizeof(struct sadb_msg);
	p = (caddr_t)ipsec_supported + sizeof(struct sadb_msg);

	for (;
	     tlen > 0;
	     tlen -= sup->sadb_supported_len, p += sup->sadb_supported_len) {

		sup = (struct sadb_supported *)p;

		if (sup->sadb_supported_exttype != supported)
			continue;

	    {
		u_int ttlen = sup->sadb_supported_len;
		caddr_t pp = p + sizeof(*sup);

		for (;
		     ttlen > 0;
	     	     ttlen -= sizeof(*alg), pp += sizeof(*alg)) {
			alg = (struct sadb_alg *)pp;

			if (alg->sadb_alg_id == alg_id)
				goto found;
	    	}
	    }
	}

	ipsec_errcode = EIPSEC_NOT_SUPPORTED;
	return -1;
	/* NOTREACHED */

    found:
	if (keylen < alg->sadb_alg_minbits
	 || keylen > alg->sadb_alg_maxbits) {
		ipsec_errcode = EIPSEC_INVAL_KEYLEN;
		return -1;
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

/*
 * check basic usage for sadb_msg,
 * and set the pointer to each header in this message buffer.
 * NOTE: This routine is derived from netkey/key.c in KAME.
 * IN:	msg: pointer to message buffer.
 *	mhp: pointer to the buffer initialized like below:
 *
 *		caddr_t mhp[SADB_EXT_MAX + 1];
 *
 * OUT:	-1: invalid.
 *	 0: valid.
 */
int pfkey_check(struct sadb_msg *msg, caddr_t *mhp)
{
	/* validity check */
	if (msg == NULL || mhp == NULL) {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}

	/* initialize */
    {
	int i;
	for (i = 0; i < SADB_EXT_MAX + 1; i++)
		mhp[i] = NULL;
    }

	/* check version */
	if (msg->sadb_msg_version != PF_KEY_V2) {
		ipsec_errcode = EIPSEC_INVAL_VERSION;
		return -1;
	}

	/* check type */
	if (msg->sadb_msg_type > SADB_MAX) {
		ipsec_errcode = EIPSEC_INVAL_MSGTYPE;
		return -1;
	}

	/* check SA type */
	switch (msg->sadb_msg_satype) {
	case SADB_SATYPE_UNSPEC:
		if (msg->sadb_msg_type != SADB_REGISTER
		 && msg->sadb_msg_type != SADB_FLUSH
		 && msg->sadb_msg_type != SADB_DUMP
		 && msg->sadb_msg_type != SADB_X_PROMISC
		 && msg->sadb_msg_type != SADB_X_SPDADD
		 && msg->sadb_msg_type != SADB_X_SPDDELETE
		 && msg->sadb_msg_type != SADB_X_SPDDUMP
		 && msg->sadb_msg_type != SADB_X_SPDFLUSH) {
			ipsec_errcode = EIPSEC_INVAL_SATYPE;
			return -1;
		}

	case SADB_SATYPE_AH:
	case SADB_SATYPE_ESP:
	case SADB_X_SATYPE_IPCOMP:
		break;
	case SADB_SATYPE_RSVP:
	case SADB_SATYPE_OSPFV2:
	case SADB_SATYPE_RIPV2:
	case SADB_SATYPE_MIP:
		ipsec_errcode = EIPSEC_NOT_SUPPORTED;
		return -1;

	case 1:
		if (msg->sadb_msg_type == SADB_X_PROMISC)
			break;
		/* FALLTHROUGH */
	default:
		ipsec_errcode = EIPSEC_INVAL_SATYPE;
		return -1;
	}

	mhp[0] = (caddr_t)msg;

    {
	struct sadb_ext *ext;
	int tlen, extlen;

	tlen = PFKEY_UNUNIT64(msg->sadb_msg_len) - sizeof(struct sadb_msg);
	ext = (struct sadb_ext *)((caddr_t)msg + sizeof(struct sadb_msg));

	while (tlen > 0) {
		/* duplicate check */
		/* XXX Are there duplication either KEY_AUTH or KEY_ENCRYPT ?*/
		if (mhp[ext->sadb_ext_type] != NULL) {
			ipsec_errcode = EIPSEC_INVAL_EXTTYPE;
			return -1;
		}

		/* set pointer */
		switch (ext->sadb_ext_type) {
		case SADB_EXT_SA:
		case SADB_EXT_LIFETIME_CURRENT:
		case SADB_EXT_LIFETIME_HARD:
		case SADB_EXT_LIFETIME_SOFT:
		case SADB_EXT_ADDRESS_SRC:
		case SADB_EXT_ADDRESS_DST:
		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_KEY_AUTH:
			/* XXX should to be check weak keys. */
		case SADB_EXT_KEY_ENCRYPT:
			/* XXX should to be check weak keys. */
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		case SADB_EXT_SENSITIVITY:
		case SADB_EXT_PROPOSAL:
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
		case SADB_EXT_SPIRANGE:
		case SADB_X_EXT_POLICY:
			mhp[ext->sadb_ext_type] = (caddr_t)ext;
			break;
		default:
			ipsec_errcode = EIPSEC_INVAL_EXTTYPE;
			return -1;
		}

		extlen = PFKEY_UNUNIT64(ext->sadb_ext_len);
		tlen -= extlen;
		ext = (struct sadb_ext *)((caddr_t)ext + extlen);
	}
    }

	/* check field of upper layer protocol and address family */
	if (mhp[SADB_EXT_ADDRESS_SRC] != NULL
	 && mhp[SADB_EXT_ADDRESS_DST] != NULL) {
		struct sadb_address *src0, *dst0;
		struct sockaddr *src, *dst;

		src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
		dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);
		src = (struct sockaddr *)((caddr_t)src0 + sizeof(*src0));
		dst = (struct sockaddr *)((caddr_t)dst0 + sizeof(*dst0));

		if (src0->sadb_address_proto != dst0->sadb_address_proto) {
			ipsec_errcode = EIPSEC_PROTO_MISMATCH;
			return -1;
		}

		if (src->sa_family != dst->sa_family) {
			ipsec_errcode = EIPSEC_FAMILY_MISMATCH;
			return -1;
		}

		if (src->sa_family != AF_INET
		 && src->sa_family != AF_INET6) {
			ipsec_errcode = EIPSEC_INVAL_FAMILY;
			return -1;
		}

		/*
		 * prefixlen == 0 is valid because there must be the case
		 * all addresses are matched.
		 */
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

/*
 * set the rate for SOFT lifetime against HARD one.
 * If rate is more than 100 or equal to zero, then set to 100.
 */
static u_int soft_lifetime_allocations_rate = PFKEY_SOFT_LIFETIME_RATE;
static u_int soft_lifetime_bytes_rate = PFKEY_SOFT_LIFETIME_RATE;
static u_int soft_lifetime_addtime_rate = PFKEY_SOFT_LIFETIME_RATE;
static u_int soft_lifetime_usetime_rate = PFKEY_SOFT_LIFETIME_RATE;

u_int pfkey_set_softrate(u_int type, u_int rate)
{
	ipsec_errcode = EIPSEC_NO_ERROR;

	if (rate > 100 || rate == 0)
		rate = 100;

	switch (type) {
	case SADB_X_LIFETIME_ALLOCATIONS:
		soft_lifetime_allocations_rate = rate;
		return 0;
	case SADB_X_LIFETIME_BYTES:
		soft_lifetime_bytes_rate = rate;
		return 0;
	case SADB_X_LIFETIME_ADDTIME:
		soft_lifetime_addtime_rate = rate;
		return 0;
	case SADB_X_LIFETIME_USETIME:
		soft_lifetime_usetime_rate = rate;
		return 0;
	}

	ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
	return 1;
}

/*
 * get current rate for SOFT lifetime against HARD one.
 * ATTENTION: ~0 is returned if invalid type was passed.
 */
u_int pfkey_get_softrate(u_int type)
{
	switch (type) {
	case SADB_X_LIFETIME_ALLOCATIONS:
		return soft_lifetime_allocations_rate;
	case SADB_X_LIFETIME_BYTES:
		return soft_lifetime_bytes_rate;
	case SADB_X_LIFETIME_ADDTIME:
		return soft_lifetime_addtime_rate;
	case SADB_X_LIFETIME_USETIME:
		return soft_lifetime_usetime_rate;
	}

	return ~0;
}

/*
 * sending SADB_GETSPI message to the kernel.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_getspi(
	int so,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,
	u_int32_t min,
	u_int32_t max,
	u_int32_t seq)
{
	struct sadb_msg *newmsg;
	int len;
	int need_spirange = 0;
	caddr_t p;

	/* validity check */
	if (src == NULL || dst == NULL) {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}
	if (src->sa_family != dst->sa_family) {
		ipsec_errcode = EIPSEC_FAMILY_MISMATCH;
		return -1;
	}
	if (prefs > _INALENBYAF(src->sa_family) * 8
	 || prefd > _INALENBYAF(dst->sa_family) * 8) {
		ipsec_errcode = EIPSEC_INVAL_PREFIXLEN;
		return -1;
	}
	if (min > max || (min > 0 && min <= 255)) {
		ipsec_errcode = EIPSEC_INVAL_SPI;
		return -1;
	}

	/* create new sadb_msg to send. */
	len = sizeof(struct sadb_msg)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family));

	if (min > 255 && max < ~0) {
		need_spirange++;
		len += sizeof(struct sadb_spirange);
	}

	if ((newmsg = CALLOC(len, struct sadb_msg *)) == NULL) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = SADB_GETSPI;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = satype;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	newmsg->sadb_msg_seq = seq;
	newmsg->sadb_msg_pid = getpid();
	p = (caddr_t)newmsg + sizeof(*newmsg);

	/* set sadb_address for source */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
			src->sa_family,
			_INADDRBYSA(src),
			prefs,
			proto,
			_INPORTBYSA(src));

	/* set sadb_address for destination */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
			dst->sa_family,
			_INADDRBYSA(dst),
			prefd,
			proto,
			_INPORTBYSA(dst));

	/* proccessing spi range */
	if (need_spirange) {
		int _len = sizeof(struct sadb_spirange);

#define _SADB_SPIRANGE(p) ((struct sadb_spirange *)(p))
		_SADB_SPIRANGE(p)->sadb_spirange_len = PFKEY_UNIT64(_len);
		_SADB_SPIRANGE(p)->sadb_spirange_exttype = SADB_EXT_SPIRANGE;
		_SADB_SPIRANGE(p)->sadb_spirange_min = min;
		_SADB_SPIRANGE(p)->sadb_spirange_max = max;
#undef _SADB_SPIRANGE(p)
		p += _len;
	}

	/* send message */
	len = pfkey_send(so, newmsg, len);
	free(newmsg);

	if (len < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return len;
}

/*
 * sending SADB_UPDATE message to the kernel.
 * The length of key material is a_keylen + e_keylen.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_update(
	int so,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,
	struct sockaddr *proxy,
	u_int32_t spi,
	caddr_t keymat,
	u_int e_type,
	u_int e_keylen,
	u_int a_type,
	u_int a_keylen,
	u_int flags,
	u_int32_t l_alloc,
	u_int32_t l_bytes,
	u_int32_t l_addtime,
	u_int32_t l_usetime,
	u_int32_t seq)
{
	int len;
	if ((len = pfkey_send_x1(so, SADB_UPDATE,
			satype, src, prefs, dst, prefd, proto, proxy, spi,
			keymat, e_type, e_keylen, a_type, a_keylen, flags,
			l_alloc, l_bytes, l_addtime, l_usetime, seq)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_ADD message to the kernel.
 * The length of key material is a_keylen + e_keylen.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_add(
	int so,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,
	struct sockaddr *proxy,
	u_int32_t spi,
	caddr_t keymat,
	u_int e_type,
	u_int e_keylen,
	u_int a_type,
	u_int a_keylen,
	u_int flags,
	u_int32_t l_alloc,
	u_int32_t l_bytes,
	u_int32_t l_addtime,
	u_int32_t l_usetime,
	u_int32_t seq)
{
	int len;
	if ((len = pfkey_send_x1(so, SADB_ADD,
			satype, src, prefs, dst, prefd, proto, proxy, spi,
			keymat, e_type, e_keylen, a_type, a_keylen, flags,
			l_alloc, l_bytes, l_addtime, l_usetime, seq)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_DELETE message to the kernel.
 * The length of key material is a_keylen + e_keylen.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_delete(
	int so,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,
	u_int32_t spi)
{
	int len;
	if ((len = pfkey_send_x2(so, SADB_DELETE,
			satype, src, prefs, dst, prefd, proto, spi)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_GET message to the kernel.
 * The length of key material is a_keylen + e_keylen.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_get(
	int so,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,
	u_int32_t spi)
{
	int len;
	if ((len = pfkey_send_x2(so, SADB_GET,
			satype, src, prefs, dst, prefd, proto, spi)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_REGISTER message to the kernel.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_register(int so, u_int satype)
{
	int len;

	if ((len = pfkey_send_x3(so, SADB_REGISTER, satype)) < 0)
		return -1;

	return len;
}

/*
 * receiving SADB_REGISTER message from the kernel, and copy buffer for
 * sadb_supported returned into ipsec_supported.
 * OUT:
 *	 0: success and return length sent.
 *	-1: error occured, and set errno.
 */
int pfkey_recv_register(int so)
{
	pid_t pid = getpid();
	struct sadb_msg *newmsg;
	struct sadb_supported *sup;
	caddr_t p;
	int tlen;

	/* receive message */
	do {
		if ((newmsg = pfkey_recv(so)) == NULL)
			return -1;

	} while (newmsg->sadb_msg_type != SADB_REGISTER
	    || newmsg->sadb_msg_pid != pid);

	/* check and fix */
	newmsg->sadb_msg_len = PFKEY_UNUNIT64(newmsg->sadb_msg_len);

	tlen = newmsg->sadb_msg_len - sizeof(struct sadb_msg);
	p = (caddr_t)newmsg + sizeof(struct sadb_msg);
	while (tlen > 0) {
		sup = (struct sadb_supported *)p;
		switch (sup->sadb_supported_exttype) {
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
			sup->sadb_supported_len =
				PFKEY_UNUNIT64(sup->sadb_supported_len);
			break;
		default:
			ipsec_errcode = EIPSEC_INVAL_SATYPE;
			free(newmsg);
			return -1;
		}

		tlen -= sup->sadb_supported_len;
		p += sup->sadb_supported_len;
	}

	if (tlen < 0) {
		ipsec_errcode = EIPSEC_INVAL_SATYPE;
		return -1;
	}

	if (ipsec_supported != NULL)
		free(ipsec_supported);

	ipsec_supported = newmsg;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

/*
 * sending SADB_FLUSH message to the kernel.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_flush(int so, u_int satype)
{
	int len;

	if ((len = pfkey_send_x3(so, SADB_FLUSH, satype)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_DUMP message to the kernel.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 */
int pfkey_send_dump(int so, u_int satype)
{
	int len;

	if ((len = pfkey_send_x3(so, SADB_DUMP, satype)) < 0)
		return -1;

	return len;
}

/*
 * sending SADB_X_PROMISC message to the kernel.
 * NOTE that this function handles promisc mode toggle only.
 * IN:
 *	flag:	set promisc off if zero, set promisc on if non-zero.
 * OUT:
 *	positive: success and return length sent.
 *	-1	: error occured, and set errno.
 *	0     : error occured, and set errno.

 *	others: a pointer to new allocated buffer in which supported
 *	        algorithms is.
 */
int pfkey_send_promisc_toggle(int so, int flag)
{
	int len;

	if ((len = pfkey_send_x3(so, SADB_X_PROMISC, (flag ? 1 : 0))) < 0)
		return -1;

	return len;
}

/* sending SADB_ADD or SADB_UPDATE message to the kernel */
static int pfkey_send_x1(
	int so,
	u_int type,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,		/* upper layer protocol */
	struct sockaddr *proxy,
	u_int32_t spi,
	caddr_t keymat,
	u_int e_type,
	u_int e_keylen,
	u_int a_type,
	u_int a_keylen,
	u_int flags,
	u_int32_t l_alloc,
	u_int32_t l_bytes,
	u_int32_t l_addtime,
	u_int32_t l_usetime,
	u_int32_t seq)
{
	struct sadb_msg *newmsg;
	int len;
	caddr_t p;

	/* validity check */
	if (src == NULL || dst == NULL) {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}
	if (src->sa_family != dst->sa_family) {
		ipsec_errcode = EIPSEC_FAMILY_MISMATCH;
		return -1;
	}
	if (prefs > _INALENBYAF(src->sa_family) * 8
	 || prefd > _INALENBYAF(dst->sa_family) * 8) {
		ipsec_errcode = EIPSEC_INVAL_PREFIXLEN;
		return -1;
	}

	switch (satype) {
	case SADB_SATYPE_ESP:
		if (e_type == SADB_EALG_NONE) {
			ipsec_errcode = EIPSEC_NO_ALGS;
			return -1;
		}
		break;
	case SADB_SATYPE_AH:
		if (e_type != SADB_EALG_NONE) {
			ipsec_errcode = EIPSEC_INVAL_ALGS;
			return -1;
		}
		if (a_type == SADB_AALG_NONE) {
			ipsec_errcode = EIPSEC_NO_ALGS;
			return -1;
		}
		break;
	case SADB_X_SATYPE_IPCOMP:
		break;
	default:
		ipsec_errcode = EIPSEC_INVAL_SATYPE;
		return -1;
	}

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
		+ sizeof(struct sadb_sa)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family))
		+ sizeof(struct sadb_lifetime)
		+ sizeof(struct sadb_lifetime);

	if (proxy != NULL)
		len += (sizeof(struct sadb_address)
			+ PFKEY_ALIGN8(_SALENBYAF(proxy->sa_family)));
	if (e_type != SADB_EALG_NONE)
		len += (sizeof(struct sadb_key) + PFKEY_ALIGN8(e_keylen));
	if (a_type != SADB_AALG_NONE)
		len += (sizeof(struct sadb_key) + PFKEY_ALIGN8(a_keylen));

	if ((newmsg = CALLOC(len, struct sadb_msg *)) == NULL) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = type;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = satype;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	newmsg->sadb_msg_seq = seq;
	newmsg->sadb_msg_pid = getpid();
	p = (caddr_t)newmsg + sizeof(*newmsg);

	/* set sadb_sa */
	p = pfkey_setsadbsa(p, (caddr_t)&spi, a_type, e_type, flags);

	/* set sadb_address for source */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
			src->sa_family,
			_INADDRBYSA(src),
			prefs,
			proto,
			_INPORTBYSA(src));

	/* set sadb_address for destination */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
			dst->sa_family,
			_INADDRBYSA(dst),
			prefd,
			proto,
			_INPORTBYSA(dst));

	if (e_type != SADB_EALG_NONE)
		p = pfkey_setsadbkey(p, SADB_EXT_KEY_ENCRYPT,
		                   keymat, e_keylen);
	if (a_type != SADB_AALG_NONE)
		p = pfkey_setsadbkey(p, SADB_EXT_KEY_AUTH,
		                   keymat + e_keylen, a_keylen);

	/* set sadb_address for proxy, if present */
	if (proxy != NULL)
		p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_PROXY,
				proxy->sa_family,
				_INADDRBYSA(proxy),
				_INALENBYAF(proxy->sa_family) << 3,
				0,
				0);

	/* set sadb_lifetime for destination */
	p = pfkey_setsadblifetime(p, SADB_EXT_LIFETIME_HARD,
			l_alloc, l_bytes, l_addtime, l_usetime);
	p = pfkey_setsadblifetime(p, SADB_EXT_LIFETIME_SOFT,
			l_alloc, l_bytes, l_addtime, l_usetime);

	/* send message */
	len = pfkey_send(so, newmsg, len);
	free(newmsg);

	if (len < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return len;
}

/* sending SADB_DELETE or SADB_GET message to the kernel */
static int pfkey_send_x2(
	int so,
	u_int type,
	u_int satype,
	struct sockaddr *src,
	u_int prefs,
	struct sockaddr *dst,
	u_int prefd,
	u_int proto,		/* upper layer protocol */
	u_int32_t spi)
{
	struct sadb_msg *newmsg;
	int len;
	caddr_t p;

	/* validity check */
	if (src == NULL || dst == NULL) {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}
	if (src->sa_family != dst->sa_family) {
		ipsec_errcode = EIPSEC_FAMILY_MISMATCH;
		return -1;
	}
	if (prefs > _INALENBYAF(src->sa_family) * 8
	 || prefd > _INALENBYAF(dst->sa_family) * 8) {
		ipsec_errcode = EIPSEC_INVAL_PREFIXLEN;
		return -1;
	}

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
		+ sizeof(struct sadb_sa)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(src->sa_family));

	if ((newmsg = CALLOC(len, struct sadb_msg *)) == NULL) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = type;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = satype;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	newmsg->sadb_msg_seq = 0;
	newmsg->sadb_msg_pid = getpid();
	p = (caddr_t)newmsg + sizeof(*newmsg);

	/* set sadb_sa */
	p = pfkey_setsadbsa(p, (caddr_t)&spi, 0, 0, 0);

	/* set sadb_address for source */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
			src->sa_family,
			_INADDRBYSA(src),
			prefs,
			proto,
			_INPORTBYSA(src));

	/* set sadb_address for destination */
	p = pfkey_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
			dst->sa_family,
			_INADDRBYSA(dst),
			prefd,
			proto,
			_INPORTBYSA(dst));

	/* send message */
	len = pfkey_send(so, newmsg, len);
	free(newmsg);

	if (len < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return len;
}

/*
 * sending SADB_REGISTER, SADB_FLUSH, SADB_DUMP or SADB_X_PROMISC message
 * to the kernel
 */
static int pfkey_send_x3(int so, u_int type, u_int satype)
{
	struct sadb_msg *newmsg;
	int len;

	/* validity check */
	switch (type) {
	case SADB_X_PROMISC:
		if (satype != 0 && satype != 1) {
			ipsec_errcode = EIPSEC_INVAL_SATYPE;
			return -1;
		}
		break;
	default:
		switch (satype) {
		case SADB_SATYPE_UNSPEC:
		case SADB_SATYPE_AH:
		case SADB_SATYPE_ESP:
		case SADB_X_SATYPE_IPCOMP:
			break;
		default:
			ipsec_errcode = EIPSEC_INVAL_SATYPE;
			return -1;
		}
	}

	/* create new sadb_msg to send. */
	len = sizeof(struct sadb_msg);

	if ((newmsg = CALLOC(len, struct sadb_msg *)) == NULL) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = type;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = satype;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	newmsg->sadb_msg_seq = 0;
	newmsg->sadb_msg_pid = getpid();

	/* send message */
	len = pfkey_send(so, newmsg, len);
	free(newmsg);

	if (len < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return len;
}

/*
 * open a socket.
 * OUT:
 *	-1: fail.
 *	others : success and return value of socket.
 */
int pfkey_open(void)
{
	int so;

	if ((so = socket(PF_KEY, SOCK_RAW, PF_KEY_V2)) < 0) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return so;
}

/*
 * close a socket.
 * OUT:
 *	 0: success.
 *	-1: fail.
 */
void pfkey_close(int so)
{
	(void)close(so);

	ipsec_errcode = EIPSEC_NO_ERROR;
	return;
}

/*
 * receive sadb_msg data, and return pointer to new buffer allocated.
 * Must free this buffer later.
 * OUT:
 *	NULL	: error occured.
 *	others	: a pointer to sadb_msg structure.
 */
struct sadb_msg *pfkey_recv(int so)
{
	struct sadb_msg buf, *newmsg;
	int len, reallen;

	while ((len = recv(so, (caddr_t)&buf, sizeof(buf), MSG_PEEK)) < 0) {
		if (errno == EINTR) continue;
		ipsec_set_strerror(strerror(errno));
		return NULL;
	}

	if (len < sizeof(buf)) {
		recv(so, (caddr_t)&buf, sizeof(buf), 0);
		ipsec_errcode = EIPSEC_MAX;
		return NULL;
	}

	/* read real message */
	reallen = PFKEY_UNUNIT64(buf.sadb_msg_len);
	if ((newmsg = CALLOC(reallen, struct sadb_msg *)) == 0) {
		ipsec_set_strerror(strerror(errno));
		return NULL;
	}

	while ((len = recv(so, (caddr_t)newmsg, reallen, 0)) < 0) {
		if (errno == EINTR) continue;
		ipsec_set_strerror(strerror(errno));
		free(newmsg);
		return NULL;
	}

	if (len != reallen) {
		ipsec_errcode = EIPSEC_MAX;
		free(newmsg);
		return NULL;
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return newmsg;
}

/*
 * send message to a socket.
 * OUT:
 *	 others: success and return length sent.
 *	-1     : fail.
 */
int pfkey_send(int so, struct sadb_msg *msg, int len)
{
	if ((len = send(so, (caddr_t)msg, len, 0)) < 0) {
		ipsec_set_strerror(strerror(errno));
		return -1;
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return len;
}

/* %%% */
/*
 * set sadb_address structure after clearing buffer with zero.
 * OUT: the pointer of buf + len.
 */
static caddr_t pfkey_setsadbaddr(caddr_t buf, u_int type,
			u_int family, caddr_t addr,
			u_int pref, u_int proto, u_int port)
{
	caddr_t p = buf; /* save */
	int len;

	len = sizeof(struct sadb_address) + PFKEY_ALIGN8(_SALENBYAF(family));
	memset(p, 0, len);

#define _SADB_ADDRESS(p) ((struct sadb_address *)(p))
	_SADB_ADDRESS(p)->sadb_address_len = PFKEY_UNIT64(len);
	_SADB_ADDRESS(p)->sadb_address_exttype = type;
	_SADB_ADDRESS(p)->sadb_address_proto = proto;
	_SADB_ADDRESS(p)->sadb_address_prefixlen = pref;
#undef _SADB_ADDRESS(p)
	p += sizeof(struct sadb_address);

	((struct sockaddr *)p)->sa_len = _SALENBYAF(family);
	((struct sockaddr *)p)->sa_family = family;
	_INPORTBYSA(p) = port;
	memcpy(_INADDRBYSA(p), addr, _INALENBYAF(family));

	return buf + len;
}

/*
 * set sadb_key structure after clearing buffer with zero.
 * OUT: the pointer of buf + len.
 */
static caddr_t pfkey_setsadbkey(caddr_t buf, u_int type,
			caddr_t key, u_int keylen)
{
	caddr_t p = buf; /* save */
	int len, aligned_len;

	aligned_len = PFKEY_ALIGN8(keylen);
	len = sizeof(struct sadb_key) + aligned_len;
	memset(p, 0, len);

#define _SADB_KEY(p) ((struct sadb_key *)(p))
	_SADB_KEY(p)->sadb_key_len = PFKEY_UNIT64(len);
	_SADB_KEY(p)->sadb_key_exttype = type;
	_SADB_KEY(p)->sadb_key_bits = keylen << 3;
	_SADB_KEY(p)->sadb_key_reserved = 0;
#undef _SADB_KEY(p)
	p += sizeof(struct sadb_key);

	memcpy(p, key, keylen);
	p += aligned_len;

	return buf + len;
}

/*
 * set sadb_sa structure after clearing buffer with zero.
 * OUT: the pointer of buf + len.
 */
static caddr_t pfkey_setsadbsa(caddr_t buf, caddr_t spi,
			u_int a_type, u_int e_type, u_int flags)
{
	caddr_t p = buf;
	int len;

	len = sizeof(struct sadb_sa);
	memset(p, 0, len);

#define _SADB_SA(p) ((struct sadb_sa *)(p))
	_SADB_SA(p)->sadb_sa_len = PFKEY_UNIT64(len);
	_SADB_SA(p)->sadb_sa_exttype = SADB_EXT_SA;
	memcpy(&_SADB_SA(p)->sadb_sa_spi, spi, PFKEY_SPI_SIZE);
	_SADB_SA(p)->sadb_sa_replay = 0;
	_SADB_SA(p)->sadb_sa_state = 0;
	_SADB_SA(p)->sadb_sa_auth = a_type;
	_SADB_SA(p)->sadb_sa_encrypt = e_type;
	_SADB_SA(p)->sadb_sa_flags = flags;
#undef _SADB_SA(p)

	return buf + len;
}

/*
 * set sadb_lifetime structure after clearing buffer with zero.
 * OUT: the pointer of buf + len.
 */
static caddr_t pfkey_setsadblifetime(caddr_t buf, u_int type,
		u_int32_t l_alloc, u_int32_t l_bytes,
		u_int32_t l_addtime, u_int32_t l_usetime)
{
	caddr_t p = buf;
	int len;

	len = sizeof(struct sadb_lifetime);
	memset(p, 0, len);

#define _SADB_LIFETIME(p) ((struct sadb_lifetime *)(p))
	_SADB_LIFETIME(p)->sadb_lifetime_len = PFKEY_UNIT64(len);
	_SADB_LIFETIME(p)->sadb_lifetime_exttype = type;

	switch (type) {
	case SADB_EXT_LIFETIME_SOFT:
		_SADB_LIFETIME(p)->sadb_lifetime_allocations
			= (l_alloc * soft_lifetime_allocations_rate) /100;
		_SADB_LIFETIME(p)->sadb_lifetime_bytes
			= ((l_bytes * soft_lifetime_bytes_rate) /100) << 10;
		_SADB_LIFETIME(p)->sadb_lifetime_addtime
			= (l_addtime * soft_lifetime_addtime_rate) /100;
		_SADB_LIFETIME(p)->sadb_lifetime_usetime
			= (l_usetime * soft_lifetime_usetime_rate) /100;
		break;
	case SADB_EXT_LIFETIME_HARD:
		_SADB_LIFETIME(p)->sadb_lifetime_allocations = l_alloc;
		_SADB_LIFETIME(p)->sadb_lifetime_bytes = l_bytes << 10;
		_SADB_LIFETIME(p)->sadb_lifetime_addtime = l_addtime;
		_SADB_LIFETIME(p)->sadb_lifetime_usetime = l_usetime;
		break;
	}
#undef _SADB_LIFETIME(p)

	return buf + len;
}

