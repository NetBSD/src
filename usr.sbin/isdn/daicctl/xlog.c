/*
 * Copyright (c) 1997 Martin Husemann <martin@duskware.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Last Edit-Date: [Sat Jan  6 12:31:20 2001]
 *
 * daicctl - maintenance tool and debug utility
 * xlog.c: output on-board log from active cards
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netisdn/i4b_ioctl.h>

#define LOG                     1

#define OK 			0xff
#define MORE_EVENTS 		0xfe
#define NO_EVENT 		1

#define OFF_LOG_RC	0	/* 1 byte */
#define	OFF_LOG_TIMEL1	1	/* 1 byte */
#define	OFF_LOG_TIMEL2	2	/* 1 byte */
#define	OFF_LOG_TIMES	3	/* 2 byte */
#define	OFF_LOG_TIMEH	5	/* 2 byte */
#define	OFF_LOG_CODE	7	/* 2 byte */
#define	OFF_PAR_TEXT	9	/* 42 bytes */
#define	OFF_PAR_L1_LEN	9	/* 2 byte */
#define	OFF_PAR_L1_I	11	/* 22 byte */
#define OFF_PAR_L2_CODE	9	/* 2 byte */
#define	OFF_PAR_L2_LEN	11	/* 2 byte */
#define	OFF_PAR_L2_I	13	/* 20 byte */

#define	DIAG_SIZE	49
#define	WORD(d,o)	(((d)[(o)])|((d)[(o)+1])<<8)
#define	DWORD(d,o)	(WORD(d,o)|(WORD(d,o+2)<<16))

void xlog(int fd, int controller);

static char *ll_name[10] = {
  "LL_UDATA",
  "LL_ESTABLISH",
  "LL_RELEASE",
  "LL_DATA",
  "LL_RESTART",
  "LL_RESET",
  "LL_POLL",
  "LL_TEST",
  "LL_MDATA",
  "LL_BUDATA"
};

static char *ns_name[11] = {
  "N_MDATA",
  "N_CONNECT",
  "N_CONNECT ACK",
  "N_DISC",
  "N_DISC ACK",
  "N_RESET",
  "N_RESET ACK",
  "N_DATA",
  "N_EDATA",
  "N_UDATA",
  "N_BDATA"
};

void
xlog(int fd, int controller)
{
	int i, n, code, fin;
	struct isdn_diagnostic_request req;
	u_int8_t data[DIAG_SIZE];
	u_int8_t rc;

	printf("xlog:\n");
	memset(&req, 0, sizeof(req));
	req.controller = controller;
	req.cmd = LOG;
	req.out_param_len = DIAG_SIZE;
	req.out_param = &data;

	for (fin = 0; !fin; ) {
		if (ioctl(fd, I4B_ACTIVE_DIAGNOSTIC, &req) == -1) {
			perror("ioctl(I4B_ACTIVE_DIAGNOSTIC)");
			fin = 1;
			break;
		}
		rc = data[OFF_LOG_RC];
		if (rc == NO_EVENT) {
			fin = 1;
			printf("No log event\n");
			break;
		}
		if (rc == MORE_EVENTS) {
			printf("More events...(0x%02x)\n", rc);
			fin = 0;
		} else if (rc == OK) {
			printf("Last event...(0x%02x)\n", rc);
			fin = 1;
		} else {
			printf("error: unknown rc = 0x%02x\n", rc);
			fin = 1;
			break;
		}

		/* print timestamp */
		printf("%5d:%04d:%03d - ", WORD(data,OFF_LOG_TIMEH), 
			WORD(data,OFF_LOG_TIMES),
			data[OFF_LOG_TIMEL2]*20 + data[OFF_LOG_TIMEL1]);

		code = data[OFF_LOG_CODE];
		switch (code) {
		  case 1:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("B-X(%03d) ",n);
			for(i=0; i<n && i<30; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 2:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("B-R(%03d) ", n);
			for(i=0; i<n && i<30; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 3:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("D-X(%03d) ",n);
			for(i=0; i<n && i<38; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 4:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("D-R(%03d) ",n);
			for(i=0; i<n && i<38; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 5:
			n = WORD(data, OFF_PAR_L2_LEN);
			printf("SIG-EVENT(%03d)%04X - ", n, WORD(data, OFF_PAR_L2_CODE));
			for(i=0; i<n && i<28; i++)
			    printf("%02X ", data[OFF_PAR_L2_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 6:
			code = WORD(data, OFF_PAR_L2_CODE);
			if(code && code <= 10)
				printf("%s IND",ll_name[code-1]);
			else
				printf("UNKNOWN LL IND");
			break;
		  case 7:
			code = WORD(data, OFF_PAR_L2_CODE);
			if(code && code <= 10)
				printf("%s REQ",ll_name[code-1]);
			else
				printf("UNKNOWN LL REQ");
			break;
		  case 8:
			n = WORD(data, OFF_PAR_L2_LEN);
			printf("DEBUG%04X - ",WORD(data, OFF_PAR_L2_CODE));
			for(i=0; i<n && i<38; i++)
			    printf("%02X ", data[OFF_PAR_L2_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 9:
			printf("MDL-ERROR(%s)",&data[OFF_PAR_TEXT]);
			break;
		  case 10:
			printf("UTASK->PC(%02X)",WORD(data, OFF_PAR_L2_CODE));
			break;
		  case 11:
			printf("PC->UTASK(%02X)",WORD(data, OFF_PAR_L2_CODE));
			break;
		  case 12:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("X-X(%03d) ",n);
			for(i=0; i<n && i<30; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 13:
			n = WORD(data, OFF_PAR_L1_LEN);
			printf("X-R(%03d) ",n);
			for(i=0; i<n && i<30; i++)
			    printf("%02X ", data[OFF_PAR_L1_I+i]);
			if(n>i) printf(" ...");
			break;
		  case 14:
			code = WORD(data, OFF_PAR_L2_CODE)-1;
			if((code &0x0f)<=10)
				printf("%s IND",ns_name[code &0x0f]);
			else
				printf("UNKNOWN NS IND");
			break;
		  case 15:
			code = WORD(data, OFF_PAR_L2_CODE)-1;
			if((code & 0x0f)<=10)
				printf("%s REQ",ns_name[code &0x0f]);
			else
				printf("UNKNOWN NS REQ");
			break;
		  case 16:
			printf("TASK %02i: %s",
				WORD(data, OFF_PAR_L2_CODE), &data[OFF_PAR_L2_I]);
			break;
		  case 18:
			code = WORD(data, OFF_PAR_L2_CODE);
			printf("IO-REQ %02x",code);
			break;
		  case 19:
			code = WORD(data, OFF_PAR_L2_CODE);
			printf("IO-CON %02x",code);
			break;
		  default:
		  	printf("unknown event code = %d\n", code);
		  	break;
		}
		printf("\n");
	}
	printf("\n");
}

