/*	$NetBSD: scsivar.h,v 1.1.1.1 1998/06/09 07:53:06 dbj Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
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

struct	scsi_softc {
    char sc_state;      /* controller state */
    short sc_result;	/* result of scsi command */
    /* saved interrupt state */
    u_char sc_status;     /* last 5390 status */
    u_char sc_seqstep;    /* last 5390 seqstep */
    u_char sc_intrstatus; /* last 5390 intrstatus */
    u_char sc_dmastatus;  /* mass-storage chip status */

    char *dma_addr;
    int dma_len;
};

/* scsi states */
#define SCSI_IDLE	0
#define SCSI_SELECTING	1
#define SCSI_HASBUS	2
#define SCSI_DMA	3
#define SCSI_CLEANUP	4
#define SCSI_DONE	5


/* scsi phase bits */
#define IOI	1
#define CDI	2
#define MSGI	4

/* information transfer phases */
#define DATA_OUT_PHASE	(0)
#define DATA_IN_PHASE	(IOI)
#define COMMAND_PHASE	(CDI)
#define STATUS_PHASE	(CDI|IOI)
#define MSG_OUT_PHASE	(MSGI|CDI)
#define MSG_IN_PHASE	(MSGI|CDI|IOI)

/* simple loop timeout */
#define SCSI_TIMEOUT 2000000

extern	struct scsi_softc scsi_softc;

#define DELAY(n)        { register int N = (n); while (--N > 0); }
