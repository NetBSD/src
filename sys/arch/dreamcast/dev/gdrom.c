/*	$NetBSD: gdrom.c,v 1.2 2001/01/25 01:41:47 marcus Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/cdio.h>

#include <machine/conf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <sh3/shbvar.h>


int	gdrommatch __P((struct device *, struct cfdata *, void *));
void	gdromattach __P((struct device *, struct device *, void *));
int	gdromopen __P((dev_t, int, int, struct proc *));
int	gdromclose __P((dev_t, int, int, struct proc *));
void	gdromstrategy __P((struct buf *));
int	gdromioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int	gdromdump __P((dev_t, daddr_t, caddr_t, size_t));
int	gdromsize __P((dev_t));
int	gdromread __P((dev_t, struct uio *, int));
int	gdromwrite __P((dev_t, struct uio *, int));


struct gdrom_softc {
	struct device sc_dv;	/* generic device info; must come first */
	struct disk dkdev;	/* generic disk info */
	struct buf_queue bufq;	/* queue pending I/O operations */
	struct buf curbuf;	/* state of current I/O operation */

	int is_open;
	int openpart_start;	/* start sector of currently open partition */
};

struct cfattach gdrom_ca = {
	sizeof(struct gdrom_softc), gdrommatch, gdromattach
};

struct dkdriver gdromdkdriver = { gdromstrategy };

extern struct cfdriver gdrom_cd;


struct gd_toc {
  unsigned int entry[99];
  unsigned int first, last;
  unsigned int leadout;
};

#define TOC_LBA(n) ((n)&0xffffff00)
#define TOC_ADR(n) ((n)&0x0f)
#define TOC_CTRL(n) (((n)&0xf0)>>4)
#define TOC_TRACK(n) (((n)&0x0000ff00)>>8)


#define GDROM(o) (*(volatile unsigned char *)(0xa05f7000+(o)))

#define GDSTATSTAT(n) ((n)&0xf)
#define GDSTATDISK(n) (((n)>>4)&0xf)

#define GDROM_BUSY  GDROM(0x18)
#define GDROM_DATA  (*(volatile short *)&GDROM(0x80))
#define GDROM_REGX  GDROM(0x84)
#define GDROM_STAT  GDROM(0x8c)
#define GDROM_CNTLO GDROM(0x90)
#define GDROM_CNTHI GDROM(0x94)
#define GDROM_COND  GDROM(0x9c)

int	gdrom_getstat	__P((void));
void	gdrom_wait_irq	__P((void));
int	gdrom_send_command	__P((short *ptr));
int	gdrom_do_read_command	__P((void *req, void *buf, unsigned int nbyt));
int	gdrom_do_nodata_command	__P((void *req));
int	gdrom_check_sense	__P((int cond));
int	gdrom_read_toc	__P((struct gd_toc *toc));
int	gdrom_read_sectors	__P((void *buf, int sector, int cnt));
int	gdrom_mount_disk	__P((void));


int gdrom_getstat()
{
	int s1, s2, s3;

	if(GDROM_BUSY & 0x80) return -1;
	s1 = GDROM_STAT;
	s2 = GDROM_STAT;
	s3 = GDROM_STAT;
	if(GDROM_BUSY & 0x80) return -1;
	if(s1 == s2)
	  return s1;
	else if(s2 == s3)
	  return s2;
	else
	  return -1;
}

void gdrom_wait_irq()
{
	volatile unsigned int *irq = (volatile unsigned int *)0xa05f6904;

	while(!((*irq) & 1))
	  ;
}

int gdrom_send_command(ptr)
	short *ptr;
{
	int i;

	if(GDSTATSTAT(GDROM_STAT) == 6)
	  return (-1);
	
	GDROM_COND = 0xa0;
	for(i = 0; i < 64; i++) ;
	while( (GDROM_BUSY & 0x88) != 8 ) ;
	for(i = 0; i< 6; i++)
	  GDROM_DATA = ptr[i];
	
	return (0);
}

int gdrom_do_read_command(req, buf, nbyt)
	void *req;
	void *buf;
	unsigned int nbyt;
{
	short *ptr = buf;
	int cond;
	
	while( GDROM_BUSY & 0x88 ) ;
	GDROM_CNTLO = nbyt & 0xff;
	GDROM_CNTHI = (nbyt >> 8) & 0xff;
	GDROM_REGX = 0;
	if(gdrom_send_command(req)) return -1;
	do {
	  gdrom_wait_irq();
	  cond = GDROM_COND;
#ifdef GDROMDEBUG
	  printf("cond = %x\n", cond);
#endif
	  if((cond & 8) == 8) {
	    int cnt = (GDROM_CNTHI<<8) | GDROM_CNTLO;
#ifdef GDROMDEBUG
	    printf("cnt = %d\n", cnt);
#endif
	    while(cnt > 0) {
	      *ptr++ = GDROM_DATA;
	      cnt -= 2;
	    }
	  }
	  while( GDROM_BUSY & 0x80 );
	} while(cond & 8);
	return cond;
}

int gdrom_do_nodata_command(req)
	void *req;
{
	int cond;

	while( GDROM_BUSY & 0x88 ) ;
	if(gdrom_send_command(req)) return -1;  
	gdrom_wait_irq();
	cond = GDROM_COND;
	while( GDROM_BUSY & 0x88 ) ;
	return cond;
}

int gdrom_check_sense(cond)
	int cond;
{
	/* 76543210 76543210
	 0   0x13      -
	 2    -      bufsz(hi)
	 4 bufsz(lo)   -
	 6    -        -
	 8    -        -
	 10    -        -        */
	unsigned short sense_data[5];
	unsigned char cmd[12];
	int sense_key, sense_specific;

	if(cond < 0) {
#ifdef GDROMDEBUG
	  printf("not ready (2:58)\n");
#endif
	  return EIO;
	}
	
	if(!(cond & 1)) {
#ifdef GDROMDEBUG
	  printf("no sense.  0:0\n");
#endif
	  return (0);
	}
	
	bzero(cmd, sizeof(cmd));
	
	cmd[0] = 0x13;
	cmd[4] = sizeof(sense_data);
	
	gdrom_do_read_command(cmd, sense_data, sizeof(sense_data));
	
	sense_key = sense_data[1] & 0xf;
	sense_specific = sense_data[4];
	if(sense_key == 11 && sense_specific == 0) {
#ifdef GDROMDEBUG
	  printf("aborted (ignored).  0:0\n");
#endif
	  return (0);
	}
	
#ifdef GDROMDEBUG
	printf("SENSE %d:", sense_key);
	printf("%d\n", sense_specific);
#endif
	
	return (sense_key==0? 0 : EIO);
}

int gdrom_read_toc(toc)
	struct gd_toc *toc;
{
	/* 76543210 76543210
	 0   0x14      -
	 2    -      bufsz(hi)
	 4 bufsz(lo)   -
	 6    -        -
	 8    -        -
	 10    -        -        */
	unsigned char cmd[12];

	bzero(cmd, sizeof(cmd));
	
	cmd[0] = 0x14;
	cmd[3] = sizeof(struct gd_toc)>>8;
	cmd[4] = sizeof(struct gd_toc)&0xff;
	
	return gdrom_check_sense( gdrom_do_read_command(cmd, toc, sizeof(struct gd_toc)) );
}

int gdrom_read_sectors(buf, sector, cnt)
	void *buf;
	int sector;
	int cnt;
{
	/* 76543210 76543210
	 0   0x30    datafmt
	 2  sec(hi)  sec(mid)
	 4  sec(lo)    -
	 6    -        -
	 8  cnt(hi)  cnt(mid)
	 10  cnt(lo)    -        */
	unsigned char cmd[12];

	bzero(cmd, sizeof(cmd));

	cmd[0] = 0x30;
	cmd[1] = 0x20;
	cmd[2] = sector>>16;
	cmd[3] = sector>>8;
	cmd[4] = sector;
	cmd[8] = cnt>>16;
	cmd[9] = cnt>>8;
	cmd[10] = cnt;

	return gdrom_check_sense( gdrom_do_read_command(cmd, buf, cnt<<11) );
}

int gdrom_mount_disk()
{
	/* 76543210 76543210
	 0   0x70      -
	 2   0x1f      -
	 4    -        -
	 6    -        -
	 8    -        -
	 10    -        -        */
	unsigned char cmd[12];

	bzero(cmd, sizeof(cmd));
	
	cmd[0] = 0x70;
	cmd[1] = 0x1f;
	
	return gdrom_check_sense( gdrom_do_nodata_command(cmd) );
}




int
gdrommatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	static int gdrom_matched = 0;
  	struct shb_attach_args *sa = auxp;

	/* Allow only once instance. */
	if (strcmp("gdrom", cfp->cf_driver->cd_name) || gdrom_matched)
		return(0);
	gdrom_matched = 1;
	sa->ia_iosize = 0 /* 0x100 */;
	return(1);
}

void
gdromattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct gdrom_softc *sc;

	sc = (struct gdrom_softc *)dp;

	BUFQ_INIT(&sc->bufq);

	printf("\n");

	/*
	 * Initialize and attach the disk structure.
	 */
	sc->dkdev.dk_name = sc->sc_dv.dv_xname;
	sc->dkdev.dk_driver = &gdromdkdriver;
	disk_attach(&sc->dkdev);

	/*
	 * reenable disabled drive
	 */
	{
	  register u_int32_t p, x;

	  *((volatile u_int32_t *)0xa05f74e4) = 0x1fffff;
	  for(p=0; p<0x200000/4; p++)
	    x = ((volatile u_int32_t *)0xa0000000)[p];
	}

	/*	bootdev = MAKEBOOTDEV(19, 0, 0, 0, 0);  */
}

int
gdromopen(dev, flags, devtype, p)
	dev_t dev;
	int flags, devtype;
	struct proc *p;
{
	struct gdrom_softc *sc;
	int error, unit, cnt;
	struct gd_toc toc;

#ifdef GDROMDEBUG
	printf("GDROM: open\n");
#endif

	unit = DISKUNIT(dev);
	if (unit >= gdrom_cd.cd_ndevs)
		return (ENXIO);

	sc = gdrom_cd.cd_devs[unit];
	if (sc == NULL)
		return (ENXIO);

	if (sc->is_open)
		return (EBUSY);

	for (cnt = 0; cnt < 5; cnt++)
	  if ((error = gdrom_mount_disk()) == 0)
	    break;

	if (error)
		return error;

	if ((error = gdrom_read_toc(&toc)) != 0)
	  return error;

	sc->is_open = 1;
	sc->openpart_start = 150;

#ifdef GDROMDEBUG
	printf("open OK\n");
#endif
	return (0);
}

int
gdromclose(dev, flags, devtype, p)
	dev_t dev;
	int flags, devtype;
	struct proc *p;
{
	struct gdrom_softc *sc;
	int unit;
#ifdef GDROMDEBUG
	printf("GDROM: close\n");
#endif
	unit = DISKUNIT(dev);
	sc = gdrom_cd.cd_devs[unit];

	sc->is_open = 0;

	return (0);
}

void
gdromstrategy(bp)
	struct buf *bp;
{
	struct gdrom_softc *sc;
	int unit, error;
#ifdef GDROMDEBUG
	printf("GDROM: strategy\n");
#endif
	
	unit = DISKUNIT(bp->b_dev);
	sc = gdrom_cd.cd_devs[unit];

	if (bp->b_bcount == 0)
	  goto done;

	bp->b_rawblkno = bp->b_blkno / (2048 / DEV_BSIZE) + sc->openpart_start;

#ifdef GDROMDEBUG
	printf("read_sectors(%p, %d, %ld) [%ld bytes]\n",
	       bp->b_data, bp->b_rawblkno,
	       bp->b_bcount>>11, bp->b_bcount);
#endif

	if (error = gdrom_read_sectors(bp->b_data, bp->b_rawblkno,
				       bp->b_bcount>>11)) {
	  bp->b_error = error;
	  bp->b_flags |= B_ERROR;
	}

done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

int
gdromioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct gdrom_softc *sc;
	int unit, error;
#ifdef GDROMDEBUG
	printf("GDROM: ioctl %lx\n", cmd);
#endif

	unit = DISKUNIT(dev);
	sc = gdrom_cd.cd_devs[unit];

	switch (cmd) {
	case CDIOREADMSADDR: {
		int track, sessno = *(int*)addr;
		struct gd_toc toc;

		if (sessno != 0)
			return (EINVAL);

		if ((error = gdrom_read_toc(&toc)) != 0)
		  return error;

		for (track = TOC_TRACK(toc.last);
		     track >= TOC_TRACK(toc.first);
		     --track)
		  if (TOC_CTRL(toc.entry[track-1]))
		    break;

		if (track < TOC_TRACK(toc.first) || track > 100)
		  return (ENXIO);

		*(int*)addr = htonl(TOC_LBA(toc.entry[track-1])) -
		  sc->openpart_start;

		return 0;
	}
	 default:
	   return (EINVAL);
	}

#ifdef DIAGNOSTIC
	panic("gdromioctl: impossible");
#endif
}


/*
 * Can't dump to CD; read only media...
 */
int
gdromdump(dev, blkno, va, size)
	dev_t	dev;
	daddr_t	blkno;
	caddr_t	va;
	size_t	size;
{
	return (EINVAL);
}

int
gdromsize(dev)
	dev_t dev;
{
	return (-1);
}

int
gdromread(dev, uio, flags)
	dev_t	dev;
	struct	uio *uio;
	int	flags;
{
#ifdef GDROMDEBUG
	printf("GDROM: read\n");
#endif
	return (physio(gdromstrategy, NULL, dev, B_READ, minphys, uio));
}

int
gdromwrite(dev, uio, flags)
	dev_t	dev;
	struct	uio *uio;
	int	flags;
{
	return (EROFS);
}

