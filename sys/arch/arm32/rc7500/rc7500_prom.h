/* $NetBSD: rc7500_prom.h,v 1.1 1996/08/29 22:35:45 mark Exp $ */

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
 * Copyright (c) 1995 Michael L. Hitch
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
 *      This product includes software developed by Michael L. Hitch.
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

#ifndef _PROM_H_
#define _PROM_H_

#ifndef _LOCORE
/*
 * This is the data returned in PROM_GETENV.
 */
struct prom_id {
	u_int ramsize;
	u_int physmem_start;
	u_int physmem_end;
	u_int video_start;
	u_int video_size;
	u_int display_width;
	u_int display_height;
	u_int tlb;	/* translation table base */
	u_int tlbsize;
	u_int ksize;	/* kernel size */
	u_int kstart;	/* kernel start address */
	char bootdev[32];
	char bootfile[32];
	char bootargs[32];
	u_int bootdevnum;
};
#endif /* _LOCORE */

#define PROM_REBOOT	0x06
#define PROM_GETENV	0x07
#define PROM_PUTCHAR	0x08
#define PROM_PUTS	0x09
#define PROM_GETCHAR	0x0A
#define PROM_OPEN	0x0B
#define PROM_CLOSE	0x0C
#define PROM_READ	0x0D

#ifndef _LOCORE
extern int prom_putchar(int c);
extern void prom_puts(char *s);
extern int prom_getchar(void);
extern char *prom_getenv(void);
extern int prom_open(char *dev_name, u_int devnum);
extern int prom_close(int dev);
extern int prom_read(int dev, char *io);
extern void prom_reboot(void);
#endif /* _LOCORE */

#endif /* _PROM_H_ */
