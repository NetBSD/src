/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <ipkdb/ipkdb.h>
#include <machine/ipkdb.h>

/* These should be in <net/if_sl.h> or some such! */
#define	FRAME_END		0xc0		/* Frame End */
#define	FRAME_ESCAPE		0xdb		/* Frame Esc */
#define	TRANS_FRAME_END		0xdc		/* transposed frame end */
#define	TRANS_FRAME_ESCAPE	0xdd		/* transposed frame esc */

struct cfdriver ipkdbslip_cd = {
	NULL, "ipkdb", DV_DULL
};

static int
ipkdbrcv(kip, buf, poll)
	struct ipkdb_if *kip;
	u_char *buf;
	int poll;
{
	int c, len = 0;

	/* Fake an ether header: */
	ipkdbcopy(kip->myenetaddr, buf, sizeof kip->myenetaddr);
	buf += sizeof kip->myenetaddr;
	ipkdbcopy(kip->hisenetaddr, buf, sizeof kip->hisenetaddr);
	buf += sizeof kip->hisenetaddr;
	*buf++ = ETHERTYPE_IP >> 8;
	*buf++ = (u_char)ETHERTYPE_IP;
	do {
		switch ((c = kip->getc(kip, poll))) {
		case -1:
			break;
		case TRANS_FRAME_ESCAPE:
		case TRANS_FRAME_END:
			if (kip->drvflags&0x100) {
				kip->drvflags &= ~0x100;
				c = c == TRANS_FRAME_ESCAPE ? FRAME_ESCAPE : FRAME_END;
			}
		default:
			if (kip->fill >= 0 && kip->fill < ETHERMTU)
				buf[kip->fill++] = c;
			else
				kip->fill = -1;
			break;
		case FRAME_END:
			len = kip->fill;
			kip->fill = 0;
			break;
		case FRAME_ESCAPE:
			kip->drvflags |= 0x100;
			break;
		}
	} while (!poll && len <= 0);
	return len ? len + 14 : 0;
}

static void
ipkdbsend(kip, buf, l)
	struct ipkdb_if *kip;
	u_char *buf;
	int l;
{
	buf += 14;
	l -= 14;
	while (--l >= 0) {
		switch (*buf) {
		default:
			kip->putc(kip, *buf++);
			break;
		case FRAME_ESCAPE:
		case FRAME_END:
			kip->putc(kip, FRAME_ESCAPE);
			kip->putc(kip, *buf == FRAME_END ? TRANS_FRAME_END : TRANS_FRAME_ESCAPE);
			buf++;
			break;
		}
	}
	kip->putc(kip, FRAME_END);
}

void
ipkdb_serial(kip)
	struct ipkdb_if *kip;
{
	struct cfdata *cf = kip->cfp;

	kip->myenetaddr[0] = 1;	/* make it a local address */
	kip->myenetaddr[1] = 0;
	kip->myenetaddr[2] = cf->cf_loc[1] >> 24;
	kip->myenetaddr[3] = cf->cf_loc[1] >> 16;
	kip->myenetaddr[4] = cf->cf_loc[1] >> 8;
	kip->myenetaddr[5] = cf->cf_loc[1];
	kip->flags |= IPKDB_MYHW;

	kip->mtu = 296;
	kip->receive = ipkdbrcv;
	kip->send = ipkdbsend;
}
