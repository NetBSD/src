/*	$NetBSD: bootxx.c,v 1.1 1995/03/29 21:24:06 ragge Exp $ */
/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)boot.c	7.15 (Berkeley) 5/4/91
 */

#include "sys/param.h"
#include "sys/reboot.h"
#include "lib/libsa/stand.h"
#include "lib/libsa/ufs.h"
#include "sys/disklabel.h"
#include "../mba/mbareg.h"
#include "../mba/hpreg.h"
#include "../include/pte.h"
#include "../include/sid.h"
#include "../include/mtpr.h"
#include "data.h"

#include <a.out.h>

int	romstrategy(),romopen();
/*
 * Boot program... arguments passed in r10 and r11 determine
 * whether boot stops to ask for system name and which device
 * boot comes from.
 *
 * bertram 22-mar-1995: passing arguments in registers removed.
 *			also any asm-declaration for fixed registers.
 *			(this was a problem for my cross-compiler)
 *			[also in boot.c and init.c]
 */

volatile u_int devtype, bootdev;
unsigned opendev,boothowto,bootset;

int cpu_type, cpunumber;

int is_uVAX (void) {
  return ((cpunumber == VAX_78032 || cpunumber == VAX_650) ? 1 : 0);
}
int is_750 (void) {
  return (cpunumber == VAX_750 ? 1 : 0);
}

main()
{
	int io, retry, type, i;
	
	initData ();

#ifndef FILES_OK	/* need this for now. (bertram) */
	printf ("\ncpunumber: %d, cpu_type: %x\n", cpunumber, cpu_type);
        printf  ("files: %d %d %d %d\n", files[0].f_flags, 
                files[1].f_flags, files[2].f_flags, files[2].f_flags);
	for (i=0; i<SOPEN_MAX; i++)
	  files[i].f_flags = 0;
#endif

	if (is_uVAX()) {
	  boothowto = bootregs[5];
	  bootdev = rpb->devtyp;
	}
	else {
	  bootdev  = bootregs[10];
	  boothowto = bootregs[11];
	}
	bootset=getbootdev();
	
	printf("howto %x, bdev %x, booting...\n", boothowto, bootdev);
	io = open("boot", 0);
	printf ("io = %d\n", io);
	if (io >= 0 && io < SOPEN_MAX) {
		copyunix(io);
	} else {
		printf("Boot failed.\n");
	}
	asm("halt");
}

/*ARGSUSED*/
copyunix(aio)
{
	register int esym;		/* must be r9 */
	struct exec x;
	register int io = aio, i;
	char *addr;

	i=read(io, (char *)&x, sizeof(x));
	if (i != sizeof(x) || N_BADMAG(x)) {
		printf("Bad format: errno %s\n",strerror(errno));
		return;
	}
	printf("%d", x.a_text);
	if (N_GETMAGIC(x) == ZMAGIC && lseek(io, 0x400, SEEK_SET) == -1)
		goto shread;
	if (read(io, (char *)0x10000, x.a_text) != x.a_text)
		goto shread;
	addr = (char *)x.a_text;
	if (N_GETMAGIC(x) == ZMAGIC || N_GETMAGIC(x) == NMAGIC)
		while ((int)addr & CLOFSET)
			*addr++ = 0;
	printf("+%d", x.a_data);
	if (read(io, addr+0x10000, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	bcopy((void*)0x10000,0,(int)addr);
	printf("+%d", x.a_bss);
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	for (i = 0; i < 128*512; i++)	/* slop */
		*addr++ = 0;
	printf(" start 0x%x, bootdev %x\n", x.a_entry,bootset);
	hoppabort(x.a_entry, boothowto, bootset);
	(*((int (*)()) x.a_entry))();
	return;
shread:
	printf("Short read\n");
	return;
}

getbootdev()
{
	int i,major, adaptor, controller, unit, partition;

	if (is_uVAX()) {
	  unit = rpb->unit;
	  controller = rpb->slave;
	  adaptor = rpb->csrphy;
	}
	else {
	  unit=bootregs[3];
	  controller = 0;
	}
	partition = 0;

	switch(bootdev){
	case 0: /* massbuss boot */
		major = 0;		/*  hp / ...  */
		adaptor=(bootregs[1] & 0x6000) >> 17;
		break;

	case 17: /* UDA50 boot */
		major = 9;		/*  ra / mscp  */
		if (! is_uVAX()) 
		  adaptor=(bootregs[1] & 0x40000 ? 0 : 1);
		break;
		
	case 18: /* TK50 boot */
		major = 8;		/*  tm / tmscp  */
		if (is_uVAX()) 
		  break;

	default:
		printf("Unsupported boot device %d, trying anyway.\n",bootdev);
		boothowto|=(RB_SINGLE|RB_ASKNAME);
	}

	return MAKEBOOTDEV(major, adaptor, controller, unit, partition);
}

struct  devsw devsw[]={
	{"rom",romstrategy, nullsys,nullsys,noioctl}
};

int     ndevs = (sizeof(devsw)/sizeof(devsw[0]));

struct fs_ops file_system[] = {
        { ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat }
};

int nfsys = (sizeof(file_system) / sizeof(struct fs_ops));
struct disklabel lp;

devopen(f, fname, file)
        struct open_file *f;
        char *fname;
        char **file;
{
        char *msg;
	f->f_dev = &devsw[0];
	*file=fname;

	if (is_uVAX()) 
	  initCtrl ();

        msg=getdisklabel((void*)RELOC+LABELOFFSET, &lp);
        if(msg)printf("getdisklabel: %s\n",msg);
	return 0;
}

extern int bulkread630 (int lbn, int size, void *buf, int *regs);

extern int read750 (int block, int *regs);
extern int read630 (int block, int *regs);


romstrategy(sc,func,dblk,size,buf,rsize)
	void *sc;
	int func;
	daddr_t dblk;
	char *buf;
	int size, *rsize;
{
	int nsize=size,block=dblk;
        int (*readblk)(int,int*);

	if (is_uVAX() && (bootdev == 17 || bootdev == 18)) {
	  int res;
          *rsize = nsize;
          res = bulkread630 (block, size, buf, bootregs);
          return (res & 0x01 ? 0 : -1);
	}

	if (is_750() && bootdev == 0) {
		*rsize=nsize;
		return hpread(block,size,buf);
	}

	if (is_uVAX())
	  readblk = read630;
	else if (is_750())
	  readblk = read750;
	else {
          printf ("Unuspported VAX-type %d.\n", cpunumber);
          return (1);
	}

	while (size > 0) {
	  if (! (readblk (block, bootregs) & 0x01))
	    return (1);			/* low-bit clear indicates error */

	  bcopy (0, buf, 512);		/* readblk writes at adress 0x0 */
          size -= 512;
	  buf += 512;
	  block++;
	}
	*rsize=nsize;
	return 0;
}

hpread(block,size,buf)
	char *buf;
{
	volatile struct mba_regs *mr=(void *)bootregs[1];
	volatile struct hp_drv *hd=&mr->mba_md[bootregs[3]];
	struct disklabel *dp=&lp;
	u_int pfnum, nsize, mapnr,bn, cn, sn, tn;

	pfnum=(u_int)buf>>PGSHIFT;

	for(mapnr=0, nsize=size;(nsize+NBPG)>0;nsize-=NBPG)
		mr->mba_map[mapnr++]=PG_V|pfnum++;

	mr->mba_var=((u_int)buf&PGOFSET);
	mr->mba_bc=(~size)+1;
	bn=block;
	cn=bn / dp->d_secpercyl;
	sn=bn % dp->d_secpercyl;
	tn=sn / dp->d_nsectors;
	sn=sn % dp->d_nsectors;
	hd->hp_dc=cn;
	hd->hp_da=(tn<<8)|sn;
	hd->hp_cs1=HPCS_READ;
	while(mr->mba_sr&MBASR_DTBUSY);
	if(mr->mba_sr&MBACR_ABORT) return 1;
	return 0;
}
