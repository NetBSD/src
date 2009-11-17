/*	$NetBSD: dl.c,v 1.7 2009/11/17 18:58:07 drochner Exp $	*/

/*
 * Copyright (c) 1993-95 Mats O Jansson.  All rights reserved.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: dl.c,v 1.7 2009/11/17 18:58:07 drochner Exp $");
#endif

#include "os.h"
#include "dl.h"
#include "get.h"
#include "mopdef.h"
#include "print.h"

void
mopDumpDL(FILE *fd, const u_char *pkt, int trans)
{
	int	i,idx = 0;
	u_int32_t tmpl;
	u_char	tmpc,c,program[257],code;
	const u_char *ucp;
	u_short	len,tmps,moplen;

	len = mopGetLength(pkt, trans);

	switch (trans) {
	case TRANS_8023:
		idx = 22;
		moplen = len - 8;
		break;
	default:
		idx = 16;
		moplen = len;
	}
	code = mopGetChar(pkt,&idx);
	
	switch (code) {
	case MOP_K_CODE_MLT:
		
		tmpc = mopGetChar(pkt,&idx);	/* Load Number */
		(void)fprintf(fd,"Load Number  :   %02x\n",tmpc);
		
		if (moplen > 6) {
			tmpl = mopGetLong(pkt,&idx);/* Load Address */
			(void)fprintf(fd,"Load Address : %08x\n", tmpl);
		}
		
		if (moplen > 10) {
#ifndef SHORT_PRINT
			for (i = 0; i < (moplen - 10); i++) {
				if ((i % 16) == 0) {
					if ((i / 16) == 0) {
					 	(void)fprintf(fd,
						       "Image Data   : %04x ",
							      moplen-10);
					} else {
						(void)fprintf(fd,
						       "                    ");
				        }
				}
				
				(void)fprintf(fd, "%02x ",
					      mopGetChar(pkt,&idx));
				if ((i % 16) == 15)
					(void)fprintf(fd,"\n");
			}
			
			if ((i % 16) != 15)
				(void)fprintf(fd,"\n");
#else
			idx = idx + moplen - 10;
#endif
		}
		
		tmpl = mopGetLong(pkt,&idx);	/* Load Address */
		(void)fprintf(fd,"Xfer Address : %08x\n", tmpl);
		
		break;
	case MOP_K_CODE_DCM:

		/* Empty Message */

		break;
	case MOP_K_CODE_MLD:
		
		tmpc = mopGetChar(pkt,&idx);	/* Load Number */
		(void)fprintf(fd,"Load Number  :   %02x\n",tmpc);
		
		tmpl = mopGetLong(pkt,&idx);	/* Load Address */
		(void)fprintf(fd,"Load Address : %08x\n", tmpl);
		
		if (moplen > 6) {
#ifndef SHORT_PRINT
			for (i = 0; i < (moplen - 6); i++) {
				if ((i % 16) == 0) {
					if ((i / 16) == 0) {
						(void)fprintf(fd,
						       "Image Data   : %04x ",
							      moplen-6);
					} else {
						(void)fprintf(fd,
						       "                    ");
					}
				}
				(void)fprintf(fd,"%02x ",
					      mopGetChar(pkt,&idx));
				if ((i % 16) == 15)
					(void)fprintf(fd,"\n");
			}

			if ((i % 16) != 15)
				(void)fprintf(fd,"\n");
#else
			idx = idx + moplen - 6;
#endif
		}
		
		break;
	case MOP_K_CODE_ASV:
		
		/* Empty Message */
		
		break;
	case MOP_K_CODE_RMD:

		tmpl = mopGetLong(pkt,&idx);	/* Memory Address */
		(void)fprintf(fd,"Mem Address  : %08x\n", tmpl);
		
		tmps = mopGetShort(pkt,&idx);	/* Count */
		(void)fprintf(fd,"Count        : %04x (%d)\n",tmps,tmps);
		
		break;
	case MOP_K_CODE_RPR:
		
		tmpc = mopGetChar(pkt,&idx);	/* Device Type */
		(void)fprintf(fd, "Device Type  :   %02x ",tmpc);
		mopPrintDevice(fd, tmpc); (void)fprintf(fd, "\n");
		
		tmpc = mopGetChar(pkt,&idx);	/* Format Version */
		(void)fprintf(fd,"Format       :   %02x\n",tmpc);
		
		tmpc = mopGetChar(pkt,&idx);	/* Program Type */
		(void)fprintf(fd,"Program Type :   %02x ",tmpc);
		mopPrintPGTY(fd, tmpc); (void)fprintf(fd, "\n");
		
		program[0] = 0;
		tmpc = mopGetChar(pkt,&idx);	/* Software ID Len */
		for (i = 0; i < tmpc; i++) {
			program[i] = mopGetChar(pkt,&idx);
			program[i+1] = '\0';
		}
		
		(void)fprintf(fd,"Software     :   %02x '%s'\n",tmpc,program);
		
		tmpc = mopGetChar(pkt,&idx);	/* Processor */
		(void)fprintf(fd,"Processor    :   %02x ",tmpc);
		mopPrintBPTY(fd, tmpc); (void)fprintf(fd, "\n");
		
		mopPrintInfo(fd, pkt, &idx, moplen, code, trans);
		
		break;
	case MOP_K_CODE_RML:
		
		tmpc = mopGetChar(pkt,&idx);	/* Load Number */
		(void)fprintf(fd,"Load Number  :   %02x\n",tmpc);
		
		tmpc = mopGetChar(pkt,&idx);	/* Error */
		(void)fprintf(fd,"Error        :   %02x (",tmpc);
		if ((tmpc == 0)) {
			(void)fprintf(fd,"no error)\n");
		} else {
		  	(void)fprintf(fd,"error)\n");
		}
		
		break;
	case MOP_K_CODE_RDS:
		
		tmpc = mopGetChar(pkt,&idx);	/* Device Type */
		(void)fprintf(fd, "Device Type  :   %02x ",tmpc);
		mopPrintDevice(fd, tmpc); (void)fprintf(fd, "\n");
		
		tmpc = mopGetChar(pkt,&idx);	/* Format Version */
		(void)fprintf(fd,"Format       :   %02x\n",tmpc);
		
		tmpl = mopGetLong(pkt,&idx);	/* Memory Size */
		(void)fprintf(fd,"Memory Size  : %08x\n", tmpl);
		
		tmpc = mopGetChar(pkt,&idx);	/* Bits */
		(void)fprintf(fd,"Bits         :   %02x\n",tmpc);
		
		mopPrintInfo(fd, pkt, &idx, moplen, code, trans);
		
		break;
	case MOP_K_CODE_MDD:
		
		tmpl = mopGetLong(pkt,&idx);	/* Memory Address */
		(void)fprintf(fd,"Mem Address  : %08x\n", tmpl);
		
		if (moplen > 5) {
#ifndef SHORT_PRINT
			for (i = 0; i < (moplen - 5); i++) {
				if ((i % 16) == 0) {
					if ((i / 16) == 0) {
						(void)fprintf(fd,
						       "Image Data   : %04x ",
							      moplen-5);
					} else {
						(void)fprintf(fd,
						       "                    ");
				        }
				}
				(void)fprintf(fd,"%02x ",
					      mopGetChar(pkt,&idx));
				if ((i % 16) == 15)
					(void)fprintf(fd,"\n");
			}
			if ((i % 16) != 15)
				(void)fprintf(fd,"\n");
#else
			idx = idx + moplen - 5;
#endif
		}
		
		break;
	case MOP_K_CODE_PLT:
		
		tmpc = mopGetChar(pkt,&idx);	/* Load Number */
		(void)fprintf(fd,"Load Number  :   %02x\n",tmpc);
		
		tmpc = mopGetChar(pkt,&idx);	/* Parameter Type */
		while (tmpc != MOP_K_PLTP_END) {
			c = mopGetChar(pkt,&idx);	/* Parameter Length */
			switch(tmpc) {
			case MOP_K_PLTP_TSN:		/* Target Name */
				(void)fprintf(fd,"Target Name  :   %02x '",
					      tmpc);
				for (i = 0; i < ((int) c); i++) {
					(void)fprintf(fd,"%c",
						    mopGetChar(pkt,&idx));
				}
				(void)fprintf(fd,"'\n");
				break;
			case MOP_K_PLTP_TSA:		/* Target Address */
				(void)fprintf(fd,"Target Addr  :   %02x ",c);
				for (i = 0; i < ((int) c); i++) {
					(void)fprintf(fd,"%02x ",
						    mopGetChar(pkt,&idx));
				}
				(void)fprintf(fd,"\n");
				break;
			case MOP_K_PLTP_HSN:		/* Host Name */
				(void)fprintf(fd,"Host Name    :   %02x '",
					      tmpc);
				for (i = 0; i < ((int) c); i++) {
					(void)fprintf(fd,"%c",
						    mopGetChar(pkt,&idx));
				}
				(void)fprintf(fd,"'\n");
				break;
			case MOP_K_PLTP_HSA:		/* Host Address */
				(void)fprintf(fd,"Host Addr    :   %02x ",c);
				for (i = 0; i < ((int) c); i++) {
					(void)fprintf(fd,"%02x ",
						    mopGetChar(pkt,&idx));
				}
				(void)fprintf(fd,"\n");
				break;
			case MOP_K_PLTP_HST:		/* Host Time */
				ucp = pkt + idx; idx = idx + 10;
				(void)fprintf(fd,"Host Time    : ");
				mopPrintTime(fd, ucp);
				(void)fprintf(fd,"\n");
				break;
			default:
				break;
			}
			tmpc = mopGetChar(pkt,&idx);/* Parameter Type */
		}
		
		tmpl = mopGetLong(pkt,&idx);	/* Transfer Address */
		(void)fprintf(fd,"Transfer Addr: %08x\n", tmpl);
		
		break;
	default:
		break;
	}
}


