/*	$NetBSD: isapnpres.c,v 1.2 1997/01/22 23:51:38 mycroft Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * Resource parser for Plug and Play cards.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>


static int isapnp_wait_status __P((struct isapnp_softc *));
static struct isapnp_attach_args *isapnp_process_tag __P((u_char, u_char,
    u_char *, struct isapnp_attach_args *));


/* isapnp_wait_status():
 *	Wait for the next byte of resource data to become available
 */
static int
isapnp_wait_status(sc)
	struct isapnp_softc *sc;
{
	int i;

	for (i = 0; i < 10; i++) {
		if (isapnp_read_reg(sc, ISAPNP_STATUS) & 1)
			return 0;
		DELAY(100);
	}
	return 1;
}

/* isapnp_process_tag():
 *	Process a resource tag
 */
static struct isapnp_attach_args *
isapnp_process_tag(tag, len, buf, pa)
	u_char tag, len, *buf;
	struct isapnp_attach_args *pa;
{
	struct isapnp_attach_args *npa;
	char str[64];
	struct isapnp_region *r;
	struct isapnp_pin *p;

#define COPY(a, b) strncpy((a), (b), sizeof(a)), (a)[sizeof(a) - 1] = '\0'

	switch (tag) {
	case ISAPNP_TAG_VERSION_NUM:
		DPRINTF(("PnP version %d.%d, Vendor version %d.%d\n",
		    buf[0] >> 4, buf[0] & 0xf, buf[1] >> 4,  buf[1] & 0xf));
		break;

	case ISAPNP_TAG_LOGICAL_DEV_ID:
		(void) isapnp_id_to_vendor(str, buf);
		DPRINTF(("Logical device id %s\n", str));
		COPY(pa->ipa_devlogic, str);
		break;

	case ISAPNP_TAG_COMPAT_DEV_ID:
		(void) isapnp_id_to_vendor(str, buf);
		DPRINTF(("Compatible device id %s\n", str));
		break;
		
	case ISAPNP_TAG_IRQ_FORMAT:
		if (len < 2)
			break;

		if (len != 3)
			buf[2] = ISAPNP_IRQTYPE_EDGE_PLUS;

		p = &pa->ipa_irq[pa->ipa_nirq++];
		p->bits = buf[0] | (buf[1] << 8);
		p->flags = buf[2];
#ifdef DEBUG_ISAPNP
		isapnp_print_irq("", p);
#endif
		break;

	case ISAPNP_TAG_DMA_FORMAT:
		if (buf[0] == 0)
			break;

		p = &pa->ipa_drq[pa->ipa_ndrq++];
		p->bits = buf[0];
		p->flags = buf[1];
#ifdef DEBUG_ISAPNP
		isapnp_print_drq("", p);
#endif
		break;

	case ISAPNP_TAG_DEP_START:
		if (len == 0)
			buf[0] = ISAPNP_DEP_ACCEPTABLE;
		if (pa->ipa_pref != ISAPNP_DEP_UNSET) {
			npa = ISAPNP_MALLOC(sizeof(*npa));
			memset(npa, 0, sizeof(*npa));
			memcpy(npa->ipa_devident, pa->ipa_devident,
			    sizeof(pa->ipa_devident));
			memcpy(npa->ipa_devlogic, pa->ipa_devlogic,
			    sizeof(pa->ipa_devlogic));
			memcpy(npa->ipa_devclass, pa->ipa_devclass,
			    sizeof(pa->ipa_devclass));
			pa->ipa_next = npa;
			pa = npa;
		}
		pa->ipa_pref = buf[0];
#ifdef DEBUG_ISAPNP
		isapnp_print_dep_start(">>> Start dependent functions ",
		    pa->ipa_pref);
#endif
		break;
		
	case ISAPNP_TAG_DEP_END:
		DPRINTF(("<<<End dependend functions\n"));
		npa = ISAPNP_MALLOC(sizeof(*npa));
		memset(npa, 0, sizeof(*npa));
		npa->ipa_pref = ISAPNP_DEP_UNSET;
		memcpy(npa->ipa_devident, pa->ipa_devident,
		    sizeof(pa->ipa_devident));

		pa->ipa_next = npa;
		pa = npa;
		break;

	case ISAPNP_TAG_IO_PORT_DESC:
		r = &pa->ipa_io[pa->ipa_nio++];
		r->flags = buf[0];
		r->minbase = (buf[2] << 8) | buf[1];
		r->maxbase = (buf[4] << 8) | buf[3];
		r->align = buf[5];
		r->length = buf[6];

#ifdef DEBUG_ISAPNP
		isapnp_print_io("", r);
#endif
		break;

	case ISAPNP_TAG_FIXED_IO_PORT_DESC:
		r = &pa->ipa_io[pa->ipa_nio++];
		r->flags = 0;
		r->minbase = (buf[2] << 8) | buf[1];
		r->maxbase = r->minbase;
		r->align = 1;
		r->length = buf[3];

#ifdef DEBUG_ISAPNP
		isapnp_print_io("FIXED", r);
#endif
		break;

	case ISAPNP_TAG_RESERVED1:
	case ISAPNP_TAG_RESERVED2:
	case ISAPNP_TAG_RESERVED3:
	case ISAPNP_TAG_RESERVED4:
		break;

	case ISAPNP_TAG_VENDOR_DEF:
		break;

	case ISAPNP_TAG_END:
		break;

	case ISAPNP_TAG_MEM_RANGE_DESC:
		r = &pa->ipa_mem[pa->ipa_nmem++];
		r->minbase = (buf[2] << 8) | buf[1];
		r->maxbase = (buf[4] << 8) | buf[3];
		r->align = (buf[6] << 8) | buf[5];
		r->length = (buf[8] << 8) | buf[7];
#ifdef DEBUG_ISAPNP
		isapnp_print_mem("16 bit", r);
#endif
		break;

	case ISAPNP_TAG_ANSI_IDENT_STRING:
		buf[len] = '\0';
		DPRINTF(("ANSI Ident: %s\n", buf));
		if (pa->ipa_devident[0] == '\0')
			COPY(pa->ipa_devident, buf);
		else
			COPY(pa->ipa_devclass, buf);
		break;

	case ISAPNP_TAG_UNICODE_IDENT_STRING:
		buf[len] = '\0';
		DPRINTF(("Unicode Ident: %s\n", buf));
		break;

	case ISAPNP_TAG_VENDOR_DEFINED:
		break;

	case ISAPNP_TAG_MEM32_RANGE_DESC:
		r = &pa->ipa_mem32[pa->ipa_nmem32++];
		r->flags = buf[0];
		r->minbase = (buf[4] << 24) | (buf[3] << 16) |
		    (buf[2] << 8) | buf[1];
		r->maxbase = (buf[8] << 24) | (buf[7] << 16) |
		    (buf[6] << 8) | buf[5];
		r->align = (buf[12] << 24) | (buf[11] << 16) | 
		    (buf[10] << 8) | buf[9];
		r->length = (buf[16] << 24) | (buf[15] << 16) |
		    (buf[14] << 8) | buf[13];
#ifdef DEBUG_ISAPNP
		isapnp_print_mem("32 bit", r);
#endif
		break;

	case ISAPNP_TAG_FIXED_MEM32_RANGE_DESC:
		r = &pa->ipa_mem32[pa->ipa_nmem32++];
		r->flags = buf[0];
		r->minbase = (buf[4] << 24) | (buf[3] << 16) |
		    (buf[2] << 8) | buf[1];
		r->maxbase = r->minbase;
		r->align = 1;
		r->length = (buf[8] << 24) | (buf[7] << 16) |
		    (buf[6] << 8) | buf[5];
#ifdef DEBUG_ISAPNP
		isapnp_print_mem("FIXED 32 bit", r);
#endif
		break;

	default:
#ifdef DEBUG_ISAPNP
		{
			int i;
			printf("tag %.2x, len %d: ", tag, len);
			for (i = 0; i < len; i++)
				printf("%.2x ", buf[i]);
			printf("\n");
		}
#endif
		break;
	}
	return pa;
}


/* isapnp_get_resource():
 *	Read the resources for card c
 */
struct isapnp_attach_args *
isapnp_get_resource(sc, c)
	struct isapnp_softc *sc;
	int c;
{
	u_char d, tag;
	u_short len;
	int i;
	struct isapnp_attach_args *ipa, *pa;

	pa = ipa = ISAPNP_MALLOC(sizeof(*ipa));
	memset(ipa, 0, sizeof(*ipa));
	pa->ipa_pref = ISAPNP_DEP_UNSET;

	for (i = 0; i < ISAPNP_SERIAL_SIZE; i++) {
		if (isapnp_wait_status(sc))
			goto bad;

		d = isapnp_read_reg(sc, ISAPNP_RESOURCE_DATA);

		if (d != sc->sc_id[c][i] && i != ISAPNP_SERIAL_SIZE - 1) {
			printf("isapnp: card %d violates PnP spec; byte %d\n",
			    c + 1, i);
			break;
		}
	}

	do {
		u_char buf[ISAPNP_MAX_TAGSIZE], *p;

		memset(buf, 0, sizeof(buf));

#define NEXT_BYTE \
		if (isapnp_wait_status(sc)) \
			goto bad; \
		d = isapnp_read_reg(sc, ISAPNP_RESOURCE_DATA);
		
		NEXT_BYTE;

		if (d & ISAPNP_LARGE_TAG) {
			tag = d;
			NEXT_BYTE;
			buf[0] = d;
			NEXT_BYTE;
			buf[1] = d;
			len = (buf[1] << 8) | buf[0];
		}
		else {
			tag = (d >> 3) & 0xf;
			len = d & 0x7;
		}

		for (p = buf, i = 0; i < len; i++) {
			NEXT_BYTE;
			if (i < ISAPNP_MAX_TAGSIZE)
				*p++ = d;
		}

		if (len >= ISAPNP_MAX_TAGSIZE) {
			printf("isapnp: Maximum tag size exceeded, card %d\n",
			    c + 1);
			len = ISAPNP_MAX_TAGSIZE;
		}

		pa = isapnp_process_tag(tag, len, buf, pa);
	}
	while (tag != ISAPNP_TAG_END);
	return ipa;
bad:
	while (ipa) {
		pa = ipa->ipa_next;
		ISAPNP_FREE(ipa);
		ipa = pa;
	}
	printf("isapnp: Resource timeout, card %d\n", c + 1);
	return NULL;
}
