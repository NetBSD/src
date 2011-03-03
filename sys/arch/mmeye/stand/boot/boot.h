/*	$NetBSD: boot.h,v 1.1 2011/03/03 05:59:37 kiyohara Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#if defined(_DEBUG)
#define DPRINTF(x)	printf x;
#else
#define DPRINTF(x)
#endif

#define MAXDEVNAME	16
#define DEFBOOTDEV	"wd0a"
#define DEFKERNELNAME	kernelnames[0]

extern char *kernelnames[];

/*
 * entry
 */
void _cpu_spin(uint32_t);	/* for delay loop. */

/*
 * boot
 */
void delay(int);
void _rtt(void);

/*
 * console
 */
char *cninit(void);
int   cngetc(void);
void  cnputc(int);
int   cnscan(void);

/*
 * clock
 */
void tmu_init(void);
void delay(int);

/*
 * wd
 */
int wdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int wdopen(struct open_file *, ...);
int wdclose(struct open_file *);

/*
 * devopen
 */
int devparse(const char *, int *, uint8_t *, uint8_t *, const char **);

/*
 * tgetc
 */
int tgets(char *);
