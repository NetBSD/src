/*	$NetBSD: libi386.h,v 1.16 2004/03/24 16:46:28 drochner Exp $	*/

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

typedef unsigned long physaddr_t;

/* this is in startup code */
void vpbcopy __P((const void *, void *, size_t));
void pvbcopy __P((const void *, void *, size_t));
void pbzero __P((void *, size_t));
physaddr_t vtophys __P((void*));

ssize_t pread __P((int, void *, size_t));
void startprog __P((physaddr_t, int, unsigned long*, physaddr_t));

int exec_netbsd __P((const char*, physaddr_t, int));

void delay __P((int));
int getbasemem __P((void));
int getextmemx __P((void));
int getextmem1 __P((void));
int biosvideomode __P((void));
#ifdef CONSERVATIVE_MEMDETECT
#define getextmem() getextmem1()
#else
#define getextmem() getextmemx()
#endif
void printmemlist __P((void));
void reboot __P((void));
void gateA20 __P((void));

void initio __P((int));
#define CONSDEV_PC 0
#define CONSDEV_COM0 1
#define CONSDEV_COM1 2
#define CONSDEV_COM2 3
#define CONSDEV_COM3 4
#define CONSDEV_COM0KBD 5
#define CONSDEV_COM1KBD 6
#define CONSDEV_COM2KBD 7
#define CONSDEV_COM3KBD 8
#define CONSDEV_AUTO (-1)
int iskey __P((void));
char awaitkey __P((int, int));

/* this is in "user code"! */
int parsebootfile __P((const char *, char**, char**, unsigned int*,
		       unsigned int*, const char**));

#ifdef XMS
physaddr_t ppbcopy __P((physaddr_t, physaddr_t, int));
int checkxms __P((void));
physaddr_t xmsalloc __P((int));
#endif

/* parseutils.c */
char *gettrailer __P((char*));
int parseopts __P((const char*, int*));
int parseboot __P((char*, char**, int*));

/* menuutils.c */
struct bootblk_command {
	const char *c_name;
	void (*c_fn) __P((char *));
};
void bootmenu __P((void));
void docommand __P((char*));

/* getsecs.c */
time_t getsecs __P((void));

/* in "user code": */
void command_help __P((char *));
extern const struct bootblk_command commands[];

/* asm bios/dos calls */
extern int biosdiskreset(int);
extern int biosextread(int, void *);
extern int biosgetrtc(u_long *);
extern int biosgetsystime(void);
extern int biosread(int, int, int, int, int, void *);
extern int comgetc(int);
extern void cominit(int);
extern int computc(int, int);
extern int comstatus(int);
extern int congetc(void);
extern int coniskey(void);
extern void conputc(int);

extern int get_diskinfo(int);
extern int getextmem2(int *);
extern int getextmemps2(void *);
extern int getmementry(int *, int *);
extern int int13_extension(int);
struct biosdisk_ext13info;
extern void int13_getextinfo(int, struct biosdisk_ext13info *);
extern int pcibios_cfgread(unsigned int, int, int *);
extern int pcibios_cfgwrite(unsigned int, int, int);
extern int pcibios_finddev(int, int, int, unsigned int *);
extern int pcibios_present(int *);

extern void dosclose(int);
extern int doserrno;	/* in dos_file.S */
extern int dosopen(char *);
extern int dosread(int, char *, int);
extern int dosseek(int, int, int);
