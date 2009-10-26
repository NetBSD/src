/*      $NetBSD: en.c,v 1.17 2009/10/26 19:16:57 cegger Exp $        */
/*
 * Copyright (c) 1996 Rolf Grossmann
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

#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <next68k/dev/enreg.h>
#include <next68k/next68k/nextrom.h>
#include "enreg.h"
#include "dmareg.h"

#include <stand.h>
#include <netif.h>
#include <net.h>
#include <nfs.h>

#include <lib/libkern/libkern.h>

extern char *mg;
#define	MON(type, off) (*(type *)((u_int) (mg) + off))

#define PRINTF(x) printf x;
#ifdef EN_DEBUG
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

#define	EN_TIMEOUT	2000000
#define EN_RETRIES	10

int en_match(struct netif *, void *);
int en_probe(struct netif *, void *);
void en_init(struct iodesc *, void *);
int en_get(struct iodesc *, void *, size_t, saseconds_t);
int en_put(struct iodesc *, void *, size_t);
void en_end(struct netif *);

static int en_wait_for_intr(int);

struct netif_stats en_stats;

struct netif_dif en_ifs[] = {
/* dif_unit	dif_nsel	dif_stats	dif_private     */
{  0,		1,		&en_stats,	NULL,	},
};

struct netif_driver en_driver = {
	"en",
	en_match, en_probe, en_init, en_get, en_put, en_end,
	en_ifs, sizeof(en_ifs) / sizeof(en_ifs[0])
};

extern int turbo;

/* ### int netdev_sock;
static int open_count; */

char dma_buffer1[MAX_DMASIZE+DMA_ENDALIGNMENT],
     dma_buffer2[MAX_DMASIZE+DMA_ENDALIGNMENT],
     *dma_buffers[2];
int next_dma_buffer;


int
en_match(struct netif *nif, void *machdep_hint)
{
	/* we always match, because every NeXT has an ethernet interface
	 * and we've checked the unit numbers before we even started this
	 * search.
	 * ### now that open is generic, we should check the params!
	 */
	return 1;
}

int
en_probe(struct netif *nif, void *machdep_hint)
{
	/* we also always probe ok, see en_match. */
	return 0;
}

void
en_init(struct iodesc *desc, void *machdep_hint)
{
	volatile struct en_regs *er;
	volatile u_int *bmap_chip;
	int i;

	DPRINTF(("en_init\n"));

	er = (struct en_regs *)P_ENET;
	bmap_chip = (u_int *)P_BMAP;

	dma_buffers[0] = DMA_ALIGN(char *, dma_buffer1);
	dma_buffers[1] = DMA_ALIGN(char *, dma_buffer2);

	er->reset = EN_RST_RESET;
/* 	if (turbo) */
/* 		er->reset = 0; */

	er->txmask = 0;
	er->txstat = 0xff;
	if (turbo)
		er->txmode = 0 | EN_TMD_COLLSHIFT;
	else
		er->txmode = EN_TMD_LB_DISABLE;

	/* setup for bnc/tp */
	if (!turbo) {
		DPRINTF (("en_media: %s\n",
			  (bmap_chip[13] & 0x20000000) ? "BNC" : "TP"));
		if (!(bmap_chip[13] & 0x20000000)) {
			bmap_chip[12] |= 0x90000000;
			bmap_chip[13] |= (0x80000000|0x10000000); /* TP */
		}
	}

/* 	if (turbo) { */
/* 		er->txmode |= EN_TMD_COLLSHIFT; */
/* 	} else { */
/* 		er->txmode &= ~EN_TMD_LB_DISABLE; /\* ZZZ *\/ */
/* 	} */

	er->rxmask = 0;
	er->rxstat = 0xff;
	if (turbo)
		er->rxmode = EN_RMD_TEST | EN_RMD_RECV_NORMAL;
	else
		er->rxmode = EN_RMD_RECV_NORMAL;
	for (i=0; i<6; i++)
	  er->addr[i] = desc->myea[i] = MON(char *,MG_clientetheraddr)[i];

	DPRINTF(("ethernet addr (%x:%x:%x:%x:%x:%x)\n",
			desc->myea[0],desc->myea[1],desc->myea[2],
			desc->myea[3],desc->myea[4],desc->myea[5]));

/* 	if (!turbo) */
		er->reset = 0;
}

#if 0
/* ### remove this when things work! */
#define XCHR(x) hexdigits[(x) & 0xf]
void
dump_pkt(unsigned char *pkt, size_t len)
{
	size_t i, j;

	printf("0000: ");
	for(i=0; i<len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i+1) % 16 == 0) {
			printf("  %c", '"');
			for(j=0; j<16; j++)
				printf("%c", pkt[i-15+j]>=32 && pkt[i-15+j]<127?pkt[i-15+j]:'.');
			printf("%c\n%c%c%c%c: ", '"', XCHR((i+1)>>12),
				XCHR((i+1)>>8), XCHR((i+1)>>4), XCHR(i+1));
		}
	}
	printf("\n");
}
#endif

int
en_put(struct iodesc *desc, void *pkt, size_t len)
{
	volatile struct en_regs *er;
	volatile struct dma_dev *txdma;
	int state, txs;
	int retries;

	DPRINTF(("en_put: %d bytes at 0x%lx\n", len, (unsigned long)pkt));
#if 0
	dump_pkt(pkt,len);
#endif

	er = (struct en_regs *)P_ENET;
	txdma = (struct dma_dev *)P_ENETX_CSR;

	DPRINTF(("en_put: txdma->dd_csr = %x\n",txdma->dd_csr));

	if (len > 1600) {
		errno = EINVAL;
		return -1;
	}

	if (!turbo) {
		while ((er->txstat & EN_TXS_READY) == 0)
			printf("en: tx not ready\n");
	}

	for (retries = 0; retries < EN_RETRIES; retries++) {
		er->txstat = 0xff;
		memcpy(dma_buffers[0], pkt, len);
		txdma->dd_csr = (turbo ? DMACSR_INITBUFTURBO : DMACSR_INITBUF) |
			DMACSR_RESET | DMACSR_WRITE;
		txdma->dd_csr = 0;
		txdma->dd_next/* _initbuf */ = dma_buffers[0];
		txdma->dd_start = (turbo ? dma_buffers[0] : 0);
		txdma->dd_limit = ENDMA_ENDALIGN(char *,  dma_buffers[0]+len);
		txdma->dd_stop = 0;
		txdma->dd_csr = DMACSR_SETENABLE;
		if (turbo)
			er->txmode |= 0x80;

		while (1) {
			if (en_wait_for_intr(ENETX_DMA_INTR)) {
				printf("en_put: timed out\n");
				errno = EIO;
				return -1;
			}

			state = txdma->dd_csr &
				(DMACSR_BUSEXC | DMACSR_COMPLETE
				 | DMACSR_SUPDATE | DMACSR_ENABLE);

#if 01
			DPRINTF(("en_put: DMA state = 0x%x.\n", state));
#endif
			if (state & (DMACSR_COMPLETE|DMACSR_BUSEXC))
				txdma->dd_csr = DMACSR_RESET | DMACSR_CLRCOMPLETE;
				break;
		}

		txs = er->txstat;

#if 01
		DPRINTF(("en_put: done txstat=%x.\n", txs));
#endif

#define EN_TXS_ERROR (EN_TXS_SHORTED | EN_TXS_UNDERFLOW | EN_TXS_PARERR)
		if ((state & DMACSR_COMPLETE) == 0 ||
		    (txs & EN_TXS_ERROR) != 0) {
			errno = EIO;
			return -1;
		}
		if ((txs & EN_TXS_COLLERR) == 0)
			return len;		/* success */
	}

	errno = EIO;		/* too many retries */
	return -1;
}

int
en_get(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
	volatile struct en_regs *er;
	volatile struct dma_dev *rxdma;
	volatile struct dma_dev *txdma;
	int state, rxs;
	size_t rlen;
	char *gotpkt;

	rxdma = (struct dma_dev *)P_ENETR_CSR;
	txdma = (struct dma_dev *)P_ENETX_CSR;
	er = (struct en_regs *)P_ENET;

	DPRINTF(("en_get: rxdma->dd_csr = %x\n",rxdma->dd_csr));

	er->rxstat = 0xff;

	/* this is mouse's code now ... still doesn't work :( */
	/* The previous comment is now a lie, this does work
	 * Darrin B Jewell <jewell@mit.edu>  Sat Jan 24 21:44:56 1998
	 */

	rxdma->dd_csr = 0;
	rxdma->dd_csr = (turbo ? DMACSR_INITBUFTURBO : DMACSR_INITBUF) |
		DMACSR_READ | DMACSR_RESET;

	if (!turbo) {
		rxdma->dd_saved_next = 0;
		rxdma->dd_saved_limit = 0;
		rxdma->dd_saved_start = 0;
		rxdma->dd_saved_stop = 0;
	} else {
		rxdma->dd_saved_next = dma_buffers[0];
	}

	rxdma->dd_next = dma_buffers[0];
	rxdma->dd_limit = DMA_ENDALIGN(char *, dma_buffers[0]+MAX_DMASIZE);
#if 0
	if (turbo) {
		/* !!! not a typo: txdma */
		txdma->dd_stop = dma_buffers[0];
	}
#endif
	rxdma->dd_start = 0;
	rxdma->dd_stop = 0;
	rxdma->dd_csr = DMACSR_SETENABLE | DMACSR_READ;
	if (turbo)
		er->rxmode = EN_RMD_TEST | EN_RMD_RECV_NORMAL;
	else
		er->rxmode = EN_RMD_RECV_NORMAL;

#if 01
	DPRINTF(("en_get: blocking on rcv DMA\n"));
#endif

	while (1) {
		if (en_wait_for_intr(ENETR_DMA_INTR))	/* ### use timeout? */
			return 0;

		state = rxdma->dd_csr &
			(DMACSR_BUSEXC | DMACSR_COMPLETE
			 | DMACSR_SUPDATE | DMACSR_ENABLE);
		DPRINTF(("en_get: DMA state = 0x%x.\n", state));
		if ((state & DMACSR_ENABLE) == 0) {
			rxdma->dd_csr = DMACSR_RESET | DMACSR_CLRCOMPLETE;
			break;
		}

		if (state & DMACSR_COMPLETE) {
			PRINTF(("en_get: ending DMA sequence\n"));
			rxdma->dd_csr = DMACSR_CLRCOMPLETE;
		}
	}

	rxs = er->rxstat;

	if ((state & DMACSR_COMPLETE) == 0 ||
	    (rxs & EN_RX_OK) == 0) {
		errno = EIO;
		return -1;	/* receive failed */
	}

	if (turbo) {
		gotpkt = rxdma->dd_saved_next;
		rlen = rxdma->dd_next - rxdma->dd_saved_next;
	} else {
		gotpkt = rxdma->dd_saved_next;
		rlen = rxdma->dd_next - rxdma->dd_saved_next;
	}

	if (gotpkt != dma_buffers[0]) {
		printf("Unexpected received packet location\n");
		printf("DEBUG: rxstat=%x.\n", rxs);
		printf("DEBUG: gotpkt = 0x%lx, rlen = %d, len = %d\n",(u_long)gotpkt,rlen,len);
		printf("DEBUG: rxdma->dd_csr = 0x%lx\n",(u_long)rxdma->dd_csr);
		printf("DEBUG: rxdma->dd_saved_next = 0x%lx\n",(u_long)rxdma->dd_saved_next);
		printf("DEBUG: rxdma->dd_saved_limit = 0x%lx\n",(u_long)rxdma->dd_saved_limit);
		printf("DEBUG: rxdma->dd_saved_start = 0x%lx\n",(u_long)rxdma->dd_saved_start);
		printf("DEBUG: rxdma->dd_saved_stop = 0x%lx\n",(u_long)rxdma->dd_saved_stop);
		printf("DEBUG: rxdma->dd_next = 0x%lx\n",(u_long)rxdma->dd_next);
		printf("DEBUG: rxdma->dd_limit = 0x%lx\n",(u_long)rxdma->dd_limit);
		printf("DEBUG: rxdma->dd_start = 0x%lx\n",(u_long)rxdma->dd_start);
		printf("DEBUG: rxdma->dd_stop = 0x%lx\n",(u_long)rxdma->dd_stop);
		printf("DEBUG: rxdma->dd_next_initbuf = 0x%lx\n",(u_long)rxdma->dd_next_initbuf);
	}

#if 0
dump_pkt(gotpkt, rlen < 255 ? rlen : 128);
#endif

	DPRINTF(("en_get: done rxstat=%x.\n", rxs));

	if (rlen > len) {
		DPRINTF(("en_get: buffer too small. want %d, got %d\n",
			 len, rlen));
		rlen = len;
	}

	memcpy(pkt, gotpkt, rlen);

#if 0
	printf("DEBUG: gotpkt = 0x%lx, pkt = 0x%lx, rlen = %d\n",
			(u_long)gotpkt,(u_long)pkt,rlen);
	dump_pkt(gotpkt, rlen < 255 ? rlen : 128);
	dump_pkt(pkt, rlen < 255 ? rlen : 128);
#endif

	return rlen;
}


void
en_end(struct netif *a)
{
	DPRINTF(("en_end: WARNING not doing anything\n"));
}

#if 0

#define MKPANIC(ret,name,args) \
	ret name args { panic(#name ## ": not implemented.\n"); }

MKPANIC(void, en_end, (struct netif *a));

/* device functions */

int
enopen(struct open_file *f, char count, char lun, char part)
{
	int error;

	DPRINTF(("open: en(%d,%d,%d)\n", count, lun, part));

	if (count != 0 || lun != 0 || part != 0)
		return EUNIT;	/* there can be exactly one ethernet */

	if (open_count == 0) {
		/* Find network interface. */
		if ((netdev_sock = netif_open(NULL)) < 0)
			return errno;
		if ((error = mountroot(netdev_sock)) != 0) {
			if (open_count == 0)
				netif_close(netdev_sock);
			return error;
		}
	}
	open_count++;
	f->f_devdata = NULL; /* ### nfs_root_node ?! */
	return 0;
}

int
enclose(struct open_file *f)
{
	if (open_count > 0)
		if (--open_count == 0)
			netif_close(netdev_sock);
	f->f_devdata = NULL;
	return 0;
}

int
enstrategy(void *devdata, int rw, daddr_t dblk,
	   size_t size, void *buf, size_t *rsize)
{
	return ENXIO;		/* wrong access method */
}

/* private function */

static int
mountroot(int sock)
{
	/* Mount the root directory from a boot server */
#if 0
	struct in_addr in = {
		0xc2793418
	};
	u_char *res;

	res = arpwhohas(socktodesc(sock), in);
	panic("arpwhohas returned %s", res);
#endif
	/* 1. use bootp. This does most of the work for us. */
	bootp(sock);

	if (myip.s_addr == 0 || rootip.s_addr == 0 || rootpath[0] == '\0')
		return ETIMEDOUT;

	printf("Using IP address: %s\n", inet_ntoa(myip));
	printf("root addr=%s path=%s\n", inet_ntoa(rootip), rootpath);

	/* 2. mount. */
	if (nfs_mount(sock, rootip, rootpath) < 0)
		return errno;

	return 0;
}
#endif

static int
en_wait_for_intr(int flag)
{
	volatile int *intrstat = MON(volatile int *, MG_intrstat);

	int count;

	for (count = 0; count < EN_TIMEOUT; count++)
		if (*intrstat & flag)
			return 0;

	return -1;
}
