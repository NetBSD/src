/*	$NetBSD: nextrom.h,v 1.7 2001/08/31 04:44:56 simonb Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
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

#ifndef NEXTROM_H_INCLUDED
#define NEXTROM_H_INCLUDED

#define MG_simm 0
#define MG_flags 4
#define MG_sid 6
#define MG_pagesize 10
#define MG_mon_stack 14
#define MG_vbr 18
#define MG_nvram 22
#define MG_inetntoa 54
#define MG_inputline 72
#define MG_region 200
#define MG_alloc_base 232
#define MG_alloc_brk 236
#define MG_boot_dev 240
#define MG_boot_arg 244
#define MG_boot_info 248
#define MG_boot_file 252
#define MG_bootfile 256
#define MG_boot_how 320
#define MG_km 324
#define MG_km_flags 368
#define MG_mon_init 370
#define MG_si 374
#define MG_time 378
#define MG_sddp 382
#define MG_dgp 386
#define MG_s5cp 390
#define MG_odc 394
#define MG_odd 398
#define MG_radix 402
#define MG_dmachip 404
#define MG_diskchip 408
#define MG_intrstat 412
#define MG_intrmask 416
#define MG_nofault 420
#define MG_fmt 424
#define MG_addr 426
#define MG_na 458
#define MG_mx 462
#define MG_my 466
#define MG_cursor_save 470
#define MG_getc 726
#define MG_try_getc 730
#define MG_putc 734
#define MG_alert 738
#define MG_alert_confirm 742
#define MG_alloc 746
#define MG_boot_slider 750
#define MG_eventc 754
#define MG_event_high 758
#define MG_animate 762
#define MG_anim_time 766
#define MG_scsi_intr 770
#define MG_scsi_intrarg 774
#define MG_minor 778
#define MG_seq 780
#define MG_anim_run 782
#define MG_major 786
#define MG_con_slot 844
#define MG_con_fbnum 845
#define MG_con_map_vaddr0 860
#define MG_con_map_vaddr1 872
#define MG_con_map_vaddr2 884
#define MG_con_map_vaddr3 896
#define MG_con_map_vaddr4 908
#define MG_con_map_vaddr5 920


/*
 * Darrin B Jewell <jewell@mit.edu>  Mon Jan 19 13:17:20 1998
 * I made up these:
 */
#define MG_clientetheraddr 788
#define MG_machine_type    936
#define MG_board_rev       937

#if 0

/*
 *  The ROM monitor uses the old structure alignment for backward
 *  compatibility with previous ROMs.  The old alignment is enabled
 *  with the following pragma.  The kernel uses the "MG" macro to
 *  construct an old alignment offset into the mon_global structure.
 *  The kernel file <mon/assym.h> should be copied from the "assym.h"
 *  found in the build directory of the current ROM release.
 *  It will contain the proper old alignment offset constants.
 */
#if	MONITOR
#pragma	CC_OLD_STORAGE_LAYOUT_ON
#else	MONITOR
#import <mon/assym.h>
#define	MG(type, off) \
	((type) ((u_int) (mg) + off))
#endif	/* MONITOR */

#import <mon/nvram.h>
#import <mon/region.h>
#import <mon/tftp.h>
#import <mon/sio.h>
#import <mon/animate.h>
#import <mon/kmreg.h>
#import <next/cpu.h>
#import <next/machparam.h>

#define	LMAX		128
#define	NBOOTFILE	64
#define	NADDR		8

struct mon_global {
	char mg_simm[N_SIMM];	/* MUST BE FIRST (accesed early by locore) */
	char mg_flags;		/* MUST BE SECOND */
#define	MGF_LOGINWINDOW		0x80
#define	MGF_UART_SETUP		0x40
#define	MGF_UART_STOP		0x20
#define	MGF_UART_TYPE_AHEAD	0x10
#define	MGF_ANIM_RUN		0x08
#define	MGF_SCSI_INTR		0x04
#define	MGF_KM_EVENT		0x02
#define	MGF_KM_TYPE_AHEAD	0x01
	u_int mg_sid, mg_pagesize, mg_mon_stack, mg_vbr;
	struct nvram_info mg_nvram;
	char mg_inetntoa[18];
	char mg_inputline[LMAX];
	struct mon_region mg_region[N_SIMM];
	caddr_t mg_alloc_base, mg_alloc_brk;
	char *mg_boot_dev, *mg_boot_arg, *mg_boot_info, *mg_boot_file;
	char mg_bootfile[NBOOTFILE];
	enum SIO_ARGS mg_boot_how;
	struct km_mon km;
	int mon_init;
	struct sio *mg_si;
	int mg_time;
	char *mg_sddp;
	char *mg_dgp;
	char *mg_s5cp;
	char *mg_odc, *mg_odd;
	char mg_radix;
	int mg_dmachip;
	int mg_diskchip;
	volatile int *mg_intrstat;
	volatile int *mg_intrmask;
	void (*mg_nofault)();
	char fmt;
	int addr[NADDR], na;
	int	mx, my;			/* mouse location */
	u_int	cursor_save[2][32];
	int (*mg_getc)(), (*mg_try_getc)(), (*mg_putc)();
	int (*mg_alert)(), (*mg_alert_confirm)();
	caddr_t (*mg_alloc)();
	int (*mg_boot_slider)();
	volatile u_char *eventc_latch;
	volatile u_int event_high;
	struct animation *mg_animate;
	int mg_anim_time;
	void (*mg_scsi_intr)();
	int mg_scsi_intrarg;
	short mg_minor, mg_seq;
	int (*mg_anim_run)();
	short mg_major;
	char *mg_clientetheraddr;
	int mg_console_i;
	int mg_console_o;
#define	CONS_I_KBD	0
#define	CONS_I_SCC_A	1
#define	CONS_I_SCC_B	2
#define	CONS_I_NET	3
#define	CONS_O_BITMAP	0
#define	CONS_O_SCC_A	1
#define	CONS_O_SCC_B	2
#define	CONS_O_NET	3
	char *test_msg;
	/* Next entry should be km_coni. Mach depends on this! */
	struct km_console_info km_coni;	/* Console configuration info. See kmreg.h */
	char *mg_fdgp;
	char mg_machine_type, mg_board_rev;
	int (*mg_as_tune)();
	int mg_flags2;
#define	MGF2_PARITY	0x80000000
};

struct mon_global *restore_mg();
caddr_t mon_alloc();

#endif /* if 0 */

#define	N_SIMM		4		/* number of SIMMs in machine */

/* SIMM types */
#define SIMM_SIZE       0x03
#define	SIMM_SIZE_EMPTY	0x00
#define	SIMM_SIZE_16MB	0x01
#define	SIMM_SIZE_4MB	0x02
#define	SIMM_SIZE_1MB	0x03
#define	SIMM_PAGE_MODE	0x04
#define	SIMM_PARITY	0x08 /* ?? */

/* Space for onboard RAM
 */
#define	MAX_PHYS_SEGS	N_SIMM+1

/* Machine types, used in both assembler and C sources. */
#define	NeXT_CUBE	0
#define	NeXT_WARP9	1
#define	NeXT_X15	2
#define	NeXT_WARP9C	3

#define NeXT_TURBO_COLOR 5			/* probed witnessed */

#define	ROM_STACK_SIZE	(8192 - 2048)

extern u_char rom_enetaddr[];
extern u_char rom_boot_dev[];
extern u_char rom_boot_arg[];
extern u_char rom_boot_info[];
extern u_char rom_boot_file[];
extern u_char rom_bootfile[];
extern char rom_machine_type;

extern u_int  monbootflag;

#endif /* NEXTROM_H_INCLUDED */
