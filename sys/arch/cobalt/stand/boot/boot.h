/*	$NetBSD: boot.h,v 1.12.62.1 2015/09/22 12:05:39 skrll Exp $	*/

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

#include <mips/cpuregs.h>

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
 * com
 */
int comspeed(long);

/*
 * console
 */
char *cninit(int *, int *);
int   cngetc(void);
void  cnputc(int);
int   cnscan(void);

/*
 * clock
 */
void delay(int);

/*
 * wd
 */
int wdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int wdopen(struct open_file *, ...);
int wdclose(struct open_file *);

/*
 * tlp
 */
void *tlp_init(void *); 
int tlp_send(void *, char *, u_int);
int tlp_recv(void *, char *, u_int, u_int);

extern struct netif_driver ether_tlp_driver;

/*
 * devopen
 */
int devparse(const char *, int *, uint8_t *, uint8_t *, const char **);

/*
 * tgetc
 */
int tgets(char *);

/*
 * cache
 */
#define CACHELINESIZE	32

void pdcache_wb(uint32_t, u_int);
void pdcache_inv(uint32_t, u_int);
void pdcache_wbinv(uint32_t, u_int);

/*
 * pci
 */
uint32_t pcicfgread(uint32_t, uint32_t);

/*
 * lcd
 */
void lcd_init(void);
void lcd_banner(void);
void lcd_loadfile(const char *);
void lcd_failed(void);
