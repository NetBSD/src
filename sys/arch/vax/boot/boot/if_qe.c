/*	$NetBSD: if_qe.c,v 1.1 1999/03/06 16:36:05 ragge Exp $ */

/*
 * Copyright (c) 1998 Roar Thronæs.  All rights reserved.
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
 *	This product includes software developed by Roar Thronæs.
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
 *
 *	Standalone routine for the DEQNA.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/netif.h>

#include <if/if_qereg.h>

int qe_probe(), qe_match(), qe_get(), qe_put();
void qe_init(), qe_end();

struct netif_stats qe_stats;

struct netif_dif qe_ifs[] = {
/*	dif_unit	dif_nsel	dif_stats	dif_private	*/
{	0,		1,		&qe_stats,	},
};

struct netif_stats qe_stats;

struct netif_driver qe_driver = {
	"qe", qe_match, qe_probe, qe_init, qe_get, qe_put, qe_end, qe_ifs, 1,
};

#define PG_V            0x80000000
#define QBAMAP          0x20088000

#define NRCV    1                      /* Receive descriptors          */
#define NXMT    1                       /* Transmit descriptors         */
#define NTOT    (NXMT + NRCV)

#define QE_INTS        (QE_RCV_INT | QE_XMIT_INT)
#define MAXPACKETSIZE  0x800            /* Because of (buggy) DEQNA */

struct  qe_softc {
        struct  qe_ring rring[NRCV+2];  /* Receive ring descriptors     */
        struct  qe_ring tring[NXMT+2];  /* Xmit ring descriptors        */
        u_char  setup_pkt[16][8];       /* Setup packet                 */
	char	qein[2048], qeout[2048];/* Packet buffers		*/
};

static	volatile struct qe_softc *sc;
static	volatile struct qedevice *addr;

int
qe_match(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{
	return strcmp(machdep_hint, "qe") == 0;
}

int
qe_probe(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{
	return 0;
}

void
qe_init(desc, machdep_hint)
	struct iodesc *desc;
	void *machdep_hint;
{

	int i,j;
	u_int *qm=(u_int *) QBAMAP;

	sc = (void *)alloc(sizeof(struct qe_softc));

	bzero(sc,sizeof(struct qe_softc)); 

	for(i = 0; i < 8192; i++)
		qm[i] = PG_V | i;

	/* XXX hardcoded addr */
	addr = (struct qedevice *)(0x20000000 + (0774440 & 017777));

	addr->qe_csr = QE_RESET;
	addr->qe_csr &= ~QE_RESET;
	for(i=0x10000; i; i--) ;

        for (i = 0; i < 6; i++) {
                sc->setup_pkt[i][1] = addr->qe_sta_addr[i];
                sc->setup_pkt[i+8][1] = addr->qe_sta_addr[i];
		sc->setup_pkt[i][2] = 0xff;
                sc->setup_pkt[i+8][2] = addr->qe_sta_addr[i];
		for (j=3; j < 8; j++) {
               		sc->setup_pkt[i][j] = addr->qe_sta_addr[i];
                	sc->setup_pkt[i+8][j] = addr->qe_sta_addr[i];
		}
		desc->myea[i] = addr->qe_sta_addr[i];
	}

	bzero((caddr_t)sc->rring, sizeof(struct qe_ring));
        sc->rring->qe_buf_len = -64;
        sc->rring->qe_addr_lo = (short)((int)sc->setup_pkt);
        sc->rring->qe_addr_hi = (short)((int)sc->setup_pkt >> 16);

	bzero((caddr_t)sc->tring, sizeof(struct qe_ring));
        sc->tring->qe_buf_len = -64;
        sc->tring->qe_addr_lo = (short)((int)sc->setup_pkt);
        sc->tring->qe_addr_hi = (short)((int)sc->setup_pkt >> 16);

        sc->rring[0].qe_flag = sc->rring[0].qe_status1 = QE_NOTYET;
        sc->rring[0].qe_valid = 1;

        sc->tring[0].qe_setup = 1;
        sc->tring[0].qe_eomsg = 1;
        sc->tring[0].qe_flag = sc->tring[0].qe_status1 = QE_NOTYET;
        sc->tring[0].qe_valid = 1;

	addr->qe_csr =  QE_XMIT_INT | QE_RCV_INT;

        addr->qe_rcvlist_lo = (short)((int)sc->rring & 0xffff);
        addr->qe_rcvlist_hi = (short)((int)sc->rring >> 16);

        addr->qe_xmtlist_lo = (short)((int)sc->tring & 0xffff);
        addr->qe_xmtlist_hi = (short)((int)sc->tring >> 16);

	while ((addr->qe_csr & QE_INTS) != QE_INTS)
		;
	addr->qe_csr |= QE_INTS;

	addr->qe_csr &= ~(QE_INT_ENABLE|QE_ELOOP);
	addr->qe_csr |= QE_ILOOP;

        sc->rring[0].qe_addr_lo = (short)((int)sc->qein & 0xffff);
        sc->rring[0].qe_addr_hi = (short)((int)sc->qein >> 16);
	sc->rring[0].qe_setup=0;
	sc->rring[0].qe_buf_len=-MAXPACKETSIZE/2;
	sc->rring[0].qe_valid=1;
	sc->rring[0].qe_flag=sc->rring[0].qe_status1=QE_NOTYET;
	sc->rring[0].qe_status2=1;

	sc->rring[1].qe_valid=0;
	sc->rring[1].qe_addr_lo = 0;
	sc->rring[1].qe_addr_hi = 0;
	sc->rring[1].qe_flag=sc->rring[1].qe_status1=QE_NOTYET;
	sc->rring[1].qe_status2=1;

        sc->tring[0].qe_addr_lo = (short)((int)sc->qeout & 0xffff);
        sc->tring[0].qe_addr_hi = (short)((int)sc->qeout >> 16);
	sc->tring[0].qe_setup=0;
	sc->tring[0].qe_buf_len=0;
	sc->tring[0].qe_eomsg=1;
	sc->tring[0].qe_flag=sc->tring[0].qe_status1=QE_NOTYET;
	sc->tring[0].qe_valid=1;

	sc->tring[1].qe_flag=sc->tring[1].qe_status1=QE_NOTYET;
	sc->tring[1].qe_valid=0;
	sc->tring[1].qe_addr_lo = 0;
	sc->tring[1].qe_addr_hi = 0;

        addr->qe_csr|=QE_RCV_ENABLE;
        addr->qe_rcvlist_lo = (short)((int)sc->rring & 0xffff);
        addr->qe_rcvlist_hi = (short)((int)sc->rring >> 16);
}

int
qe_get(desc, pkt, maxlen, timeout)
	struct iodesc *desc;
	void *pkt;
	int maxlen;
	time_t timeout;
{
	int len, j;

retry:
	for(j = 0x10000;j && (addr->qe_csr & QE_RCV_INT) == 0; j--)
		;

	if ((addr->qe_csr & QE_RCV_INT) == 0)
		goto fail;

	addr->qe_csr &= ~(QE_RCV_ENABLE|QE_XMIT_INT);

	len= ((sc->rring[0].qe_status1 & QE_RBL_HI) |
	    (sc->rring[0].qe_status2 & QE_RBL_LO)) + 60;

	if (sc->rring[0].qe_status1 & 0xc000)
		goto fail;

        if (len == 0)
		goto retry;

	bcopy((void*)sc->qein,pkt,len);


end:
	sc->rring[0].qe_status2 = sc->rring[1].qe_status2 = 1;
	sc->rring[0].qe_flag=sc->rring[0].qe_status1=QE_NOTYET;
	sc->rring[1].qe_flag=sc->rring[1].qe_status1=QE_NOTYET;
        addr->qe_csr |= QE_RCV_ENABLE;
        addr->qe_rcvlist_lo = (short)((int)sc->rring);
        addr->qe_rcvlist_hi = (short)((int)sc->rring >> 16);
	return len;

fail:	len = -1;
	goto end;
}

int
qe_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	int len;
{
	int j;

	bcopy(pkt,sc->qeout,len);
        sc->tring[0].qe_buf_len=-len/2;
        sc->tring[0].qe_flag=sc->tring[0].qe_status1=QE_NOTYET;
        sc->tring[1].qe_flag=sc->tring[1].qe_status1=QE_NOTYET;

        addr->qe_xmtlist_lo = (short)((int)sc->tring & 0xffff);
        addr->qe_xmtlist_hi = (short)((int)sc->tring >> 16);

	for(j = 0; (j < 0x10000) && ((addr->qe_csr & QE_XMIT_INT) == 0); j++)
		;

	if ((addr->qe_csr & QE_XMIT_INT) == 0) {
		qe_init(desc,0);
		return -1;
	}
	addr->qe_csr &= ~QE_RCV_INT;

	if (sc->tring[0].qe_status1 & 0xc000) {
		qe_init(desc,0);
		return -1;
	}
	return len;
}

void
qe_end(nif)
     struct netif *nif;
{
	int i;

	addr->qe_csr = QE_RESET;
	addr->qe_csr &= ~QE_RESET;
	for(i=0x10000; i; i--) ;
}
