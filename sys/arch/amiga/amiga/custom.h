/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/* This is a rewrite (retype) of the Amiga's custom chip register map, based
   on the Hardware Reference Manual.  It is NOT based on the Amiga's
   hardware/custom.h.  */

#ifndef _amiga_custom_
#define _amiga_custom_

/*#include <machine/vm_param.h>*/


#define PHYS_CUSTOM 0xdff000


#ifndef LOCORE
struct Custom
  {
    /*** read-only registers ***/

    short zz1;
    short dmaconr;
    short vposr;
    short vhposr;
    short zz2;
    short joy0dat;
    short joy1dat;
    short clxdat;
    short adkconr;
    short pot0dat;
    short pot1dat;
    short potgor;
    short serdatr;
    short dskbytr;
    short intenar;
    short intreqr;

    /*** write-only registers ***/

    /* disk */
    void *dskpt;
    short dsklen;

    short zz3[2];
    short vposw;
    short vhposw;
    short copcon;
    short serdat;
    short serper;
    short potgo;
    short joytest;
    short zz4[4];

    /* blitter */
    short bltcon0;
    short bltcon1;
    short bltafwm;
    short bltalwn;
    void *bltcpt;
    void *bltbpt;
    void *bltapt;
    void *bltdpt;
    short bltsize;
    short zz5[3];
    short bltcmod;
    short bltbmod;
    short bltamod;
    short bltdmod;
    short zz6[4];
    short bltcdat;
    short bltbdat;
    short bltadat;
    short zz7[4];

    /* more disk */
    short dsksync;

    /* copper */
    union {
      void *cp;
      struct {
        short ch, cl;
      } cs;
    } _cop1lc;
#define cop1lc	_cop1lc.cp
#define cop1lch	_cop1lc.cs.ch
#define cop1lcl	_cop1lc.cs.cl
    union {
      void *cp;
      struct {
        short ch, cl;
      } cs;
    } _cop2lc;
#define cop2lc	_cop2lc.cp
#define cop2lch	_cop2lc.cs.ch
#define cop2lcl	_cop2lc.cs.cl
    short copjmp1;
    short copjmp2;
    short copins;

    /* display parameters */
    short diwstrt;
    short diwstop;
    short ddfstrt;
    short ddfstop;

    /* control registers */
    short dmacon;
    short clxcon;
    short intena;
    short intreq;

    /* audio */
    short adkcon;
    struct Audio
      {
        void *lc;
        short len;
        short per;
        short vol;
        short zz[3];
      } aud[4];

    /* display */
    union {
      void *bp[6];
      struct {
        short bph;
        short bpl;
      } bs[6];

    } _bplpt;
#define bplpt	_bplpt.bp
#define bplptl(n)	_bplpt.bs[n].bpl
#define bplpth(n)	_bplpt.bs[n].bph

    short zz8[4];
    short bplcon0;
    short bplcon1;
    short bplcon2;
    short zz9;
    short bpl1mod;
    short bpl2mod;
    short zz10[2+6+2];

    /* sprites */
    void *sprpt[8];
    struct Sprite
      {
        short pos;
        short ctl;
        short data;
        short datb;
      } spr[8];

    short color[32];
  };
#endif


/* Custom chips as seen by the kernel */
#ifdef KERNEL
#ifndef LOCORE
extern volatile struct Custom *CUSTOMbase;
#endif
#define custom (*((volatile struct Custom *)CUSTOMbase))
#endif


/* This is used for making copper lists.  */
#define CUSTOM_OFS(field) ((long)&((struct Custom*)0)->field)



/* Bit definitions for dmacon and dmaconr */
#define DMAB_SETCLR     15
#define DMAB_BLTDONE    14
#define DMAB_BLTNZERO   13
#define DMAB_BLITHOG    10
#define DMAB_MASTER     9
#define DMAB_RASTER     8
#define DMAB_COPPER     7
#define DMAB_BLITTER    6
#define DMAB_SPRITE     5
#define DMAB_DISK       4
#define DMAB_AUD3       3
#define DMAB_AUD2       2
#define DMAB_AUD1       1
#define DMAB_AUD0       0

#define DMAF_SETCLR     (1<<DMAB_SETCLR)
#define DMAF_BLTDONE    (1<<DMAB_BLTDONE)
#define DMAF_BLTNZERO   (1<<DMAB_BLTNZERO)
#define DMAF_BLITHOG    (1<<DMAB_BLITHOG)
#define DMAF_MASTER     (1<<DMAB_MASTER)
#define DMAF_RASTER     (1<<DMAB_RASTER)
#define DMAF_COPPER     (1<<DMAB_COPPER)
#define DMAF_BLITTER    (1<<DMAB_BLITTER)
#define DMAF_SPRITE     (1<<DMAB_SPRITE)
#define DMAF_DISK       (1<<DMAB_DISK)
#define DMAF_AUD3       (1<<DMAB_AUD3)
#define DMAF_AUD2       (1<<DMAB_AUD2)
#define DMAF_AUD1       (1<<DMAB_AUD1)
#define DMAF_AUD0       (1<<DMAB_AUD0)



/* Bit definitions for intena, intenar, intreq, and intreqr */
#define INTB_SETCLR     15
#define INTB_INTEN      14
#define INTB_EXTER      13
#define INTB_DSKSYNC    12
#define INTB_RBF        11
#define INTB_AUD3       10
#define INTB_AUD2       9
#define INTB_AUD1       8
#define INTB_AUD0       7
#define INTB_BLIT       6
#define INTB_VERTB      5
#define INTB_COPER      4
#define INTB_PORTS      3
#define INTB_SOFTINT    2
#define INTB_DSKBLK     1
#define INTB_TBE        0

#define INTF_SETCLR     (1<<INTB_SETCLR)
#define INTF_INTEN      (1<<INTB_INTEN)
#define INTF_EXTER      (1<<INTB_EXTER)
#define INTF_DSKSYNC    (1<<INTB_DSKSYNC)
#define INTF_RBF        (1<<INTB_RBF)
#define INTF_AUD3       (1<<INTB_AUD3)
#define INTF_AUD2       (1<<INTB_AUD2)
#define INTF_AUD1       (1<<INTB_AUD1)
#define INTF_AUD0       (1<<INTB_AUD0)
#define INTF_BLIT       (1<<INTB_BLIT)
#define INTF_VERTB      (1<<INTB_VERTB)
#define INTF_COPER      (1<<INTB_COPER)
#define INTF_PORTS      (1<<INTB_PORTS)
#define INTF_SOFTINT    (1<<INTB_SOFTINT)
#define INTF_DSKBLK     (1<<INTB_DSKBLK)
#define INTF_TBE        (1<<INTB_TBE)



#endif /* _machine_custom_ */
