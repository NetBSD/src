/*	$KAME: isakmp_newg.c,v 1.10 2002/09/27 05:55:52 itojun Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: isakmp_newg.c,v 1.2 2003/07/12 09:37:10 itojun Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "schedule.h"
#include "cfparse_proto.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "isakmp_newg.h"
#include "oakley.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "handler.h"
#include "pfkey.h"
#include "admin.h"
#include "str2val.h"
#include "vendorid.h"

/*
 * New group mode as responder
 */
int
isakmp_newgroup_r(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
#if 0
	struct isakmp *isakmp = (struct isakmp *)msg->v;
	struct isakmp_pl_hash *hash = NULL;
	struct isakmp_pl_sa *sa = NULL;
	int error = -1;
	vchar_t *buf;
	struct oakley_sa *osa;
	int len;

	/* validate the type of next payload */
	/*
	 * ISAKMP_ETYPE_NEWGRP,
	 * ISAKMP_NPTYPE_HASH, (ISAKMP_NPTYPE_VID), ISAKMP_NPTYPE_SA,
	 * ISAKMP_NPTYPE_NONE
	 */
    {
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;

	if ((pbuf = isakmp_parse(msg)) == NULL)
		goto end;

	for (pa = (struct isakmp_parse_t *)pbuf->v;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {

		switch (pa->type) {
		case ISAKMP_NPTYPE_HASH:
			if (hash) {
				isakmp_info_send_n1(iph1, ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE, NULL);
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"received multiple payload type %d.\n",
					pa->type);
				vfree(pbuf);
				goto end;
			}
			hash = (struct isakmp_pl_hash *)pa->ptr;
			break;
		case ISAKMP_NPTYPE_SA:
			if (sa) {
				isakmp_info_send_n1(iph1, ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE, NULL);
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"received multiple payload type %d.\n",
					pa->type);
				vfree(pbuf);
				goto end;
			}
			sa = (struct isakmp_pl_sa *)pa->ptr;
			break;
		case ISAKMP_NPTYPE_VID:
			(void)check_vendorid(pa->ptr);
			break;
		default:
			isakmp_info_send_n1(iph1, ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE, NULL);
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"ignore the packet, "
				"received unexpecting payload type %d.\n",
				pa->type);
			vfree(pbuf);
			goto end;
		}
	}
	vfree(pbuf);

	if (!hash || !sa) {
		isakmp_info_send_n1(iph1, ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE, NULL);
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"no HASH, or no SA payload.\n");
		goto end;
	}
    }

	/* validate HASH */
    {
	char *r_hash;
	vchar_t *my_hash = NULL;
	int result;

	plog(LLV_DEBUG, LOCATION, NULL, "validate HASH\n");

	len = sizeof(isakmp->msgid) + ntohs(sa->h.len);
	buf = vmalloc(len);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		goto end;
	}
	memcpy(buf->v, &isakmp->msgid, sizeof(isakmp->msgid));
	memcpy(buf->v + sizeof(isakmp->msgid), sa, ntohs(sa->h.len));

	plog(LLV_DEBUG, LOCATION, NULL, "hash source\n");
	plogdump(LLV_DEBUG, buf->v, buf->l);

	my_hash = isakmp_prf(iph1->skeyid_a, buf, iph1);
	vfree(buf);
	if (my_hash == NULL)
		goto end;

	plog(LLV_DEBUG, LOCATION, NULL, "hash result\n");
	plogdump(LLV_DEBUG, my_hash->v, my_hash->l);

	r_hash = (char *)hash + sizeof(*hash);

	plog(LLV_DEBUG, LOCATION, NULL, "original hash\n"));
	plogdump(LLV_DEBUG, r_hash, ntohs(hash->h.len) - sizeof(*hash)));

	result = memcmp(my_hash->v, r_hash, my_hash->l);
	vfree(my_hash);

	if (result) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"HASH mismatch.\n");
		isakmp_info_send_n1(iph1, ISAKMP_NTYPE_INVALID_HASH_INFORMATION, NULL);
		goto end;
	}
    }

	/* check SA payload and get new one for use */
	buf = ipsecdoi_get_proposal((struct ipsecdoi_sa *)sa,
					OAKLEY_NEWGROUP_MODE);
	if (buf == NULL) {
		isakmp_info_send_n1(iph1, ISAKMP_NTYPE_ATTRIBUTES_NOT_SUPPORTED, NULL);
		goto end;
	}

	/* save sa parameters */
	osa = ipsecdoi_get_oakley(buf);
	if (osa == NULL) {
		isakmp_info_send_n1(iph1, ISAKMP_NTYPE_ATTRIBUTES_NOT_SUPPORTED, NULL);
		goto end;
	}
	vfree(buf);

	switch (osa->dhgrp) {
	case OAKLEY_ATTR_GRP_DESC_MODP768:
	case OAKLEY_ATTR_GRP_DESC_MODP1024:
	case OAKLEY_ATTR_GRP_DESC_MODP1536:
		/*XXX*/
	default:
		isakmp_info_send_n1(iph1, ISAKMP_NTYPE_ATTRIBUTES_NOT_SUPPORTED, NULL);
		plog(LLV_ERROR, LOCATION, NULL,
			"dh group %d isn't supported.\n", osa->dhgrp);
		goto end;
	}

	plog(LLV_INFO, LOCATION, iph1->remote,
		"got new dh group %s.\n", isakmp_pindex(&iph1->index, 0));

	error = 0;

end:
	if (error) {
		if (iph1 != NULL)
			(void)isakmp_free_ph1(iph1);
	}
	return error;
#endif
	return 0;
}

