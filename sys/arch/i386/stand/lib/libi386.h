/*	$NetBSD: libi386.h,v 1.3 1997/06/13 17:49:25 drochner Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
void vpbcopy __P((void*, physaddr_t, int));
void pvbcopy __P((physaddr_t, void*, int));
void pbzero __P((physaddr_t, int));
physaddr_t vtophys __P((void*));

int pread __P((int, physaddr_t, int));
void startprog __P((physaddr_t, int, unsigned long*, physaddr_t));

int exec_netbsd __P((const char*, physaddr_t, int, char*, char*));
int netbsd_opt __P((char));

void delay __P((int));
int getbasemem __P((void));
int getextmem __P((void));
void reboot __P((void));
void gateA20 __P((void));

char *initio __P((int));
#define CONSDEV_PC 0
#define CONSDEV_COM0 1
#define CONSDEV_COM1 2
#define CONSDEV_AUTO (-1)
int iskey __P((void));

#ifdef COMPAT_OLDBOOT
int biosdisk_gettype __P((struct open_file*));

/* this is in "user code"! */
int parsebootfile __P((const char *, char**, char**, unsigned int*,
		       unsigned int*, const char**));
#endif

#ifdef XMS
physaddr_t ppbcopy __P((physaddr_t, physaddr_t, int));
int checkxms __P((void));
physaddr_t xmsalloc __P((int));
#endif
