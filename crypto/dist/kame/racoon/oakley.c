/*	$KAME: oakley.c,v 1.86 2001/07/17 05:02:50 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>	/* XXX for subjectaltname */
#include <netinet/in.h>	/* XXX for subjectaltname */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "str2val.h"
#include "plog.h"
#include "debug.h"

#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "localconf.h"
#include "remoteconf.h"
#include "policy.h"
#include "handler.h"
#include "ipsec_doi.h"
#include "algorithm.h"
#include "sainfo.h"
#include "proposal.h"
#include "crypto_openssl.h"
#include "dnssec.h"
#include "sockmisc.h"
#include "strnames.h"
#include "gcmalloc.h"

#ifdef HAVE_GSSAPI
#include "gssapi.h"
#endif

#define OUTBOUND_SA	0
#define INBOUND_SA	1

#define INITDHVAL(s, a, d, t)                                                  \
do {                                                                           \
	(a)->v = str2val((s), 16, &(a)->l);                                    \
	memset(&dhgroup[(d)], 0, sizeof(struct dhgroup));                      \
	dhgroup[(d)].type = (t);                                               \
	dhgroup[(d)].prime = vdup(a);                                          \
	dhgroup[(d)].gen1 = 2;                                                 \
	dhgroup[(d)].gen2 = 0;                                                 \
} while(0);

struct dhgroup dhgroup[MAXDHGROUP];

static vchar_t oakley_prime768;
static vchar_t oakley_prime1024;
static vchar_t oakley_prime1536;

static struct cipher_algorithm cipher[] = {
{ "NULL",	NULL,			NULL,			NULL, },
{ "des",	eay_des_encrypt,	eay_des_decrypt,	eay_des_weakkey, },
#ifdef HAVE_OPENSSL_IDEA_H
{ "idea",	eay_idea_encrypt,	eay_idea_decrypt,	eay_idea_weakkey, },
#else
{ "*dummy*",	NULL,			NULL,			NULL, },
#endif
{ "blowfish",	eay_bf_encrypt,		eay_bf_decrypt,		eay_bf_weakkey, },
#ifdef HAVE_OPENSSL_RC5_H
{ "rc5",	eay_rc5_encrypt,	eay_rc5_decrypt,	eay_rc5_weakkey, },
#else
{ "*dummy*",	NULL,			NULL,			NULL, },
#endif
{ "3des",	eay_3des_encrypt,	eay_3des_decrypt,	eay_3des_weakkey, },
{ "cast",	eay_cast_decrypt,	eay_cast_decrypt,	eay_cast_weakkey, },
};

static int oakley_compute_keymat_x __P((struct ph2handle *, int, int));
#ifdef HAVE_SIGNING_C
static int get_cert_fromlocal __P((struct ph1handle *, int));
static int oakley_check_certid __P((struct ph1handle *iph1));
static int check_typeofcertname __P((int, int));
static cert_t *save_certbuf __P((struct isakmp_gen *));
#endif

int
oakley_get_defaultlifetime()
{
	return OAKLEY_ATTR_SA_LD_SEC_DEFAULT;
}

void
oakley_dhinit()
{
	/* set DH MODP */
	INITDHVAL(OAKLEY_PRIME_MODP768, &oakley_prime768,
		OAKLEY_ATTR_GRP_DESC_MODP768, OAKLEY_ATTR_GRP_TYPE_MODP);
	INITDHVAL(OAKLEY_PRIME_MODP1024, &oakley_prime1024,
		OAKLEY_ATTR_GRP_DESC_MODP1024, OAKLEY_ATTR_GRP_TYPE_MODP);
	INITDHVAL(OAKLEY_PRIME_MODP1536, &oakley_prime1536,
		OAKLEY_ATTR_GRP_DESC_MODP1536, OAKLEY_ATTR_GRP_TYPE_MODP);
}

void
oakley_dhgrp_free(dhgrp)
	struct dhgroup *dhgrp;
{
	if (dhgrp->prime)
		vfree(dhgrp->prime);
	if (dhgrp->curve_a)
		vfree(dhgrp->curve_a);
	if (dhgrp->curve_b)
		vfree(dhgrp->curve_b);
	if (dhgrp->order)
		vfree(dhgrp->order);
	racoon_free(dhgrp);
}

/*
 * compute sharing secret of DH
 * IN:	*dh, *pub, *priv, *pub_p
 * OUT: **gxy
 */
int
oakley_dh_compute(dh, pub, priv, pub_p, gxy)
	const struct dhgroup *dh;
	vchar_t *pub, *priv, *pub_p, **gxy;
{
	if ((*gxy = vmalloc(dh->prime->l)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get DH buffer.\n");
		return -1;
	}

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	switch (dh->type) {
	case OAKLEY_ATTR_GRP_TYPE_MODP:
		if (eay_dh_compute(dh->prime, dh->gen1, pub, priv, pub_p, gxy) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to compute dh value.\n");
			return -1;
		}
		break;
	case OAKLEY_ATTR_GRP_TYPE_ECP:
	case OAKLEY_ATTR_GRP_TYPE_EC2N:
		plog(LLV_ERROR, LOCATION, NULL,
			"dh type %d isn't supported.\n", dh->type);
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid dh type %d.\n", dh->type);
		return -1;
	}

#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s): %8.6f", __FUNCTION__,
		s_attr_isakmp_group(dh->type), timedelta(&start, &end));
    }
#endif

	plog(LLV_DEBUG, LOCATION, NULL, "compute DH's shared.\n");
	plogdump(LLV_DEBUG, (*gxy)->v, (*gxy)->l);

	return 0;
}

/*
 * generate values of DH
 * IN:	*dh
 * OUT: **pub, **priv
 */
int
oakley_dh_generate(dh, pub, priv)
	const struct dhgroup *dh;
	vchar_t **pub, **priv;
{
#ifdef ENABLE_STATS
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	switch (dh->type) {
	case OAKLEY_ATTR_GRP_TYPE_MODP:
		if (eay_dh_generate(dh->prime, dh->gen1, dh->gen2, pub, priv) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to compute dh value.\n");
			return -1;
		}
		break;

	case OAKLEY_ATTR_GRP_TYPE_ECP:
	case OAKLEY_ATTR_GRP_TYPE_EC2N:
		plog(LLV_ERROR, LOCATION, NULL,
			"dh type %d isn't supported.\n", dh->type);
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid dh type %d.\n", dh->type);
		return -1;
	}

#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s): %8.6f", __FUNCTION__,
		s_attr_isakmp_group(dh->type), timedelta(&start, &end));
#endif
	plog(LLV_DEBUG, LOCATION, NULL, "compute DH's private.\n");
	plogdump(LLV_DEBUG, (*priv)->v, (*priv)->l);
	plog(LLV_DEBUG, LOCATION, NULL, "compute DH's public.\n");
	plogdump(LLV_DEBUG, (*pub)->v, (*pub)->l);

	return 0;
}

/*
 * copy pre-defined dhgroup values.
 */
int
oakley_setdhgroup(group, dhgrp)
	int group;
	struct dhgroup **dhgrp;
{
	*dhgrp = racoon_calloc(1, sizeof(struct dhgroup));
	if (*dhgrp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get DH buffer.\n");
		return 0;
	}
	switch (group) {
	case OAKLEY_ATTR_GRP_DESC_MODP768:
	case OAKLEY_ATTR_GRP_DESC_MODP1024:
	case OAKLEY_ATTR_GRP_DESC_MODP1536:
		if (group > ARRAYLEN(dhgroup)
		 || dhgroup[group].type == 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid DH parameter grp=%d.\n", group);
			racoon_free(*dhgrp);
			*dhgrp = NULL;
			return -1;
		}
		/* set defined dh vlaues */
		memcpy(*dhgrp, &dhgroup[group], sizeof(dhgroup[group]));
		(*dhgrp)->prime = vdup(dhgroup[group].prime);
		break;
	default:
		if (!(*dhgrp)->type || !(*dhgrp)->prime || !(*dhgrp)->gen1) {
			/* XXX unsuported */
			plog(LLV_ERROR, LOCATION, NULL,
				"unsupported DH parameters grp=%d.\n", group);
			return -1;
		}
	}

	return 0;
}

/*
 * PRF
 *
 * NOTE: we do not support prf with different input/output bitwidth,
 * so we do not implement RFC2409 Appendix B (DOORAK-MAC example) in
 * oakley_compute_keymat().  If you add support for such prf function,
 * modify oakley_compute_keymat() accordingly.
 */
vchar_t *
oakley_prf(key, buf, iph1)
	vchar_t *key, *buf;
	struct ph1handle *iph1;
{
	vchar_t *res;

	if (iph1->approval == NULL) {
		/*
		 * it's before negotiating hash algorithm.
		 * We use md5 as default.
		 */
		goto defs;
	}

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	switch (iph1->approval->dh_group) {
	default:
		switch (iph1->approval->hashtype) {
		case OAKLEY_ATTR_HASH_ALG_MD5:
defs:
			plog(LLV_DEBUG, LOCATION, NULL, "hmac-md5 used.\n");
			res = eay_hmacmd5_one(key, buf);
			break;
		case OAKLEY_ATTR_HASH_ALG_SHA:
			plog(LLV_DEBUG, LOCATION, NULL, "hmac-sha1 used.\n");
			res = eay_hmacsha1_one(key, buf);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"hash type %d isn't supported.\n",
				iph1->approval->hashtype);
			return NULL;
			break;
		}
	}
#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s size=%d): %8.6f", __FUNCTION__,
		s_attr_isakmp_hash(iph1->approval->hashtype),
		buf->l, timedelta(&start, &end));
    }
#endif

	return res;
}

/*
 * hash
 */
vchar_t *
oakley_hash(buf, iph1)
	vchar_t *buf;
	struct ph1handle *iph1;
{
	vchar_t *res;

	if (iph1->approval == NULL) {
		/*
		 * it's before negotiating hash algorithm.
		 * We use md5 as default.
		 */
		goto defs;
	}

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	switch (iph1->approval->dh_group) {
	default:
		switch (iph1->approval->hashtype) {
		case OAKLEY_ATTR_HASH_ALG_MD5:
defs:
			plog(LLV_DEBUG, LOCATION, NULL,
				"use md5 to calculate phase 1.\n");
			res = eay_md5_one(buf);
			break;
		case OAKLEY_ATTR_HASH_ALG_SHA:
			plog(LLV_DEBUG, LOCATION, NULL,
				"use sha1 to calculate phase 1.\n");
			res = eay_sha1_one(buf);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"hash type %d isn't supported.\n",
				iph1->approval->hashtype);
			return NULL;
			break;
		}
	}
#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s size=%d): %8.6f", __FUNCTION__,
		s_attr_isakmp_hash(iph1->approval->hashtype),
		buf->l, timedelta(&start, &end));
    }
#endif

	return res;
}

/*
 * compute KEYMAT
 *   see seciton 5.5 Phase 2 - Quick Mode in isakmp-oakley-05.
 */
int
oakley_compute_keymat(iph2, side)
	struct ph2handle *iph2;
	int side;
{
	int error = -1;

	/* compute sharing secret of DH when PFS */
	if (iph2->approval->pfs_group && iph2->dhpub_p) {
		if (oakley_dh_compute(iph2->pfsgrp, iph2->dhpub,
				iph2->dhpriv, iph2->dhpub_p, &iph2->dhgxy) < 0)
			goto end;
	}

	/* compute keymat */
	if (oakley_compute_keymat_x(iph2, side, INBOUND_SA) < 0
	 || oakley_compute_keymat_x(iph2, side, OUTBOUND_SA) < 0)
		goto end;

	plog(LLV_DEBUG, LOCATION, NULL, "KEYMAT computed.\n");

	error = 0;

end:
	return error;
}

/*
 * compute KEYMAT.
 * KEYMAT = prf(SKEYID_d, protocol | SPI | Ni_b | Nr_b).
 * If PFS is desired and KE payloads were exchanged,
 *   KEYMAT = prf(SKEYID_d, g(qm)^xy | protocol | SPI | Ni_b | Nr_b)
 *
 * NOTE: we do not support prf with different input/output bitwidth,
 * so we do not implement RFC2409 Appendix B (DOORAK-MAC example).
 */
static int
oakley_compute_keymat_x(iph2, side, sa_dir)
	struct ph2handle *iph2;
	int side;
	int sa_dir;
{
	vchar_t *buf = NULL, *res = NULL, *bp;
	char *p;
	int len;
	int error = -1;
	int pfs = 0;
	int dupkeymat;	/* generate K[1-dupkeymat] */
	struct saproto *pr;

	pfs = ((iph2->approval->pfs_group && iph2->dhgxy) ? 1 : 0);
	
	len = pfs ? iph2->dhgxy->l : 0;
	len += (1
		+ sizeof(u_int32_t)	/* XXX SPI size */
		+ iph2->nonce->l
		+ iph2->nonce_p->l);
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get keymat buffer.\n");
		goto end;
	}

	for (pr = iph2->approval->head; pr != NULL; pr = pr->next) {
		p = buf->v;

		/* if PFS */
		if (pfs) {
			memcpy(p, iph2->dhgxy->v, iph2->dhgxy->l);
			p += iph2->dhgxy->l;
		}

		p[0] = pr->proto_id;
		p += 1;

		memcpy(p, (sa_dir == INBOUND_SA ? &pr->spi : &pr->spi_p),
			sizeof(pr->spi));
		p += sizeof(pr->spi);

		bp = (side == INITIATOR ? iph2->nonce : iph2->nonce_p);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		bp = (side == INITIATOR ? iph2->nonce_p : iph2->nonce);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		/* compute IV */
		plog(LLV_DEBUG, LOCATION, NULL, "KEYMAT compute with\n");
		plogdump(LLV_DEBUG, buf->v, buf->l);

		/* res = K1 */
		res = oakley_prf(iph2->ph1->skeyid_d, buf, iph2->ph1);
		if (res == NULL)
			goto end;

		/* a guess: ESP: 128bit minimum, AH: 160 bit minimum */
		dupkeymat = ((pr->len ? pr->len : 128) + 160) / 8 / res->l;
		dupkeymat += 2;	/* safety mergin */
		if (dupkeymat < 3)
			dupkeymat = 3;
		plog(LLV_DEBUG, LOCATION, NULL, "dupkeymat=%d\n", dupkeymat);
		if (0 < --dupkeymat) {
			vchar_t *prev = res;	/* K(n-1) */
			vchar_t *seed = NULL;	/* seed for Kn */
			size_t l;

			/*
			 * generating long key (isakmp-oakley-08 5.5)
			 *	KEYMAT = K1 | K2 | K3 | ...
			 * where
			 *	src = [ g(qm)^xy | ] protocol | SPI | Ni_b | Nr_b
			 *	K1 = prf(SKEYID_d, src)
			 *	K2 = prf(SKEYID_d, K1 | src)
			 *	K3 = prf(SKEYID_d, K2 | src)
			 *	Kn = prf(SKEYID_d, K(n-1) | src)
			 */
			plog(LLV_DEBUG, LOCATION, NULL,
				"generating K1...K%d for KEYMAT.\n",
				dupkeymat + 1);

			seed = vmalloc(prev->l + buf->l);
			if (seed == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to get keymat buffer.\n");
				if (prev && prev != res)
					vfree(prev);
				goto end;
			}

			while (dupkeymat--) {
				vchar_t *this = NULL;	/* Kn */

				memcpy(seed->v, prev->v, prev->l);
				memcpy(seed->v + prev->l, buf->v, buf->l);
				this = oakley_prf(iph2->ph1->skeyid_d, seed, iph2->ph1);
				if (!this) {
					plog(LLV_ERROR, LOCATION, NULL,
						"oakley_prf memory overflow\n");
					if (prev && prev != res)
						vfree(prev);
					vfree(this);
					vfree(seed);
					goto end;
				}

				l = res->l;
				if (!VREALLOC(res, l + this->l)) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to get keymat buffer.\n");
					if (prev && prev != res)
						vfree(prev);
					vfree(this);
					vfree(seed);
					goto end;
				}
				memcpy(res->v + l, this->v, this->l);

				if (prev && prev != res)
					vfree(prev);
				prev = this;
				this = NULL;
			}

			if (prev && prev != res)
				vfree(prev);
			vfree(seed);
		}

		plogdump(LLV_DEBUG, res->v, res->l);

		if (sa_dir == INBOUND_SA)
			pr->keymat = res;
		else
			pr->keymat_p = res;
		res = NULL;
	}

	error = 0;

end:
	if (error) {
		for (pr = iph2->approval->head; pr != NULL; pr = pr->next) {
			if (pr->keymat) {
				vfree(pr->keymat);
				pr->keymat = NULL;
			}
			if (pr->keymat_p) {
				vfree(pr->keymat_p);
				pr->keymat_p = NULL;
			}
		}
	}

	if (buf != NULL)
		vfree(buf);
	if (res)
		vfree(res);

	return error;
}

#if notyet
/*
 * NOTE: Must terminate by NULL.
 */
vchar_t *
oakley_compute_hashx(struct ph1handle *iph1, ...)
{
	vchar_t *buf, *res;
	vchar_t *s;
	caddr_t p;
	int len;

	va_list ap;

	/* get buffer length */
	va_start(ap, iph1);
	len = 0;
        while ((s = va_arg(ap, vchar_t *)) != NULL) {
		len += s->l
        }
	va_end(ap);

	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get hash buffer\n");
		return NULL;
	}

	/* set buffer */
	va_start(ap, iph1);
	p = buf->v;
        while ((s = va_arg(ap, char *)) != NULL) {
		memcpy(p, s->v, s->l);
		p += s->l;
	}
	va_end(ap);

	plog(LLV_DEBUG, LOCATION, NULL, "HASH with: \n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(iph1->skeyid_a, buf, iph1);
	vfree(buf);
	if (res == NULL)
		return NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH computed: ");
	plogdump(LLV_DEBUG, res->v, res->l);

	return res;
}
#endif

/*
 * compute HASH(3) prf(SKEYID_a, 0 | M-ID | Ni_b | Nr_b)
 *   see seciton 5.5 Phase 2 - Quick Mode in isakmp-oakley-05.
 */
vchar_t *
oakley_compute_hash3(iph1, msgid, body)
	struct ph1handle *iph1;
	u_int32_t msgid;
	vchar_t *body;
{
	vchar_t *buf = 0, *res = 0;
	int len;
	int error = -1;

	/* create buffer */
	len = 1 + sizeof(u_int32_t) + body->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}

	buf->v[0] = 0;

	memcpy(buf->v + 1, (char *)&msgid, sizeof(msgid));

	memcpy(buf->v + 1 + sizeof(u_int32_t), body->v, body->l);

	plog(LLV_DEBUG, LOCATION, NULL, "HASH with: \n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(iph1->skeyid_a, buf, iph1);
	if (res == NULL)
		goto end;

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH computed: ");
	plogdump(LLV_DEBUG, res->v, res->l);

end:
	if (buf != NULL)
		vfree(buf);
	return res;
}

/*
 * compute HASH type of prf(SKEYID_a, M-ID | buffer)
 *	e.g.
 *	for quick mode HASH(1):
 *		prf(SKEYID_a, M-ID | SA | Ni [ | KE ] [ | IDci | IDcr ])
 *	for quick mode HASH(2):
 *		prf(SKEYID_a, M-ID | Ni_b | SA | Nr [ | KE ] [ | IDci | IDcr ])
 *	for Informational exchange:
 *		prf(SKEYID_a, M-ID | N/D)
 */
vchar_t *
oakley_compute_hash1(iph1, msgid, body)
	struct ph1handle *iph1;
	u_int32_t msgid;
	vchar_t *body;
{
	vchar_t *buf = NULL, *res = NULL;
	char *p;
	int len;
	int error = -1;

	/* create buffer */
	len = sizeof(u_int32_t) + body->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}

	p = buf->v;

	memcpy(buf->v, (char *)&msgid, sizeof(msgid));
	p += sizeof(u_int32_t);

	memcpy(p, body->v, body->l);

	plog(LLV_DEBUG, LOCATION, NULL, "HASH with:\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(iph1->skeyid_a, buf, iph1);
	if (res == NULL)
		goto end;

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH computed: ");
	plogdump(LLV_DEBUG, res->v, res->l);

end:
	if (buf != NULL)
		vfree(buf);
	return res;
}

/*
 * compute phase1 HASH
 * main/aggressive
 *   I-digest = prf(SKEYID, g^i | g^r | CKY-I | CKY-R | SAi_b | ID_i1_b)
 *   R-digest = prf(SKEYID, g^r | g^i | CKY-R | CKY-I | SAi_b | ID_r1_b)
 * for gssapi, also include all GSS tokens, and call gss_wrap on the result
 */
vchar_t *
oakley_ph1hash_common(iph1, sw)
	struct ph1handle *iph1;
	int sw;
{
	vchar_t *buf = NULL, *res = NULL, *bp;
	char *p, *bp2;
	int len, bl;
	int error = -1;
#ifdef HAVE_GSSAPI
	vchar_t *gsstokens = NULL;
#endif

	/* create buffer */
	len = iph1->dhpub->l
		+ iph1->dhpub_p->l
		+ sizeof(cookie_t) * 2
		+ iph1->sa->l
		+ (sw == GENERATE ? iph1->id->l : iph1->id_p->l);

#ifdef HAVE_GSSAPI
	if (iph1->approval->authmethod == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		if (iph1->gi_i != NULL && iph1->gi_r != NULL) {
			bp = (sw == GENERATE ? iph1->gi_i : iph1->gi_r);
			len += bp->l;
		}
		if (sw == GENERATE)
			gssapi_get_itokens(iph1, &gsstokens);
		else
			gssapi_get_rtokens(iph1, &gsstokens);
		if (gsstokens == NULL)
			return NULL;
		len += gsstokens->l;
	}
#endif

	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}

	p = buf->v;

	bp = (sw == GENERATE ? iph1->dhpub : iph1->dhpub_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	bp = (sw == GENERATE ? iph1->dhpub_p : iph1->dhpub);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	if (iph1->side == INITIATOR)
		bp2 = (sw == GENERATE ?
		      (char *)&iph1->index.i_ck : (char *)&iph1->index.r_ck);
	else
		bp2 = (sw == GENERATE ?
		      (char *)&iph1->index.r_ck : (char *)&iph1->index.i_ck);
	bl = sizeof(cookie_t);
	memcpy(p, bp2, bl);
	p += bl;

	if (iph1->side == INITIATOR)
		bp2 = (sw == GENERATE ?
		      (char *)&iph1->index.r_ck : (char *)&iph1->index.i_ck);
	else
		bp2 = (sw == GENERATE ?
		      (char *)&iph1->index.i_ck : (char *)&iph1->index.r_ck);
	bl = sizeof(cookie_t);
	memcpy(p, bp2, bl);
	p += bl;

	bp = iph1->sa;
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	bp = (sw == GENERATE ? iph1->id : iph1->id_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

#ifdef HAVE_GSSAPI
	if (iph1->approval->authmethod == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		if (iph1->gi_i != NULL && iph1->gi_r != NULL) {
			bp = (sw == GENERATE ? iph1->gi_i : iph1->gi_r);
			memcpy(p, bp->v, bp->l);
			p += bp->l;
		}
		memcpy(p, gsstokens->v, gsstokens->l);
		p += gsstokens->l;
	}
#endif

	plog(LLV_DEBUG, LOCATION, NULL, "HASH with:\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(iph1->skeyid, buf, iph1);
	if (res == NULL)
		goto end;

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH computed: ");
	plogdump(LLV_DEBUG, res->v, res->l);

end:
	if (buf != NULL)
		vfree(buf);
#ifdef HAVE_GSSAPI
	if (gsstokens != NULL)
		vfree(gsstokens);
#endif
	return res;
}

/*
 * compute HASH_I on base mode.
 * base:psk,rsa
 *   HASH_I = prf(SKEYID, g^xi | CKY-I | CKY-R | SAi_b | IDii_b)
 * base:sig
 *   HASH_I = prf(hash(Ni_b | Nr_b), g^xi | CKY-I | CKY-R | SAi_b | IDii_b)
 */
vchar_t *
oakley_ph1hash_base_i(iph1, sw)
	struct ph1handle *iph1;
	int sw;
{
	vchar_t *buf = NULL, *res = NULL, *bp;
	vchar_t *hashkey = NULL;
	vchar_t *hash = NULL;	/* for signature mode */
	char *p;
	int len;
	int error = -1;

	/* sanity check */
	if (iph1->etype != ISAKMP_ETYPE_BASE) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid etype for this hash function\n");
		return NULL;
	}

	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
		if (iph1->skeyid == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, "no SKEYID found.\n");
			return NULL;
		}
		hashkey = iph1->skeyid;
		break;

	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
		/* make hash for seed */
		len = iph1->nonce->l + iph1->nonce_p->l;
		buf = vmalloc(len);
		if (buf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get hash buffer\n");
			goto end;
		}
		p = buf->v;

		bp = (sw == GENERATE ? iph1->nonce_p : iph1->nonce);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		bp = (sw == GENERATE ? iph1->nonce : iph1->nonce_p);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		hash = oakley_hash(buf, iph1);
		if (hash == NULL)
			goto end;
		vfree(buf);
		buf = NULL;

		hashkey = hash;
		break;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"not supported authentication method %d\n",
			iph1->approval->authmethod);
		return NULL;

	}

	len = (sw == GENERATE ? iph1->dhpub->l : iph1->dhpub_p->l)
		+ sizeof(cookie_t) * 2
		+ iph1->sa->l
		+ (sw == GENERATE ? iph1->id->l : iph1->id_p->l);
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}
	p = buf->v;

	bp = (sw == GENERATE ? iph1->dhpub : iph1->dhpub_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	memcpy(p, &iph1->index.i_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	memcpy(p, &iph1->index.r_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);

	memcpy(p, iph1->sa->v, iph1->sa->l);
	p += iph1->sa->l;

	bp = (sw == GENERATE ? iph1->id : iph1->id_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH_I with:\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(hashkey, buf, iph1);
	if (res == NULL)
		goto end;

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH_I computed:");
	plogdump(LLV_DEBUG, res->v, res->l);

end:
	if (hash != NULL)
		vfree(hash);
	if (buf != NULL)
		vfree(buf);
	return res;
}

/*
 * compute HASH_R on base mode for signature method.
 * base:
 * HASH_R = prf(hash(Ni_b | Nr_b), g^xi | g^xr | CKY-I | CKY-R | SAi_b | IDii_b)
 */
vchar_t *
oakley_ph1hash_base_r(iph1, sw)
	struct ph1handle *iph1;
	int sw;
{
	vchar_t *buf = NULL, *res = NULL, *bp;
	vchar_t *hash = NULL;
	char *p;
	int len;
	int error = -1;

	/* sanity check */
	if (iph1->etype != ISAKMP_ETYPE_BASE) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid etype for this hash function\n");
		return NULL;
	}
	if (iph1->approval->authmethod != OAKLEY_ATTR_AUTH_METHOD_DSSSIG
	 && iph1->approval->authmethod != OAKLEY_ATTR_AUTH_METHOD_RSASIG) {
		plog(LLV_ERROR, LOCATION, NULL,
			"not supported authentication method %d\n",
			iph1->approval->authmethod);
		return NULL;
	}

	/* make hash for seed */
	len = iph1->nonce->l + iph1->nonce_p->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}
	p = buf->v;

	bp = (sw == GENERATE ? iph1->nonce_p : iph1->nonce);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	bp = (sw == GENERATE ? iph1->nonce : iph1->nonce_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	hash = oakley_hash(buf, iph1);
	if (hash == NULL)
		goto end;
	vfree(buf);
	buf = NULL;

	/* make really hash */
	len = (sw == GENERATE ? iph1->dhpub_p->l : iph1->dhpub->l)
		+ (sw == GENERATE ? iph1->dhpub->l : iph1->dhpub_p->l)
		+ sizeof(cookie_t) * 2
		+ iph1->sa->l
		+ (sw == GENERATE ? iph1->id_p->l : iph1->id->l);
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get hash buffer\n");
		goto end;
	}
	p = buf->v;


	bp = (sw == GENERATE ? iph1->dhpub_p : iph1->dhpub);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	bp = (sw == GENERATE ? iph1->dhpub : iph1->dhpub_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	memcpy(p, &iph1->index.i_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	memcpy(p, &iph1->index.r_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);

	memcpy(p, iph1->sa->v, iph1->sa->l);
	p += iph1->sa->l;

	bp = (sw == GENERATE ? iph1->id_p : iph1->id);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH with:\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* compute HASH */
	res = oakley_prf(hash, buf, iph1);
	if (res == NULL)
		goto end;

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "HASH computed:");
	plogdump(LLV_DEBUG, res->v, res->l);

end:
	if (buf != NULL)
		vfree(buf);
	if (hash)
		vfree(hash);
	return res;
}

/*
 * compute each authentication method in phase 1.
 * OUT:
 *	0:	OK
 *	-1:	error
 *	other:	error to be reply with notification.
 *	        the value is notification type.
 */
int
oakley_validate_auth(iph1)
	struct ph1handle *iph1;
{
	vchar_t *my_hash = NULL;
	int result;
#ifdef HAVE_GSSAPI
	vchar_t *gsshash = NULL;
#endif

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
		/* validate HASH */
	    {
		char *r_hash;

		if (iph1->id_p == NULL || iph1->pl_hash == NULL) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"few isakmp message received.\n");
			return ISAKMP_NTYPE_PAYLOAD_MALFORMED;
		}

		r_hash = (caddr_t)(iph1->pl_hash + 1);

		plog(LLV_DEBUG, LOCATION, NULL, "HASH received:");
		plogdump(LLV_DEBUG, r_hash,
			ntohs(iph1->pl_hash->h.len) - sizeof(*iph1->pl_hash));

		switch (iph1->etype) {
		case ISAKMP_ETYPE_IDENT:
		case ISAKMP_ETYPE_AGG:
			my_hash = oakley_ph1hash_common(iph1, VALIDATE);
			break;
		case ISAKMP_ETYPE_BASE:
			if (iph1->side == INITIATOR)
				my_hash = oakley_ph1hash_common(iph1, VALIDATE);
			else
				my_hash = oakley_ph1hash_base_i(iph1, VALIDATE);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid etype %d\n", iph1->etype);
			return ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE;
		}
		if (my_hash == NULL)
			return -1;

		result = memcmp(my_hash->v, r_hash, my_hash->l);
		vfree(my_hash);

		if (result) {
			plog(LLV_ERROR, LOCATION, NULL, "HASH mismatched\n");
			return ISAKMP_NTYPE_INVALID_HASH_INFORMATION;
		}

		plog(LLV_DEBUG, LOCATION, NULL, "HASH for PSK validated.\n");
	    }
		break;
#ifdef HAVE_SIGNING_C
	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
	    {
		int error = 0;

		/* validation */
		if (iph1->id_p == NULL) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"no ID payload was passed.\n");
			return ISAKMP_NTYPE_PAYLOAD_MALFORMED;
		}
		if (iph1->sig_p == NULL) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"no SIG payload was passed.\n");
			return ISAKMP_NTYPE_PAYLOAD_MALFORMED;
		}

		plog(LLV_DEBUG, LOCATION, NULL, "SIGN passed:\n");
		plogdump(LLV_DEBUG, iph1->sig_p->v, iph1->sig_p->l);

		/* get peer's cert */
		switch (iph1->rmconf->getcert_method) {
		case ISAKMP_GETCERT_PAYLOAD:
			if (iph1->cert_p == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no peer's CERT payload found "
					"even though CR sent.\n");
				return -1;
			}
			break;
		case ISAKMP_GETCERT_LOCALFILE:
			if (iph1->rmconf->peerscertfile == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no peer's CERT file found.\n");
				return -1;
			}

			/* don't cache cert */
			if (iph1->cert_p != NULL) {
				oakley_delcert(iph1->cert_p);
				iph1->cert_p = NULL;
			}

			error = get_cert_fromlocal(iph1, 0);
			if (error)
				return -1;
			break;
		case ISAKMP_GETCERT_DNS:
			if (iph1->rmconf->peerscertfile != NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"why peer's CERT file is defined "
					"though getcert method is dns ?\n");
				return -1;
			}

			/* don't cache cert */
			if (iph1->cert_p != NULL) {
				oakley_delcert(iph1->cert_p);
				iph1->cert_p = NULL;
			}

			iph1->cert_p = dnssec_getcert(iph1->id_p);
			if (iph1->cert_p == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no CERT RR found.\n");
				return -1;
			}
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid getcert_mothod: %d\n",
				iph1->rmconf->getcert_method);
			return -1;
		}

		/* compare ID payload and certificate name */
		if (iph1->rmconf->verify_cert && oakley_check_certid(iph1))
			return error;

		/* verify certificate */
		if (iph1->rmconf->verify_cert
		 && iph1->rmconf->getcert_method == ISAKMP_GETCERT_PAYLOAD) {
			switch (iph1->rmconf->certtype) {
			case ISAKMP_CERT_X509SIGN:
				error = eay_check_x509cert(&iph1->cert_p->cert,
					lcconf->pathinfo[LC_PATHTYPE_CERT]);
				/* XXX to be checked subjectAltName */
				break;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"no supported certtype %d\n",
					iph1->rmconf->certtype);
				return -1;
			}
			if (error != 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"Invalid authority of the CERT.\n");
				return ISAKMP_NTYPE_INVALID_CERT_AUTHORITY;
			}
		}

		plog(LLV_DEBUG, LOCATION, NULL, "CERT validated\n");

		/* compute hash */
		switch (iph1->etype) {
		case ISAKMP_ETYPE_IDENT:
		case ISAKMP_ETYPE_AGG:
			my_hash = oakley_ph1hash_common(iph1, VALIDATE);
			break;
		case ISAKMP_ETYPE_BASE:
			if (iph1->side == INITIATOR)
				my_hash = oakley_ph1hash_base_r(iph1, VALIDATE);
			else
				my_hash = oakley_ph1hash_base_i(iph1, VALIDATE);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid etype %d\n", iph1->etype);
			return ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE;
		}
		if (my_hash == NULL)
			return -1;

		/* check signature */
		switch (iph1->rmconf->certtype) {
		case ISAKMP_CERT_X509SIGN:
		case ISAKMP_CERT_DNS:
			error = eay_check_x509sign(my_hash,
					iph1->sig_p,
					&iph1->cert_p->cert);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"no supported certtype %d\n",
				iph1->rmconf->certtype);
			vfree(my_hash);
			return -1;
		}

		vfree(my_hash);
		if (error != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Invalid SIG.\n");
			return ISAKMP_NTYPE_INVALID_SIGNATURE;
		}
		plog(LLV_DEBUG, LOCATION, NULL, "SIG authenticated\n");
	    }
		break;
#endif
#ifdef HAVE_GSSAPI
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
		switch (iph1->etype) {
		case ISAKMP_ETYPE_IDENT:
		case ISAKMP_ETYPE_AGG:
			my_hash = oakley_ph1hash_common(iph1, VALIDATE);
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid etype %d\n", iph1->etype);
			return ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE;
		}

		if (my_hash == NULL) {
			if (gssapi_more_tokens(iph1))
				return ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE;
			else
				return ISAKMP_NTYPE_INVALID_HASH_INFORMATION;
		}

		gsshash = gssapi_unwraphash(iph1);
		if (gsshash == NULL) {
			vfree(my_hash);
			return ISAKMP_NTYPE_INVALID_HASH_INFORMATION;
		}

		result = memcmp(my_hash->v, gsshash->v, my_hash->l);
		vfree(my_hash);
		vfree(gsshash);

		if (result) {
			plog(LLV_ERROR, LOCATION, NULL, "HASH mismatched\n");
			return ISAKMP_NTYPE_INVALID_HASH_INFORMATION;
		}
		plog(LLV_DEBUG, LOCATION, NULL, "hash compared OK\n");
		break;
#endif
	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
		if (iph1->id_p == NULL || iph1->pl_hash == NULL) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"few isakmp message received.\n");
			return ISAKMP_NTYPE_PAYLOAD_MALFORMED;
		}
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"not supported authmethod type %s\n",
			s_oakley_attr_method(iph1->approval->authmethod));
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"invalid authmethod %d why ?\n",
			iph1->approval->authmethod);
		return -1;
	}
#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s): %8.6f", __FUNCTION__,
		s_oakley_attr_method(iph1->approval->authmethod),
		timedelta(&start, &end));
    }
#endif

	return 0;
}

#ifdef HAVE_SIGNING_C
/* get my certificate
 * NOTE: include certificate type.
 */
int
oakley_getmycert(iph1)
	struct ph1handle *iph1;
{
	if (iph1->cert)
		return 0;	/* There is CERT. */

	return get_cert_fromlocal(iph1, 1);
}

/*
 * get a CERT from local file.
 * IN:
 *	my != 0 my cert.
 *	my == 0 peer's cert.
 */
static int
get_cert_fromlocal(iph1, my)
	struct ph1handle *iph1;
	int my;
{
	char path[MAXPATHLEN];
	vchar_t *cert = NULL;
	cert_t **certpl;
	char *certfile;
	int error = -1;

	if (my) {
		certfile = iph1->rmconf->mycertfile;
		certpl = &iph1->cert;
	} else {
		certfile = iph1->rmconf->peerscertfile;
		certpl = &iph1->cert_p;
	}
	if (!certfile) {
		plog(LLV_ERROR, LOCATION, NULL, "no CERT defined.\n");
		return 0;
	}

	switch (iph1->rmconf->certtype) {
	case ISAKMP_CERT_X509SIGN:
	case ISAKMP_CERT_DNS:
		/* make public file name */
		getpathname(path, sizeof(path), LC_PATHTYPE_CERT, certfile);
		cert = eay_get_x509cert(path);
		if (cert) {
			char *p = NULL;
			p = eay_get_x509text(cert);
			plog(LLV_DEBUG, LOCATION, NULL, "%s", p ? p : "\n");
			racoon_free(p);
		};
		break;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"not supported certtype %d\n",
			iph1->rmconf->certtype);
		goto end;
	}

	if (!cert) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get %s CERT.\n",
			my ? "my" : "peers");
		goto end;
	}

	*certpl = oakley_newcert();
	if (!*certpl) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get cert buffer.\n");
		goto end;
	}
	(*certpl)->pl = vmalloc(cert->l + 1);
	if ((*certpl)->pl == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get cert buffer\n");
		oakley_delcert(*certpl);
		*certpl = NULL;
		goto end;
	}
	memcpy((*certpl)->pl->v + 1, cert->v, cert->l);
	(*certpl)->pl->v[0] = iph1->rmconf->certtype;
	(*certpl)->type = iph1->rmconf->certtype;
	(*certpl)->cert.v = (*certpl)->pl->v + 1;
	(*certpl)->cert.l = (*certpl)->pl->l - 1;

	plog(LLV_DEBUG, LOCATION, NULL, "created CERT payload:\n");
	plogdump(LLV_DEBUG, (*certpl)->pl->v, (*certpl)->pl->l);

	error = 0;

end:
	if (cert != NULL)
		vfree(cert);

	return error;
}

/* get signature */
int
oakley_getsign(iph1)
	struct ph1handle *iph1;
{
	char path[MAXPATHLEN];
	vchar_t *privkey = NULL;
	int error = -1;

	switch (iph1->rmconf->certtype) {
	case ISAKMP_CERT_X509SIGN:
	case ISAKMP_CERT_DNS:
		if (iph1->rmconf->myprivfile == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, "no cert defined.\n");
			goto end;
		}

		/* make private file name */
		getpathname(path, sizeof(path),
			LC_PATHTYPE_CERT,
			iph1->rmconf->myprivfile);
		privkey = eay_get_pkcs1privkey(path);
		if (privkey == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get private key.\n");
			goto end;
		}
		plog(LLV_DEBUG, LOCATION, NULL, "private key:\n");
		plogdump(LLV_DEBUG, privkey->v, privkey->l);

		iph1->sig = eay_get_x509sign(iph1->hash,
					privkey, &iph1->cert->cert);
		break;
	default:
		goto end;
	}

	if (iph1->sig == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "failed to sign.\n");
		goto end;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "SIGN computed:\n");
	plogdump(LLV_DEBUG, iph1->sig->v, iph1->sig->l);

	error = 0;

end:
	if (privkey != NULL)
		vfree(privkey);

	return error;
}

/*
 * compare certificate name and ID value.
 */
static int
oakley_check_certid(iph1)
	struct ph1handle *iph1;
{
	struct ipsecdoi_id_b *id_b;
	vchar_t *name = NULL;
	char *altname = NULL;
	int idlen, type;
	int error;

	if (iph1->id_p == NULL || iph1->cert_p == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "no ID nor CERT found.\n");
		return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}

	id_b = (struct ipsecdoi_id_b *)iph1->id_p->v;
	idlen = iph1->id_p->l - sizeof(*id_b);

	switch (id_b->type) {
	case IPSECDOI_ID_DER_ASN1_DN:
		name = eay_get_x509asn1subjectname(&iph1->cert_p->cert);
		if (!name) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get subjectName\n");
			return ISAKMP_NTYPE_INVALID_CERTIFICATE;
		}
		if (idlen != name->l) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Invalid ID length in phase 1.\n");
			vfree(name);
			return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
		}
		error = memcmp(id_b + 1, name->v, idlen);
		vfree(name);
		return error == 0 ? 0 : ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV6_ADDR:
	{
		/*
		 * converting to binary from string because openssl return
		 * a string even if object is a binary.
		 * XXX fix it !  access by ASN.1 directly without.
		 */
		struct addrinfo hints, *res;
		caddr_t a = NULL;
		int pos;

		for (pos = 1; ; pos++) {
			if (eay_get_x509subjectaltname(&iph1->cert_p->cert,
					&altname, &type, pos) !=0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to get subjectAltName\n");
				return ISAKMP_NTYPE_INVALID_CERTIFICATE;
			}

			/* it's the end condition of the loop. */
			if (!altname) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no proper subjectAltName.\n");
				return ISAKMP_NTYPE_INVALID_CERTIFICATE;
			}

			if (check_typeofcertname(id_b->type, type) == 0)
				break;

			/* next name */
			racoon_free(altname);
			altname = NULL;
		}
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_RAW;
		hints.ai_flags = AI_NUMERICHOST;
		error = getaddrinfo(altname, NULL, &hints, &res);
		if (error != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"no proper subjectAltName.\n");
			racoon_free(altname);
			return ISAKMP_NTYPE_INVALID_CERTIFICATE;
		}
		switch (res->ai_family) {
		case AF_INET:
			a = (caddr_t)&((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
			break;
#ifdef INET6
		case AF_INET6:
			a = (caddr_t)&((struct sockaddr_in6 *)res->ai_addr)->sin6_addr.s6_addr;
			break;
#endif
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"family not supported: %d.\n", res->ai_family);
			racoon_free(altname);
			freeaddrinfo(res);
			return ISAKMP_NTYPE_INVALID_CERTIFICATE;
		}
		error = memcmp(id_b + 1, a, idlen);
		freeaddrinfo(res);
		vfree(name);
		return error == 0 ? 0 : ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}
	case IPSECDOI_ID_FQDN:
	case IPSECDOI_ID_USER_FQDN:
	{
		int pos;

		for (pos = 1; ; pos++) {
			if (eay_get_x509subjectaltname(&iph1->cert_p->cert,
					&altname, &type, pos) != 0){
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to get subjectAltName\n");
				return ISAKMP_NTYPE_INVALID_CERTIFICATE;
			}

			/* it's the end condition of the loop. */
			if (!altname) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no proper subjectAltName.\n");
				return ISAKMP_NTYPE_INVALID_CERTIFICATE;
			}

			if (check_typeofcertname(id_b->type, type) == 0)
				break;

			/* next name */
			racoon_free(altname);
			altname = NULL;
		}
		if (idlen != strlen(altname)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Invalid ID length in phase 1.\n");
			racoon_free(altname);
			return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
		}
		if (check_typeofcertname(id_b->type, type) != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"ID type mismatched. ID: %s CERT: %s.\n",
				s_ipsecdoi_ident(id_b->type),
				s_ipsecdoi_ident(type));
			racoon_free(altname);
			return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
		}
		error = memcmp(id_b + 1, altname, idlen);
		racoon_free(altname);
		return error == 0 ? 0 : ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"Inpropper ID type passed: %s.\n",
			s_ipsecdoi_ident(id_b->type));
		return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}
	/*NOTREACHED*/
}

static int
check_typeofcertname(doi, genid)
	int doi, genid;
{
	switch (doi) {
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
	case IPSECDOI_ID_IPV6_ADDR:
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
	case IPSECDOI_ID_IPV4_ADDR_RANGE:
	case IPSECDOI_ID_IPV6_ADDR_RANGE:
		if (genid != GENT_IPADD)
			return -1;
		return 0;
	case IPSECDOI_ID_FQDN:
		if (genid != GENT_DNS)
			return -1;
		return 0;
	case IPSECDOI_ID_USER_FQDN:
		if (genid != GENT_EMAIL)
			return -1;
		return 0;
	case IPSECDOI_ID_DER_ASN1_DN: /* should not be passed to this function*/
	case IPSECDOI_ID_DER_ASN1_GN:
	case IPSECDOI_ID_KEY_ID:
	default:
		return -1;
	}
	/*NOTREACHED*/
}

/*
 * save certificate including certificate type.
 */
int
oakley_savecert(iph1, gen)
	struct ph1handle *iph1;
	struct isakmp_gen *gen;
{
	cert_t **c;
	u_int8_t type;

	type = *(u_int8_t *)(gen + 1) & 0xff;

	switch (type) {
	case ISAKMP_CERT_DNS:
		plog(LLV_WARNING, LOCATION, NULL,
			"CERT payload is unnecessary in DNSSEC. "
			"ignore this CERT payload.\n");
		return 0;
	case ISAKMP_CERT_PKCS7:
	case ISAKMP_CERT_PGP:
	case ISAKMP_CERT_X509SIGN:
	case ISAKMP_CERT_KERBEROS:
	case ISAKMP_CERT_SPKI:
		c = &iph1->cert_p;
		break;
	case ISAKMP_CERT_CRL:
		c = &iph1->crl_p;
		break;
	case ISAKMP_CERT_X509KE:
	case ISAKMP_CERT_X509ATTR:
	case ISAKMP_CERT_ARL:
		plog(LLV_ERROR, LOCATION, NULL,
			"No supported such CERT type %d\n", type);
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"Invalid CERT type %d\n", type);
		return -1;
	}

	/* XXX choice the 1th cert, ignore after the cert. */ 
	/* XXX should be processed. */
	if (*c) {
		plog(LLV_WARNING, LOCATION, NULL,
			"ignore 2nd CERT payload.\n");
		return 0;
	}

	*c = save_certbuf(gen);
	if (!*c) {
		plog(LLV_ERROR, LOCATION, NULL,
			"Failed to get CERT buffer.\n");
		return -1;
	}

	switch ((*c)->type) {
	case ISAKMP_CERT_DNS:
		plog(LLV_WARNING, LOCATION, NULL,
			"CERT payload is unnecessary in DNSSEC. "
			"ignore it.\n");
		return 0;
	case ISAKMP_CERT_PKCS7:
	case ISAKMP_CERT_PGP:
	case ISAKMP_CERT_X509SIGN:
	case ISAKMP_CERT_KERBEROS:
	case ISAKMP_CERT_SPKI:
		plog(LLV_DEBUG, LOCATION, NULL, "CERT saved:\n");
		plogdump(LLV_DEBUG, (*c)->cert.v, (*c)->cert.l);
		{
			char *p = eay_get_x509text(&(*c)->cert);
			plog(LLV_DEBUG, LOCATION, NULL, "%s", p ? p : "\n");
			racoon_free(p);
		}
		break;
	case ISAKMP_CERT_CRL:
		plog(LLV_DEBUG, LOCATION, NULL, "CRL saved:\n");
		plogdump(LLV_DEBUG, (*c)->cert.v, (*c)->cert.l);
		break;
	case ISAKMP_CERT_X509KE:
	case ISAKMP_CERT_X509ATTR:
	case ISAKMP_CERT_ARL:
	default:
		/* XXX */
		oakley_delcert((*c));
		*c = NULL;
		return 0;
	}

	return 0;
}

/*
 * save certificate including certificate type.
 */
int
oakley_savecr(iph1, gen)
	struct ph1handle *iph1;
	struct isakmp_gen *gen;
{
	cert_t **c;
	u_int8_t type;

	type = *(u_int8_t *)(gen + 1) & 0xff;

	switch (type) {
	case ISAKMP_CERT_DNS:
		plog(LLV_WARNING, LOCATION, NULL,
			"CERT payload is unnecessary in DNSSEC\n");
		/*FALLTHRU*/
	case ISAKMP_CERT_PKCS7:
	case ISAKMP_CERT_PGP:
	case ISAKMP_CERT_X509SIGN:
	case ISAKMP_CERT_KERBEROS:
	case ISAKMP_CERT_SPKI:
		c = &iph1->cr_p;
		break;
	case ISAKMP_CERT_X509KE:
	case ISAKMP_CERT_X509ATTR:
	case ISAKMP_CERT_ARL:
		plog(LLV_ERROR, LOCATION, NULL,
			"No supported such CR type %d\n", type);
		return -1;
	case ISAKMP_CERT_CRL:
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"Invalid CR type %d\n", type);
		return -1;
	}

	*c = save_certbuf(gen);
	if (!*c) {
		plog(LLV_ERROR, LOCATION, NULL,
			"Failed to get CR buffer.\n");
		return -1;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "CR saved:\n");
	plogdump(LLV_DEBUG, (*c)->cert.v, (*c)->cert.l);

	return 0;
}

static cert_t *
save_certbuf(gen)
	struct isakmp_gen *gen;
{
	cert_t *new;

	new = oakley_newcert();
	if (!new) {
		plog(LLV_ERROR, LOCATION, NULL,
			"Failed to get CERT buffer.\n");
		return NULL;
	}

	new->pl = vmalloc(ntohs(gen->len) - sizeof(*gen));
	if (new->pl == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"Failed to copy CERT from packet.\n");
		oakley_delcert(new);
		new = NULL;
		return NULL;
	}
	memcpy(new->pl->v, gen + 1, new->pl->l);
	new->type = new->pl->v[0] & 0xff;
	new->cert.v = new->pl->v + 1;
	new->cert.l = new->pl->l - 1;

	return new;
}

/*
 * get my CR.
 * NOTE: No Certificate Authority field is included to CR payload at the
 * moment. Becuase any certificate authority are accepted without any check.
 * The section 3.10 in RFC2408 says that this field SHOULD not be included,
 * if there is no specific certificate authority requested.
 */
vchar_t *
oakley_getcr(iph1)
	struct ph1handle *iph1;
{
	vchar_t *buf;

	buf = vmalloc(1);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get cr buffer\n");
		return NULL;
	}
	buf->v[0] = iph1->rmconf->certtype;

	plog(LLV_DEBUG, LOCATION, NULL, "create my CR: %s\n",
		s_isakmp_certtype(iph1->rmconf->certtype));
	if (buf->l > 1)
		plogdump(LLV_DEBUG, buf->v, buf->l);

	return buf;
}

/*
 * check peer's CR.
 */
int
oakley_checkcr(iph1)
	struct ph1handle *iph1;
{
	if (iph1->cr_p == NULL)
		return 0;

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		"peer transmitted CR: %s\n",
		s_isakmp_certtype(iph1->cr_p->type));

	if (iph1->cr_p->type != iph1->rmconf->certtype) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"such a cert type isn't supported: %d\n",
			(char)iph1->cr_p->type);
		return -1;
	}

	return 0;
}

/*
 * check to need CR payload.
 */
int
oakley_needcr(type)
	int type;
{
	switch (type) {
	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
		return 1;
	default:
		return 0;
	}
	/*NOTREACHED*/
}
#endif	/*HAVE_SIGNING_C*/

/*
 * compute SKEYID
 * see seciton 5. Exchanges in RFC 2409
 * psk: SKEYID = prf(pre-shared-key, Ni_b | Nr_b)
 * sig: SKEYID = prf(Ni_b | Nr_b, g^ir)
 * enc: SKEYID = prf(H(Ni_b | Nr_b), CKY-I | CKY-R)
 */
int
oakley_skeyid(iph1)
	struct ph1handle *iph1;
{
	vchar_t *buf = NULL, *bp;
	char *p;
	int len;
	int error = -1;

	/* SKEYID */
	switch(iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
		if (iph1->etype != ISAKMP_ETYPE_IDENT)
			iph1->authstr = getpskbyname(iph1->id_p);
		if (iph1->authstr == NULL) {
			/*
			 * If main mode or If failed to get psk by ID,
			 * we try to get it by remote IP address.
			 * It's may be nonsense.
			 */
			iph1->authstr = getpskbyaddr(iph1->remote);
			if (iph1->authstr == NULL) {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"couldn't find pskey for %s.\n",
					saddrwop2str(iph1->remote));
				goto end;
			}
		}
		plog(LLV_DEBUG, LOCATION, NULL, "psk found.\n");
		/* should be secret PSK */
		plog(LLV_DEBUG2, LOCATION, NULL, "psk: ");
		plogdump(LLV_DEBUG2, iph1->authstr->v, iph1->authstr->l);

		len = iph1->nonce->l + iph1->nonce_p->l;
		buf = vmalloc(len);
		if (buf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get skeyid buffer\n");
			goto end;
		}
		p = buf->v;

		bp = (iph1->side == INITIATOR ? iph1->nonce : iph1->nonce_p);
		plog(LLV_DEBUG, LOCATION, NULL, "nonce 1: ");
		plogdump(LLV_DEBUG, bp->v, bp->l);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		bp = (iph1->side == INITIATOR ? iph1->nonce_p : iph1->nonce);
		plog(LLV_DEBUG, LOCATION, NULL, "nonce 2: ");
		plogdump(LLV_DEBUG, bp->v, bp->l);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		iph1->skeyid = oakley_prf(iph1->authstr, buf, iph1);
		if (iph1->skeyid == NULL)
			goto end;
		break;

	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
#ifdef HAVE_GSSAPI
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
#endif
		len = iph1->nonce->l + iph1->nonce_p->l;
		buf = vmalloc(len);
		if (buf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get nonce buffer\n");
			goto end;
		}
		p = buf->v;

		bp = (iph1->side == INITIATOR ? iph1->nonce : iph1->nonce_p);
		plog(LLV_DEBUG, LOCATION, NULL, "nonce1: ");
		plogdump(LLV_DEBUG, bp->v, bp->l);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		bp = (iph1->side == INITIATOR ? iph1->nonce_p : iph1->nonce);
		plog(LLV_DEBUG, LOCATION, NULL, "nonce2: ");
		plogdump(LLV_DEBUG, bp->v, bp->l);
		memcpy(p, bp->v, bp->l);
		p += bp->l;

		iph1->skeyid = oakley_prf(buf, iph1->dhgxy, iph1);
		if (iph1->skeyid == NULL)
			goto end;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
		plog(LLV_WARNING, LOCATION, NULL,
			"not supported authentication method %s\n",
			s_oakley_attr_method(iph1->approval->authmethod));
		goto end;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid authentication method %d\n",
			iph1->approval->authmethod);
		goto end;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "SKEYID computed: ");
	plogdump(LLV_DEBUG, iph1->skeyid->v, iph1->skeyid->l);

	error = 0;

end:
	if (buf != NULL)
		vfree(buf);
	return error;
}

/*
 * compute SKEYID_[dae]
 * see seciton 5. Exchanges in RFC 2409
 * SKEYID_d = prf(SKEYID, g^ir | CKY-I | CKY-R | 0)
 * SKEYID_a = prf(SKEYID, SKEYID_d | g^ir | CKY-I | CKY-R | 1)
 * SKEYID_e = prf(SKEYID, SKEYID_a | g^ir | CKY-I | CKY-R | 2)
 */
int
oakley_skeyid_dae(iph1)
	struct ph1handle *iph1;
{
	vchar_t *buf = NULL;
	char *p;
	int len;
	int error = -1;

	if (iph1->skeyid == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "no SKEYID found.\n");
		goto end;
	}

	/* SKEYID D */
	/* SKEYID_d = prf(SKEYID, g^xy | CKY-I | CKY-R | 0) */
	len = iph1->dhgxy->l + sizeof(cookie_t) * 2 + 1;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get skeyid buffer\n");
		goto end;
	}
	p = buf->v;

	memcpy(p, iph1->dhgxy->v, iph1->dhgxy->l);
	p += iph1->dhgxy->l;
	memcpy(p, (caddr_t)&iph1->index.i_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	memcpy(p, (caddr_t)&iph1->index.r_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	*p = 0;
	iph1->skeyid_d = oakley_prf(iph1->skeyid, buf, iph1);
	if (iph1->skeyid_d == NULL)
		goto end;

	vfree(buf);
	buf = NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "SKEYID_d computed: ");
	plogdump(LLV_DEBUG, iph1->skeyid_d->v, iph1->skeyid->l);

	/* SKEYID A */
	/* SKEYID_a = prf(SKEYID, SKEYID_d | g^xy | CKY-I | CKY-R | 1) */
	len = iph1->skeyid_d->l + iph1->dhgxy->l + sizeof(cookie_t) * 2 + 1;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get skeyid buffer\n");
		goto end;
	}
	p = buf->v;
	memcpy(p, iph1->skeyid_d->v, iph1->skeyid_d->l);
	p += iph1->skeyid_d->l;
	memcpy(p, iph1->dhgxy->v, iph1->dhgxy->l);
	p += iph1->dhgxy->l;
	memcpy(p, (caddr_t)&iph1->index.i_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	memcpy(p, (caddr_t)&iph1->index.r_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	*p = 1;
	iph1->skeyid_a = oakley_prf(iph1->skeyid, buf, iph1);
	if (iph1->skeyid_a == NULL)
		goto end;

	vfree(buf);
	buf = NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "SKEYID_a computed: ");
	plogdump(LLV_DEBUG, iph1->skeyid_a->v, iph1->skeyid_a->l);

	/* SKEYID E */
	/* SKEYID_e = prf(SKEYID, SKEYID_a | g^xy | CKY-I | CKY-R | 2) */
	len = iph1->skeyid_a->l + iph1->dhgxy->l + sizeof(cookie_t) * 2 + 1;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get skeyid buffer\n");
		goto end;
	}
	p = buf->v;
	memcpy(p, iph1->skeyid_a->v, iph1->skeyid_a->l);
	p += iph1->skeyid_a->l;
	memcpy(p, iph1->dhgxy->v, iph1->dhgxy->l);
	p += iph1->dhgxy->l;
	memcpy(p, (caddr_t)&iph1->index.i_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	memcpy(p, (caddr_t)&iph1->index.r_ck, sizeof(cookie_t));
	p += sizeof(cookie_t);
	*p = 2;
	iph1->skeyid_e = oakley_prf(iph1->skeyid, buf, iph1);
	if (iph1->skeyid_e == NULL)
		goto end;

	vfree(buf);
	buf = NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "SKEYID_e computed: ");
	plogdump(LLV_DEBUG, iph1->skeyid_e->v, iph1->skeyid_e->l);

	error = 0;

end:
	if (buf != NULL)
		vfree(buf);
	return error;
}

/*
 * compute final encryption key.
 * see Appendix B.
 */
int
oakley_compute_enckey(iph1)
	struct ph1handle *iph1;
{
	u_int keylen, prflen;
	int error = -1;

	/* RFC2409 p39 */
	switch (iph1->approval->enctype) {
	case OAKLEY_ATTR_ENC_ALG_DES:
		keylen = 8;
		break;
	case OAKLEY_ATTR_ENC_ALG_IDEA:
		keylen = 16;
		break;
	case OAKLEY_ATTR_ENC_ALG_BLOWFISH:	/* can negotiate keylen */
		keylen = iph1->approval->encklen
			? (iph1->approval->encklen + 7) / 8 : 56;
		break;
	case OAKLEY_ATTR_ENC_ALG_RC5:		/* can negotiate encklen */
	case OAKLEY_ATTR_ENC_ALG_CAST:		/* can negotiate encklen */
		keylen = iph1->approval->encklen
			? (iph1->approval->encklen + 7) / 8 : 16;
		break;
	case OAKLEY_ATTR_ENC_ALG_3DES:
		keylen = 24;
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"encryption algoritym %d isn't supported.\n",
			iph1->approval->enctype);
		goto end;
	}

	switch (iph1->approval->dh_group) {
	default:
		switch (iph1->approval->hashtype) {
		case OAKLEY_ATTR_HASH_ALG_MD5:
			prflen = 16;
			break;
		case OAKLEY_ATTR_HASH_ALG_SHA:
			prflen = 20;
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"hash type %d isn't supported.\n",
				iph1->approval->hashtype);
			return 0;
			break;
		}
	}

	iph1->key = vmalloc(keylen);
	if (iph1->key == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get key buffer\n");
		goto end;
	}

	/* see isakmp-oakley-08 5.3. */
	if (iph1->key->l <= iph1->skeyid_e->l) {
		/*
		 * if length(Ka) <= length(SKEYID_e)
		 *	Ka = first length(K) bit of SKEYID_e
		 */
		memcpy(iph1->key->v, iph1->skeyid_e->v, iph1->key->l);
	} else {
		vchar_t *buf = NULL, *res = NULL;
		u_char *p, *ep;
		int cplen;
		int subkey;

		/*
		 * otherwise,
		 *	Ka = K1 | K2 | K3
		 * where
		 *	K1 = prf(SKEYID_e, 0)
		 *	K2 = prf(SKEYID_e, K1)
		 *	K3 = prf(SKEYID_e, K2)
		 */
		plog(LLV_DEBUG, LOCATION, NULL,
			"len(SKEYID_e) < len(Ka) (%d < %d), "
			"generating long key (Ka = K1 | K2 | ...)\n",
			iph1->skeyid_e->l, iph1->key->l);

		if ((buf = vmalloc(prflen)) == 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get key buffer\n");
			goto end;
		}
		p = (u_char *)iph1->key->v;
		ep = p + iph1->key->l;

		subkey = 1;
		while (p < ep) {
			if (p == (u_char *)iph1->key->v) {
				/* just for computing K1 */
				buf->v[0] = 0;
				buf->l = 1;
			}
			res = oakley_prf(iph1->skeyid_e, buf, iph1);
			if (res == NULL) {
				vfree(buf);
				goto end;
			}
			plog(LLV_DEBUG, LOCATION, NULL,
				"compute intermediate cipher key K%d\n",
				subkey);
			plogdump(LLV_DEBUG, buf->v, buf->l);
			plogdump(LLV_DEBUG, res->v, res->l);

			cplen = (res->l < ep - p) ? res->l : ep - p;
			memcpy(p, res->v, cplen);
			p += cplen;

			buf->l = prflen;	/* to cancel K1 speciality */
			if (res->l != buf->l) {
				plog(LLV_ERROR, LOCATION, NULL,
					"internal error: res->l=%d buf->l=%d\n",
					res->l, buf->l);
				vfree(res);
				vfree(buf);
				goto end;
			}
			memcpy(buf->v, res->v, res->l);
			vfree(res);
			subkey++;
		}

		vfree(buf);
	}

	/*
	 * don't check any weak key or not.
	 * draft-ietf-ipsec-ike-01.txt Appendix B.
	 * draft-ietf-ipsec-ciph-aes-cbc-00.txt Section 2.3.
	 */
#if 0
	/* weakkey check */
	if (iph1->approval->enctype > ARRAYLEN(cipher))
		goto end;
	if (cipher[iph1->approval->enctype].weakkey == NULL
	 && (cipher[iph1->approval->enctype].weakkey)(iph1->key)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"weakkey was generated.\n");
		goto end;
	}
#endif

	plog(LLV_DEBUG, LOCATION, NULL, "final cipher key computed: ");
	plogdump(LLV_DEBUG, iph1->key->v, iph1->key->l);

	error = 0;

end:
	return error;
}

/* allocated new buffer for CERT */
cert_t *
oakley_newcert()
{
	cert_t *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get cert's buffer\n");
		return NULL;
	}

	new->pl = NULL;

	return new;
}

/* delete buffer for CERT */
void
oakley_delcert(cert)
	cert_t *cert;
{
	if (!cert)
		return;
	if (cert->pl)
		VPTRINIT(cert->pl);
	racoon_free(cert);
}

/*
 * compute IV and set to ph1handle
 *	IV = hash(g^xi | g^xr)
 * see 4.1 Phase 1 state in draft-ietf-ipsec-ike.
 */
int
oakley_newiv(iph1)
	struct ph1handle *iph1;
{
	struct isakmp_ivm *newivm = NULL;
	vchar_t *buf = NULL, *bp;
	char *p;
	int len;

	/* create buffer */
	len = iph1->dhpub->l + iph1->dhpub_p->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get iv buffer\n");
		return -1;
	}

	p = buf->v;

	bp = (iph1->side == INITIATOR ? iph1->dhpub : iph1->dhpub_p);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	bp = (iph1->side == INITIATOR ? iph1->dhpub_p : iph1->dhpub);
	memcpy(p, bp->v, bp->l);
	p += bp->l;

	/* allocate IVm */
	newivm = racoon_calloc(1, sizeof(struct isakmp_ivm));
	if (newivm == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get iv buffer\n");
		vfree(buf);
		return -1;
	}

	/* compute IV */
	newivm->iv = oakley_hash(buf, iph1);
	if (newivm->iv == NULL) {
		vfree(buf);
		oakley_delivm(newivm);
		return -1;
	}

	/* adjust length of iv */
	newivm->iv->l = CBC_BLOCKLEN;

	/* create buffer to save iv */
	if ((newivm->ive = vdup(newivm->iv)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"vdup (%s)\n", strerror(errno));
		vfree(buf);
		oakley_delivm(newivm);
		return -1;
	}

	vfree(buf);

	plog(LLV_DEBUG, LOCATION, NULL, "IV computed: ");
	plogdump(LLV_DEBUG, newivm->iv->v, newivm->iv->l);

	iph1->ivm = newivm;

	return 0;
}

/*
 * compute IV for the payload after phase 1.
 * It's not limited for phase 2.
 * if pahse 1 was encrypted.
 *	IV = hash(last CBC block of Phase 1 | M-ID)
 * if phase 1 was not encrypted.
 *	IV = hash(phase 1 IV | M-ID)
 * see 4.2 Phase 2 state in draft-ietf-ipsec-ike.
 */
struct isakmp_ivm *
oakley_newiv2(iph1, msgid)
	struct ph1handle *iph1;
	u_int32_t msgid;
{
	struct isakmp_ivm *newivm = NULL;
	vchar_t *buf = NULL;
	char *p;
	int len;
	int error = -1;

	/* create buffer */
	len = iph1->ivm->iv->l + sizeof(msgid_t);
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get iv buffer\n");
		goto end;
	}

	p = buf->v;

	memcpy(p, iph1->ivm->iv->v, iph1->ivm->iv->l);
	p += iph1->ivm->iv->l;

	memcpy(p, &msgid, sizeof(msgid));

	plog(LLV_DEBUG, LOCATION, NULL, "compute IV for phase2\n");
	plog(LLV_DEBUG, LOCATION, NULL, "phase1 last IV: ");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* allocate IVm */
	newivm = racoon_calloc(1, sizeof(struct isakmp_ivm));
	if (newivm == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get iv buffer\n");
		goto end;
	}

	/* compute IV */
	if ((newivm->iv = oakley_hash(buf, iph1)) == NULL)
		goto end;

	/* adjust length of iv */
	newivm->iv->l = CBC_BLOCKLEN;

	/* create buffer to save new iv */
	if ((newivm->ive = vdup(newivm->iv)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "vdup (%s)\n", strerror(errno));
		goto end;
	}

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "phase2 IV computed: ");
	plogdump(LLV_DEBUG, newivm->iv->v, newivm->iv->l);

end:
	if (error && newivm != NULL)
		oakley_delivm(newivm);
	if (buf != NULL)
		vfree(buf);
	return newivm;
}

void
oakley_delivm(ivm)
	struct isakmp_ivm *ivm;
{
	if (ivm == NULL)
		return;

	if (ivm->iv != NULL)
		vfree(ivm->iv);
	if (ivm->ive != NULL)
		vfree(ivm->ive);
	racoon_free(ivm);

	return;
}

/*
 * decrypt packet.
 *   save new iv and old iv.
 */
vchar_t *
oakley_do_decrypt(iph1, msg, ivdp, ivep)
	struct ph1handle *iph1;
	vchar_t *msg, *ivdp, *ivep;
{
	vchar_t *buf = NULL, *new = NULL;
	char *pl;
	int len;
	u_int8_t padlen;
	int error = -1;

	plog(LLV_DEBUG, LOCATION, NULL, "begin decryption.\n");

	/* save IV for next, but not sync. */
	memset(ivep->v, 0, ivep->l);
	memcpy(ivep->v, (caddr_t)&msg->v[msg->l - CBC_BLOCKLEN], CBC_BLOCKLEN);

	plog(LLV_DEBUG, LOCATION, NULL,
		"IV was saved for next processing: ");
	plogdump(LLV_DEBUG, ivep->v, ivep->l);

	pl = msg->v + sizeof(struct isakmp);

	len = msg->l - sizeof(struct isakmp);

	/* create buffer */
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to decrypt.\n");
		goto end;
	}
	memcpy(buf->v, pl, len);

	/* do decrypt */
	if (iph1->approval->enctype > ARRAYLEN(cipher)
	 && cipher[iph1->approval->enctype].decrypt == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid cipher algoriym was passed.\n");
		goto end;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"decrypt(%s)\n",
		cipher[iph1->approval->enctype].name);
	plog(LLV_DEBUG, LOCATION, NULL, "with key: ");
	plogdump(LLV_DEBUG, iph1->key->v, iph1->key->l);

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	new = (cipher[iph1->approval->enctype].decrypt)(buf, iph1->key, ivdp->v);
#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s size=%d): %8.6f", __FUNCTION__,
		s_attr_isakmp_enc(iph1->approval->enctype),
		buf->l, timedelta(&start, &end));
    }
#endif
	vfree(buf);
	buf = NULL;
	if (new == NULL)
		goto end;

	plog(LLV_DEBUG, LOCATION, NULL, "decrypted payload by IV: ");
	plogdump(LLV_DEBUG, ivdp->v, ivdp->l);

	plog(LLV_DEBUG, LOCATION, NULL,
		"decrypted payload, but not trimed.\n");
	plogdump(LLV_DEBUG, new->v, new->l);

	/* get padding length */
	if (lcconf->pad_excltail)
		padlen = new->v[new->l - 1] + 1;
	else
		padlen = new->v[new->l - 1];
	plog(LLV_DEBUG, LOCATION, NULL, "padding len=%u\n", padlen);

	/* trim padding */
	if (lcconf->pad_strict) {
		if (padlen > new->l) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalied padding len=%u, buflen=%u.\n",
				padlen, new->l);
			plogdump(LLV_ERROR, new->v, new->l);
			goto end;
		}
		new->l -= padlen;
		plog(LLV_DEBUG, LOCATION, NULL, "trimmed padding\n");
	} else {
		plog(LLV_DEBUG, LOCATION, NULL, "skip to trim padding.\n");
	}

	/* create new buffer */
	len = sizeof(struct isakmp) + new->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to decrypt.\n");
		goto end;
	}
	memcpy(buf->v, msg->v, sizeof(struct isakmp));
	memcpy(buf->v + sizeof(struct isakmp), new->v, new->l);
	((struct isakmp *)buf->v)->len = htonl(buf->l);

	plog(LLV_DEBUG, LOCATION, NULL, "decrypted.\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(buf, iph1->remote, iph1->local, 1);
#endif

	error = 0;

end:
	if (error && buf != NULL) {
		vfree(buf);
		buf = NULL;
	}
	if (new != NULL)
		vfree(new);

	return buf;
}

/*
 * encrypt packet.
 */
vchar_t *
oakley_do_encrypt(iph1, msg, ivep, ivp)
	struct ph1handle *iph1;
	vchar_t *msg, *ivep, *ivp;
{
	vchar_t *buf = 0, *new = 0;
	char *pl;
	int len;
	u_int padlen;
	int error = -1;

	plog(LLV_DEBUG, LOCATION, NULL, "begin encryption.\n");

	pl = msg->v + sizeof(struct isakmp);
	len = msg->l - sizeof(struct isakmp);

	/* add padding */
	padlen = oakley_padlen(len);
	plog(LLV_DEBUG, LOCATION, NULL, "pad length = %u\n", padlen);

	/* create buffer */
	buf = vmalloc(len + padlen);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to encrypt.\n");
		goto end;
	}
        if (padlen) {
                int i;
		char *p = &buf->v[len];
		if (lcconf->pad_random) {
			for (i = 0; i < padlen; i++)
				*p++ = (char)random();
		}
        }
        memcpy(buf->v, pl, len);

	/* make pad into tail */
	if (lcconf->pad_excltail)
		buf->v[len + padlen - 1] = padlen - 1;
	else
		buf->v[len + padlen - 1] = padlen;

	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* do encrypt */
	if (iph1->approval->enctype > ARRAYLEN(cipher)
	 && cipher[iph1->approval->enctype].encrypt == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid cipher algoriym was passed.\n");
		goto end;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"encrypt(%s).\n",
		cipher[iph1->approval->enctype].name);
	plog(LLV_DEBUG, LOCATION, NULL, "with key: ");
	plogdump(LLV_DEBUG, iph1->key->v, iph1->key->l);

#ifdef ENABLE_STATS
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
#endif
	new = (cipher[iph1->approval->enctype].encrypt)(buf, iph1->key, ivep->v);
#ifdef ENABLE_STATS
	gettimeofday(&end, NULL);
	syslog(LOG_NOTICE, "%s(%s size=%d): %8.6f", __FUNCTION__,
		s_attr_isakmp_enc(iph1->approval->enctype),
		buf->l, timedelta(&start, &end));
    }
#endif
	vfree(buf);
	buf = NULL;
	if (new == NULL)
		goto end;

	plog(LLV_DEBUG, LOCATION, NULL, "encrypted payload by IV: ");
	plogdump(LLV_DEBUG, ivep->v, ivep->l);

	/* save IV for next */
	memset(ivp->v, 0, ivp->l);
	memcpy(ivp->v, (caddr_t)&new->v[new->l - CBC_BLOCKLEN], CBC_BLOCKLEN);

	plog(LLV_DEBUG, LOCATION, NULL, "save IV for next: ");
	plogdump(LLV_DEBUG, ivp->v, ivp->l);

	/* create new buffer */
	len = sizeof(struct isakmp) + new->l;
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to encrypt.\n");
		goto end;
	}
	memcpy(buf->v, msg->v, sizeof(struct isakmp));
	memcpy(buf->v + sizeof(struct isakmp), new->v, new->l);
	((struct isakmp *)buf->v)->len = htonl(buf->l);

	error = 0;

	plog(LLV_DEBUG, LOCATION, NULL, "encrypted.\n");

end:
	if (error && buf != NULL) {
		vfree(buf);
		buf = NULL;
	}
	if (new != NULL)
		vfree(new);

	return buf;
}

/* culculate padding length */
int
oakley_padlen(len)
	int len;
{
	int padlen;
	int base = CBC_BLOCKLEN;

	padlen = 8 - len % 8;

	if (lcconf->pad_randomlen)
		padlen += ((random() % (lcconf->pad_maxsize + 1) + 1) * base);

	return padlen;
}

