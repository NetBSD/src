/*	$NetBSD: data.h,v 1.1 1995/03/29 21:24:09 ragge Exp $ */
/*
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by
 * Bertram Barth.
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

 /* All bugs are subject to removal without further notice */
		


extern int bootregs[16];

/*
 * "VAX/VMS Internals and Data Structures"
 * gives the following description of RPB on page 907 ff.
 */

#ifndef byte
# define byte unsigned char
#endif

struct rpb {	/* size		description */
  long base;		/*  4  physical base address of block */
  long restart;		/*  4  physical address of EXE$RESTART */
  long chksum;		/*  4  checksum of first 31 longwords of EXE$RESTART */
  long rststflg;	/*  4  Restart in progress flag */
  long haltpc;		/*  4  PC at HALT/restart */
  /* offset: 20 */
  long  haltpsl;	/*  4  PSL at HALT/restart */
  long  haltcode;	/*  4  reason for restart */
  long  bootr0;         /* 24  Saved bootstrap parameters (R0 through R5) */
  long  bootr1;
  long  bootr2;
  long  bootr3;
  long  bootr4;
  long  bootr5;
  long  iovec;		/*  4  Address of bootstrap driver */
  long  iovecsz;        /*  4  Size (in bytes) of bootstrap driver */
  /* offset: 60 */
  long  fillbn;         /*  4  LBN of seconday bootstrap file */
  long  filsiz;         /*  4  Size (in blocks) of seconday bootstrap file */
  long  pfnmap[2];	/*  8  Descriptor of PFN bitmap */
  long  pfncnt;         /*  4  Count of physical pages */
  /* offset: 80 */
  long  svaspt;         /*  4  system virtual address of system page table */
  long  csrphy;         /*  4  Physical Address of UBA device CSR */
  long  csrvir;         /*  4  Virtual Address of UBA device CSR */
  long  adpphy;         /*  4  Physical Address of adapter configurate reg. */
  long  adpvir;         /*  4  Virtual Address of adapter configurate reg. */
  /* offset: 100 */
  short unit;           /*  2  Bootstrap device unit number */
  byte  devtyp;         /*  1  Bootstrap device type code */
  byte  slave;          /*  1  Bootstrap device slave unit number */
  char  file[40];       /* 40  Secondary bootstrap file name */
  byte  confreg[16];    /* 16  Byte array of adapter types */
  /* offset: 160 */
#if 0
  byte  hdrpgcnt;	/*  1  Count of header pages in 2nd bootstrap image */
  short bootndt;	/*  2  Type of boot adapter */
  byte  flags;		/*  1  Miscellaneous flag bits */
#else
  long  align;		/*     if the compiler doesnt proper alignment */
#endif
  long  max_pfn;	/*  4  Absolute highest PFN */
  long  sptep;		/*  4  System space PTE prototype register */
  long  sbr;		/*  4  Saved system base register */
  long  cpudbvec;	/*  4  Physical address of per-CPU database vector */
  /* offset: 180 */
  long  cca_addr;	/*  4  Physical address of CCA */
  long  slr;		/*  4  Saved system length register */
  long  memdesc[8];	/* 64  Longword array of memory descriptors */
  long  smp_pc;		/*  4  SMP boot page physical address */
  long  wait;		/*  4  Bugcheck loop code for attached processor */
  /* offset: 260 */
  long  badpgs;		/*  4  Number of bad pages found in memory scan */
  byte  ctrlltr;        /*  1  Controller letter designation */
  byte  scbpagct;       /*  1  SCB page count */
  byte  reserved[6];	/*  6  -- */
  long  vmb_revision;	/*  4  VMB revision label */
} ;

extern struct rpb *rpb;

/*
 * rpb->iovec gives pointer to this structure.
 *
 * bqo->unit_init() is used to initialize the controller,
 * bqo->qio() is used to read from boot-device/
 */

struct bqo {
  long  qio;            /*  4  QIO entry  */
  long  map;            /*  4  Mapping entry  */
  long  select;         /*  4  Selection entry  */
  long  drivrname;      /*  4  Offset to driver name  */
  short version;        /*  2  Version number of VMB  */
  short vercheck;       /*  2  Check field  */
  /* offset: 20 */
  long  reselect;       /*  4  Reselection entry  */
  long  move;           /*  4  Move driver entry  */
  long  unit_init;      /*  4  Unit initialization entry  */
  long  auxdrname;      /*  4  Offset to auxiliary driver name  */
  long  umr_dis;        /*  4  UNIBUS Map Registers to disable  */
  /* offset: 40 */
  long  ucode;          /*  4  Absolute address of booting microcode  */
  long  unit_disc;      /*  4  Unit disconnecting entry */
  long  devname;        /*  4  Offset to boot device name */
  long  umr_tmpl;       /*  4  UNIBUS map register template */
  /* offset: 60 */
  /*
   * the rest is unknown / unneccessary ...
   */
  long  xxx[6];		/* 24 --	total: 84 bytes */
} ;

extern struct bqo *bqo;

/*
 * EOF
 */
