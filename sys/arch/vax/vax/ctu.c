/*	$NetBSD: ctu.c,v 1.15 2002/07/01 16:20:35 ragge Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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

/*
 * Device driver for 11/750 Console TU58.
 *
 * Writing of tapes does not work, by some unknown reason so far.
 * It is almost useless to try to use this driver when running
 * multiuser, because the serial device don't have any buffers 
 * so we will loose interrupts.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/conf.h>

#include <machine/mtpr.h>
#include <machine/rsp.h>
#include <machine/scb.h>
#include <machine/trap.h>

#undef TUDEBUG

#define	TU_IDLE		0
#define	TU_RESET	1
#define	TU_RUNNING	2
#define	TU_WORKING	3
#define TU_READING	4
#define TU_WRITING	5
#define	TU_ENDPACKET	6
#define	TU_RESTART	7

struct tu_softc {
	int	sc_state;
	int	sc_step;
	char	sc_rsp[15];	/* Should be struct rsb; but don't work */
	int	sc_tpblk;	/* Start block number */
	int 	sc_wto;		/* Timeout counter */
	int	sc_xbytes;	/* Number of xfer'd bytes */
	int	sc_op;		/* Read/write */
	struct	buf_queue sc_bufq;	/* pending I/O requests */
} tu_sc;

struct	ivec_dsp tu_recv, tu_xmit;

	void ctuattach(void);
static	void ctutintr(void *);
static	void cturintr(void *);
static	void ctustart(void);
static	void ctuwatch(void *);
static	u_short ctu_cksum(unsigned short *, int);

bdev_decl(ctu);

static struct callout ctu_watch_ch = CALLOUT_INITIALIZER;

void
ctuattach()
{
	BUFQ_INIT(&tu_sc.sc_bufq);

	tu_recv = idsptch;
	tu_recv.hoppaddr = cturintr;
	scb->scb_csrint = (void *)&tu_recv;

	tu_xmit = idsptch;
	tu_xmit.hoppaddr = ctutintr;
	scb->scb_cstint = (void *)&tu_xmit;
}

static void
ctuinit(void)
{
	int s = spl7();
#define	WAIT	while ((mfpr(PR_CSTS) & 0x80) == 0)

	/*
	 * Do a reset as described in the
	 * "TU58 DECtape II Users Guide".
	 */
	mtpr(0101, PR_CSTS);	/* Enable transmit interrupt + send break */
	WAIT;
	mtpr(0, PR_CSTD); WAIT;
	mtpr(0, PR_CSTD); WAIT;
	mtpr(RSP_TYP_INIT, PR_CSTD); WAIT;
	mtpr(RSP_TYP_INIT, PR_CSTD); WAIT;
#undef	WAIT
	splx(s);
}

int
ctuopen(dev_t dev, int oflags, int devtype, struct proc *p)
{
	int error;

	if (minor(dev))
		return ENXIO;

	if (tu_sc.sc_state != TU_IDLE)
		return EBUSY;

	tu_sc.sc_state = TU_RESET;
	tu_sc.sc_step = 0;
	mtpr(0100, PR_CSRS);	/* Enable receive interrupt */
	mtpr(0101, PR_CSTS);	/* Enable transmit interrupt + send break */
	if ((error = tsleep((caddr_t)&tu_sc, (PZERO + 10)|PCATCH, "reset", 0)))
		return error;
	
#ifdef TUDEBUG
	printf("ctuopen: running\n");
#endif
	tu_sc.sc_state = TU_RUNNING;
	callout_reset(&ctu_watch_ch, hz, ctuwatch, NULL);
	return 0;

}

int
ctuclose(dev_t dev, int oflags, int devtype, struct proc *p)
{
	struct buf *bp;
	int s = spl7();
	while ((bp = BUFQ_FIRST(&tu_sc.sc_bufq)))
		BUFQ_REMOVE(&tu_sc.sc_bufq, bp);
	splx(s);

	mtpr(0, PR_CSRS);
	mtpr(0, PR_CSTS);
	tu_sc.sc_state = TU_IDLE;
	callout_stop(&ctu_watch_ch);
	return 0;
}

void
ctustrategy(struct buf *bp)
{
	int s, empty;

#ifdef TUDEBUG
	printf("ctustrategy: bcount %ld blkno %d\n", bp->b_bcount, bp->b_blkno);
	printf("ctustrategy: bp %p\n", bp);
#endif
	s = spl7();
	if (bp->b_blkno >= 512) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		splx(s);
		return;
	}

	empty = TAILQ_EMPTY(&tu_sc.sc_bufq.bq_head);
	BUFQ_INSERT_TAIL(&tu_sc.sc_bufq, bp);
	if (empty)
		ctustart();
	splx(s);
}

void
ctustart()
{
	struct rsp *rsp = (struct rsp *)tu_sc.sc_rsp;
	struct buf *bp;

	bp = BUFQ_FIRST(&tu_sc.sc_bufq);
	if (bp == NULL)
		return;
#ifdef TUDEBUG
	printf("ctustart: %s\n", bp->b_flags & B_READ ? "READING":"WRITING");
#endif
	tu_sc.sc_tpblk = bp->b_blkno;
	tu_sc.sc_xbytes = 0;
	tu_sc.sc_op = bp->b_flags & B_READ ? RSP_OP_READ : RSP_OP_WRITE;
	tu_sc.sc_step = 0;
	bp->b_resid = bp->b_bcount;
	tu_sc.sc_wto = 0;

	rsp->rsp_typ = RSP_TYP_COMMAND;
	rsp->rsp_sz = 012;
	rsp->rsp_op = tu_sc.sc_op;
	rsp->rsp_mod = 0;
	rsp->rsp_drv = 0;
	rsp->rsp_sw = rsp->rsp_xx1 = rsp->rsp_xx2 = 0;
	rsp->rsp_cnt = bp->b_bcount;
	rsp->rsp_blk = tu_sc.sc_tpblk;
	rsp->rsp_sum = ctu_cksum((unsigned short *)rsp, 6);
	tu_sc.sc_state = TU_WORKING;
	ctutintr(NULL);
}

int
ctuioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	return ENOTTY;
}

/*
 * Not bloody likely... 
 */
int
ctudump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	return 0;
}

static int
readchr(void)
{
	int i;

	for (i = 0; i < 5000; i++)
		if ((mfpr(PR_CSRS) & 0x80))
			break;
	if (i == 5000)
		return -1;
	return mfpr(PR_CSRD);
}

/*
 * Loop in a tight (busy-wait-)loop when receiving packets, this is
 * the only way to avoid loosing characters.
 */
void
cturintr(void *arg)
{
	int status = mfpr(PR_CSRD);
	struct	buf *bp;
	int i, c, tck;
	unsigned short ck;

	bp = BUFQ_FIRST(&tu_sc.sc_bufq);
	switch (tu_sc.sc_state) {
	case TU_RESET:
		if (status != RSP_TYP_CONTINUE)
			printf("Bad response %d\n", status);
		wakeup(&tu_sc);
		return;

	case TU_READING:
		if (status != RSP_TYP_DATA)
			bp->b_flags |= B_ERROR;
		tu_sc.sc_wto = 0;
		for (i = 0; i < 131; i++) {
			if ((c = readchr()) < 0) {
#ifdef TUDEBUG
				printf("Timeout...%d\n", i);
#endif
				goto bad;
			}
			if ((i > 0) && (i < 129))
				bp->b_data[tu_sc.sc_xbytes++] = c;
			if (i == 129)
				ck = (c & 0xff);
			if (i == 130)
				ck |= ((c & 0xff) << 8);
		}
		tck = ctu_cksum((void *)&bp->b_data[tu_sc.sc_xbytes-128], 64);
		tck += 0x8001; if (tck > 0xffff) tck -= 0xffff;
		if (tck != ck) {
#ifdef TUDEBUG
			int i;
			printf("Bad cksum: tck %x != ck %x\n", tck, ck);
			printf("block %d\n", tu_sc.sc_xbytes/128-1);
			for (i = -128; i < 0; i+=16)
				printf("%x %x %x %x\n",
				    *(int *)&bp->b_data[tu_sc.sc_xbytes+i],
				    *(int *)&bp->b_data[tu_sc.sc_xbytes+i+4],
				    *(int *)&bp->b_data[tu_sc.sc_xbytes+i+8],
				    *(int *)&bp->b_data[tu_sc.sc_xbytes+i+12]);
#endif
			goto bad;
		}
		bp->b_resid = 0;
		if (bp->b_bcount == tu_sc.sc_xbytes)
			tu_sc.sc_state = TU_ENDPACKET;
		return;

	case TU_ENDPACKET:
		if (status != RSP_TYP_COMMAND) {
#ifdef TUDEBUG
			int g[14], j;
			g[0] = status;
			for (i = 1; i < 14; i++)
				if ((g[i] = readchr()) < 0)
					break;
			j=0; while (readchr() >= 0)
				j++;
			for (i = 0; i < 14; i++)
				printf("%d: %x\n", i, g[i]);
			printf("Got %d char more\n", j);
			printf("error: state %d xbytes %d status %d\n",
			    tu_sc.sc_state, tu_sc.sc_xbytes, status);
#endif
			
			bp->b_flags |= B_ERROR;
		}
		tu_sc.sc_wto = 0;
		for (i = 0; i < 13; i++) {
			if ((c = readchr()) < 0) {
#ifdef TUDEBUG
				printf("Timeout epack %d\n", i);
#endif
				goto bad;
			}
			if ((i == 2) &&
			    ((c != RSP_MOD_OK) && (c != RSP_MOD_RETR))) {
#ifdef TUDEBUG
				printf("end packet status bad: %d\n", c);
#endif
				bp->b_flags |= B_ERROR;
			}
		}
		break;

	case TU_WRITING:
#define	WAIT	while ((mfpr(PR_CSTS) & 0x80) == 0)

		if (status != RSP_TYP_CONTINUE)
			goto bad;
#ifdef TUDEBUG
		printf("Writing byte %d\n", tu_sc.sc_xbytes);
#endif
		WAIT; mtpr(RSP_TYP_DATA, PR_CSTD); 
		WAIT; mtpr(128, PR_CSTD);
		for (i = 0; i < 128; i++) {
			WAIT;
			mtpr(bp->b_data[tu_sc.sc_xbytes++], PR_CSTD);
		}
		tck = ctu_cksum((void *)&bp->b_data[tu_sc.sc_xbytes-128], 64);
		tck += 0x8001; if (tck > 0xffff) tck -= 0xffff;
		WAIT; mtpr(tck & 0xff, PR_CSTD);
		WAIT; mtpr((tck >> 8) & 0xff, PR_CSTD);
		bp->b_resid = 0;
		if (tu_sc.sc_xbytes == bp->b_bcount)
			tu_sc.sc_state = TU_ENDPACKET;
		return;
#undef WAIT

	case TU_RESTART:
		if (status != RSP_TYP_CONTINUE)
			goto bad;
		ctustart();
		return;

	default:
		printf("bad rx state %d char %d\n", tu_sc.sc_state, status);
		return;
	}
	if ((bp->b_flags & B_ERROR) == 0) {
		BUFQ_REMOVE(&tu_sc.sc_bufq, bp);
		biodone(bp);
#ifdef TUDEBUG
		printf("biodone %p\n", bp);
#endif
	}
#ifdef TUDEBUG
	  else {
		printf("error: state %d xbytes %d status %d\n",
		    tu_sc.sc_state, tu_sc.sc_xbytes, status);
	}
#endif
	bp->b_flags &= ~B_ERROR;
	tu_sc.sc_state = TU_IDLE;
	ctustart();
	return;

bad:	tu_sc.sc_state = TU_RESTART;
	ctuinit();
}

void
ctutintr(void *arg)
{
	while ((mfpr(PR_CSTS) & 0x80) == 0)
		;
	switch (tu_sc.sc_state) {
	case TU_RESET:
		switch (tu_sc.sc_step) {
		case 0:
		case 1:
			mtpr(0, PR_CSTD);
			break;
		case 2:
		case 3:
			mtpr(RSP_TYP_INIT, PR_CSTD);
			break;
		default:
			break;
		}
		tu_sc.sc_step++;
		return;

	case TU_WORKING:
		if (tu_sc.sc_step == 14) {
			if (tu_sc.sc_op == RSP_OP_READ)
				tu_sc.sc_state = TU_READING;
			else
				tu_sc.sc_state = TU_WRITING;
		} else
			mtpr(tu_sc.sc_rsp[tu_sc.sc_step++], PR_CSTD);
		return;

	case TU_IDLE:
		printf("Idle interrupt\n");
		return;

	case TU_ENDPACKET:
	case TU_WRITING:
	case TU_RESTART:
		return;

	default:
		printf("bad tx state %d\n", tu_sc.sc_state);
	}
}

unsigned short
ctu_cksum(unsigned short *buf, int words)
{
	int i, cksum;

	for (i = cksum = 0; i < words; i++)
		cksum += buf[i];

hej:	if (cksum > 65535) {
		cksum = (cksum & 65535) + (cksum >> 16);
		goto hej;
	}
	return cksum;
}

int	oldtp;

/*
 * Watch so that we don't get blocked unnecessary due to lost int's.
 */
void
ctuwatch(void *arg)
{

	callout_reset(&ctu_watch_ch, hz, ctuwatch, NULL);

	if (tu_sc.sc_state == TU_WORKING) {
		/*
		 * Died in sending command.
		 * Wait 5 secs.
		 */
		if (tu_sc.sc_wto++ > 5) {
#ifdef TUDEBUG
			printf("Died in sending command\n");
#endif
			tu_sc.sc_state = TU_RESTART;
			ctuinit();
		}
	}
	if (tu_sc.sc_state == TU_READING || tu_sc.sc_state == TU_WRITING) {
		/*
		 * Positioning, may take long time.
		 * Wait one minute.
		 */
		if (tu_sc.sc_wto++ > 60) {
#ifdef TUDEBUG
			printf("Died in Positioning, wto %d\n", tu_sc.sc_wto);
#endif
			tu_sc.sc_state = TU_RESTART;
			ctuinit();
		}
	}
}
