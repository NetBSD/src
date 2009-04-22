/*	$NetBSD: isakmp_frag.c,v 1.5 2009/04/22 11:24:20 tteras Exp $	*/

/* Id: isakmp_frag.c,v 1.4 2004/11/13 17:31:36 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/md5.h> 

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
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
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "isakmp_var.h"
#include "isakmp.h"
#include "handler.h"
#include "isakmp_frag.h"
#include "strnames.h"

int
isakmp_sendfrags(iph1, buf) 
	struct ph1handle *iph1;
	vchar_t *buf;
{
	struct isakmp *hdr;
	struct isakmp_frag *fraghdr;
	caddr_t data;
	caddr_t sdata;
	size_t datalen;
	size_t max_datalen;
	size_t fraglen;
	vchar_t *frag;
	unsigned int trailer;
	unsigned int fragnum = 0;
	size_t len;
	int etype;

	/*
	 * Catch the exchange type for later: the fragments and the
	 * fragmented packet must have the same exchange type.
	 */
	hdr = (struct isakmp *)buf->v;
	etype = hdr->etype;

	/*
	 * We want to send a a packet smaller than ISAKMP_FRAG_MAXLEN
	 * First compute the maximum data length that will fit in it
	 */
	max_datalen = ISAKMP_FRAG_MAXLEN - 
	    (sizeof(*hdr) + sizeof(*fraghdr) + sizeof(trailer));

	sdata = buf->v;
	len = buf->l;

	while (len > 0) {
		fragnum++;

		if (len > max_datalen)
			datalen = max_datalen;
		else
			datalen = len;

		fraglen = sizeof(*hdr) 
			+ sizeof(*fraghdr) 
			+ datalen;

		if ((frag = vmalloc(fraglen)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot allocate memory\n");
			return -1;
		}

		set_isakmp_header1(frag, iph1, ISAKMP_NPTYPE_FRAG);
		hdr = (struct isakmp *)frag->v;
		hdr->etype = etype;

		fraghdr = (struct isakmp_frag *)(hdr + 1);
		fraghdr->unknown0 = htons(0);
		fraghdr->len = htons(fraglen - sizeof(*hdr));
		fraghdr->unknown1 = htons(1);
		fraghdr->index = fragnum;
		if (len == datalen)
			fraghdr->flags = ISAKMP_FRAG_LAST;
		else
			fraghdr->flags = 0;

		data = (caddr_t)(fraghdr + 1);
		memcpy(data, sdata, datalen);

		if (isakmp_send(iph1, frag) < 0) {
			plog(LLV_ERROR, LOCATION, NULL, "isakmp_send failed\n");
			return -1;
		}

		vfree(frag);

		len -= datalen;
		sdata += datalen;
	}
		
	return fragnum;
}

unsigned int 
vendorid_frag_cap(gen)
	struct isakmp_gen *gen;
{
	int *hp;

	hp = (int *)(gen + 1);

	return ntohl(hp[MD5_DIGEST_LENGTH / sizeof(*hp)]);
}

int 
isakmp_frag_extract(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	struct isakmp *isakmp;
	struct isakmp_frag *frag;
	struct isakmp_frag_item *item;
	vchar_t *buf;
	size_t len;
	int last_frag = 0;
	char *data;
	int i;

	if (msg->l < sizeof(*isakmp) + sizeof(*frag)) {
		plog(LLV_ERROR, LOCATION, NULL, "Message too short\n");
		return -1;
	}

	isakmp = (struct isakmp *)msg->v;
	frag = (struct isakmp_frag *)(isakmp + 1);

	/* 
	 * frag->len is the frag payload data plus the frag payload header,
	 * whose size is sizeof(*frag) 
	 */
	if (msg->l < sizeof(*isakmp) + ntohs(frag->len) ||
	    ntohs(frag->len) < sizeof(*frag) + 1) {
		plog(LLV_ERROR, LOCATION, NULL, "Fragment too short\n");
		return -1;
	}

	if ((buf = vmalloc(ntohs(frag->len) - sizeof(*frag))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate memory\n");
		return -1;
	}

	if ((item = racoon_malloc(sizeof(*item))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate memory\n");
		vfree(buf);
		return -1;
	}

	data = (char *)(frag + 1);
	memcpy(buf->v, data, buf->l);

	item->frag_num = frag->index;
	item->frag_last = (frag->flags & ISAKMP_FRAG_LAST);
	item->frag_next = NULL;
	item->frag_packet = buf;

	/* Look for the last frag while inserting the new item in the chain */
	if (item->frag_last)
		last_frag = item->frag_num;

	if (iph1->frag_chain == NULL) {
		iph1->frag_chain = item;
	} else {
		struct isakmp_frag_item *current;

		current = iph1->frag_chain;
		while (current->frag_next) {
			if (current->frag_last)
				last_frag = item->frag_num;
			current = current->frag_next;
		}
		current->frag_next = item;
	}

	/* If we saw the last frag, check if the chain is complete */
	if (last_frag != 0) {
		for (i = 1; i <= last_frag; i++) {
			item = iph1->frag_chain;
			do {
				if (item->frag_num == i)
					break;
				item = item->frag_next;
			} while (item != NULL);

			if (item == NULL) /* Not found */
				break;
		}

		if (item != NULL) /* It is complete */
			return 1;
	}
		
	return 0;
}

vchar_t *
isakmp_frag_reassembly(iph1)
	struct ph1handle *iph1;
{
	struct isakmp_frag_item *item;
	size_t len = 0;
	vchar_t *buf = NULL;
	int frag_count = 0;
	int i;
	char *data;

	if ((item = iph1->frag_chain) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "No fragment to reassemble\n");
		goto out;
	}

	do {
		frag_count++;
		len += item->frag_packet->l;
		item = item->frag_next;
	} while (item != NULL);
	
	if ((buf = vmalloc(len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate memory\n");
		goto out;
	}
	data = buf->v;

	for (i = 1; i <= frag_count; i++) {
		item = iph1->frag_chain;
		do {
			if (item->frag_num == i)
				break;
			item = item->frag_next;
		} while (item != NULL);

		if (item == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Missing fragment #%d\n", i);
			vfree(buf);
			buf = NULL;
			goto out;
		}
		memcpy(data, item->frag_packet->v, item->frag_packet->l);
		data += item->frag_packet->l;
	}

out:
	item = iph1->frag_chain;		
	do {
		struct isakmp_frag_item *next_item;

		next_item = item->frag_next;

		vfree(item->frag_packet);
		racoon_free(item);

		item = next_item;
	} while (item != NULL);

	iph1->frag_chain = NULL;

	return buf;
}

vchar_t *
isakmp_frag_addcap(buf, cap)
	vchar_t *buf;
	int cap;
{
	int *capp;
	size_t len;

	/* If the capability has not been added, add room now */
	len = buf->l;
	if (len == MD5_DIGEST_LENGTH) {
		if ((buf = vrealloc(buf, len + sizeof(cap))) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot allocate memory\n");
			return NULL;
		}
		capp = (int *)(buf->v + len);
		*capp = htonl(0);
	}

	capp = (int *)(buf->v + MD5_DIGEST_LENGTH);
	*capp |= htonl(cap);

	return buf;
}

