/*	$NetBSD: samachdep.h,v 1.9.2.2 2013/01/23 00:05:54 yamt Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)samachdep.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <m68k/frame.h>
#include <lib/libsa/stand.h>

#define	NSCSI		2
#define NSD		8
#define DK_NDRIVE	8

#define MHZ_8		1
#define MHZ_16		2
#define MHZ_25		3
#define MHZ_33		4
#define MHZ_50		6

#define MAXDEVNAME	16

struct consdev;
struct frame;
typedef struct label_t {
	int val[15];
} label_t;

/* autoconf.c */
void configure(void);
void find_devs(void);

/* awaitkey.c */
char awaitkey(const char *, int, bool);

/* bmc.c */
void bmccnprobe(struct consdev *);
void bmccninit(struct consdev *);
int  bmccngetc(dev_t);
void bmccnputc(dev_t, int);

/* bmd.c */
void bmdinit(void);
int bmdputc(int);
void bmdadjust(short, short);
void bmdclear(void);

/* boot.c */
extern int howto;
extern char default_file[];
int how_to_boot(int, char **);
int boot(int, char **);
int bootnetbsd(char *);

/* clock.c */
/* not yet */

/* cons.c */
void cninit(void);
int cngetc(void);
void cnputc(int);

/* devopen.c */
extern	u_int opendev;
int atoi(char *);

/* disklabel.c */
extern u_char lbl_buff[];
int disklabel(int, char **);

/* exec.c */
void exec_hp300(char *, u_long, int);

/* font.c */
extern u_short bmdfont[][20];

/* fsdump.c */
int fsdump(int, char **);
int fsrestore(int, char **);

/* getline.c */
int getline(char *, char *);

/* if_le.c */
int leinit(void *);

/* init_main.c */
extern int cpuspeed;
extern int hz;
extern int nplane;
extern int machtype;

/* kbd.c */
int kbd_decode(u_char);

/* lance.c */
void *lance_attach(int, void *, void *, uint8_t *);
void *lance_cookie(int);
uint8_t *lance_eaddr(void *);
bool lance_init(void *);
int lance_get(void *, void *, size_t);
bool lance_put(void *, void *, size_t);
bool lance_end(void *); 
int lance_intr(void);

/* locore.S */
extern	u_int bootdev;
extern int dipsw1, dipsw2;
extern int cputype;
extern volatile uint32_t tick;
int setjmp(label_t *);
int splhigh(void);
void splx(int);
int getsfc(void);
int getdfc(void);
int spl0(void);
int spl1(void);
int spl2(void);
int spl3(void);
int spl4(void);
int spl5(void);
int spl6(void);

/* ls.c */
int cmd_ls(int, char **);

/* machdep.c */
void straytrap(int);
int badaddr(volatile void *);
void regdump(int *, int);
char *hexstr(int, int);

/* prf.c */
int tgetchar(void);

/* parse.c */
int check_args(int, char **);
int exit_program(int, char **);
int parse(int, char **);
int getargs(char *, char **, int);

/* romcons.c */
void romcnprobe(struct consdev *);
void romcninit(struct consdev *);
int  romcngetc(dev_t);
void romcnputc(dev_t, int);

/* sc.c */
struct scsi_fmt_cdb;
int scsi_immed_command(int, int, int, struct scsi_fmt_cdb *, u_char *,
    unsigned int);
int scsi_request_sense(int, int, int, u_char *, unsigned int);
int scsi_test_unit_rdy(int, int, int);
int scsi_format_unit(int, int, int);
int scintr(void);

/* scsi.c */
int scsi(int, char **);
int scsi_read_raw(u_int, u_int, u_int, u_char *, u_int);
int scsi_read(u_int, u_char *, u_int);
int scsi_write(u_int, u_char *, u_int);

/* screen.c */
int screen(int, char **);

/* scsi.c */
int scsi(int, char **);

/* sd.c */
int sdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int sdopen(struct open_file *, ...);
int sdclose(struct open_file *);

/* sio.c */
void _siointr(void);
void siocnprobe(struct consdev *);
void siocninit(struct consdev *);
int  siocngetc(dev_t);
void siocnputc(dev_t, int);
void sioinit(void);

/* tape.c */
int tape(int, char **);

/* trap.c */
void trap(int, unsigned int, unsigned int, struct frame);

/* ufs_disklabel.c */
char *readdisklabel(int, int, struct disklabel *);


#define DELAY(n)							\
do {									\
	register int __N = cpuspeed * (n);				\
	do {								\
		__asm("subql #1, %0" : "=r" (__N) : "0" (__N));		\
	} while (__N > 0);						\
} while (/* CONSTCOND */ 0)

extern	struct fs_ops file_system_disk[];
extern	int nfsys_disk;
extern	struct fs_ops file_system_nfs[];

extern	const char bootprog_name[], bootprog_rev[], bootprog_kernrev[];
