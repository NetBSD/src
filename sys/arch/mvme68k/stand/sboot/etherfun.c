/*	$NetBSD: etherfun.c,v 1.8.50.1 2008/01/19 12:14:37 bouyer Exp $	*/

/*
 *
 * Copyright (c) 1995 Charles D. Cranor and Seth Widoff
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
 *      This product includes software developed by Charles D. Cranor
 *	and Seth Widoff.
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
/* etherfun.c */

#include "sboot.h"
#include "etherfun.h"

/* Construct and send a rev arp packet */
void
do_rev_arp(void) 
{
	int i;
  
	for (i = 0; i < 6; i++) {
		eh->ether_dhost[i] = 0xff;
	}
	memcpy(eh->ether_shost, myea, 6);
	eh->ether_type = ETYPE_RARP;
  
	rarp->ar_hrd = 1;	/* hardware type is 1 */
	rarp->ar_pro = PTYPE_IP;
	rarp->ar_hln = 6;	/* length of hardware address is 6 bytes */
	rarp->ar_pln = 4;	/* length of ip address is 4 byte */
	rarp->ar_op = OPCODE_RARP;
	memcpy(rarp->arp_sha, myea, sizeof(myea));
	memcpy(rarp->arp_tha, myea, sizeof(myea));
	for (i = 0; i < 4; i++) {
		rarp->arp_spa[i] = rarp->arp_tpa[i] = 0x00;
	}

	le_put(buf, 76);
}
  
/* Receive and disassemble the rev_arp reply */

int
get_rev_arp(void) 
{

	le_get(buf, sizeof(buf), 6);
	if (eh->ether_type == ETYPE_RARP && rarp->ar_op == OPCODE_REPLY) {
		memcpy(myip, rarp->arp_tpa, sizeof(rarp->arp_tpa));
		memcpy(servip, rarp->arp_spa, sizeof(rarp->arp_spa));
		memcpy(servea, rarp->arp_sha, sizeof(rarp->arp_sha));
		return 1;
	}
	return 0;
}

/* Try to get a reply to a rev arp request */

int 
rev_arp(void) 
{
	int tries = 0;

	while (tries < 5) {
		do_rev_arp();
		if (get_rev_arp()) {
			return 1;
		}
		tries++;
	}
	return 0;
} 

/* Send a tftp read request or acknowledgement 
   mesgtype 0 is a read request, 1 is an aknowledgement */

void 
do_send_tftp(int mesgtype) 
{
	u_long res, iptmp, lcv;
	u_char *tot;

	if (mesgtype == 0) {
		tot = tftp_r + (sizeof(MSG) - 1);
		myport = (u_short)time();
		if (myport < 1000)
			myport += 1000;
		servport = FTP_PORT; /* to start */
	} else {
		tot = (u_char *)tftp_a + 4;
	}

	memcpy (eh->ether_dhost, servea, sizeof(servea));
	memcpy (eh->ether_shost, myea, sizeof(myea));
	eh->ether_type = ETYPE_IP;

	iph->ip_v = IP_VERSION;
	iph->ip_hl = IP_HLEN;   
	iph->ip_tos = 0;	/* type of service is 0 */
	iph->ip_id = 0;		/* id field is 0 */
	iph->ip_off = IP_DF;
	iph->ip_ttl = 3;	/* time to live is 3 seconds/hops */
	iph->ip_p = IPP_UDP;
	memcpy(iph->ip_src, myip, sizeof(myip));
	memcpy(iph->ip_dst, servip, sizeof(servip));
	iph->ip_sum = 0;
	iph->ip_len = tot - (u_char *)iph;
	res = oc_cksum(iph, sizeof(struct ip), 0);
	iph->ip_sum = 0xffff & ~res;
	udph->uh_sport = myport;
	udph->uh_dport = servport;
	udph->uh_sum = 0;

	if (mesgtype)  {
		tftp_a->op_code = FTPOP_ACKN;
		tftp_a->block = (u_short)(mesgtype);
	} else {
		memcpy (&iptmp, myip, sizeof(iptmp));
		memcpy(tftp_r, MSG, (sizeof(MSG)-1));
		for (lcv = 9; lcv >= 2; lcv--) {
			tftp_r[lcv] = HEXDIGITS[iptmp & 0xF];

			iptmp = iptmp >> 4;
		}
	}

	udph->uh_ulen = tot - (u_char *)udph;

	le_put(buf, tot - buf);
}

/* Attempt to tftp a file and read it into memory */

int
do_get_file(void) 
{
	int fail = 0, oldlen;
	char *loadat = (char *)LOAD_ADDR;
	last_ack = 0;

	do_send_tftp(READ);
	for (;;) {
		if (le_get(buf, sizeof(buf), 5) == 0) { /* timeout occurred */
			if (last_ack) {                          
				do_send_tftp(last_ack);
			} else {
				do_send_tftp(READ);
			}
			fail++;
			if (fail > 5) {
			        printf("\n");
				return 1;
			}
		} else {
			printf("%x \r", tftp->info.block*512);
			if ((eh->ether_type != ETYPE_IP) ||
			    (iph->ip_p != IPP_UDP)) {
				fail++;
				continue;
			}
			if (servport == FTP_PORT) servport = udph->uh_sport;
			if (tftp->info.op_code == FTPOP_ERR) {
				printf("TFTP: Download error %d: %s\n", 
				    tftp->info.block, tftp->data);
				return 1;
			}
			if (tftp->info.block != last_ack + 1) {
				/* we received the wrong block */
				if (tftp->info.block < last_ack +1) {
					/* ackn whatever we received */
					do_send_tftp(tftp->info.block);
				} else {
					/* ackn the last confirmed block */
					do_send_tftp(last_ack);
				}
				fail++;
			} else {		/* we got the right block */
				fail = 0;
				last_ack++;
				oldlen = udph->uh_ulen;
				do_send_tftp( last_ack );
#if 0
				printf("memcpy %x %x %d\n", loadat,
				    &tftp->data, oldlen - 12);
#endif
				memcpy(loadat, &tftp->data, oldlen - 12);
				loadat += oldlen - 12;
				if (oldlen < (8 + 4 + 512)) {
					printf("\n");
					return 0;
				}
			}
		}
	}
	printf("\n");
	return 0;
} 
