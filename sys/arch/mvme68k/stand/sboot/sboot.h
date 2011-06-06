/*	$NetBSD: sboot.h,v 1.6.72.1 2011/06/06 09:06:16 jruoho Exp $	*/

/*
 * Copyright (c) 1995 Charles D. Cranor and Seth Widoff
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

/*
 * sboot.h: stuff for MVME147's serial line boot
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

extern void *end;


/* console */
void puts(char *);
void putchar(int);
char cngetc(void);
char *ngets(char *, int);
void consinit(void);

/* sboot */
void callrom(void);
void do_cmd(char *, char *);

/* le */
#define LANCE_ADDR 0xfffe0778
#define ERAM_ADDR  0xfffe0774
#define LANCE_REG_ADDR 0xfffe1800
void le_end(void);
void le_init(void);
int le_get(u_char *, size_t, u_long);
int le_put(u_char *, size_t);

/* etherfun */
#define READ 0
#define ACKN 1 
void do_rev_arp(void);
int get_rev_arp(void);
int rev_arp(void);
void do_send_tftp(int);
int do_get_file(void); 
void tftp_file(char *, u_long);

/* clock */
u_long time(void);

/* checksum */
u_long oc_cksum (void *, u_long, u_long);

/* start */
void go(int, void *, void *);

#define CONS_ZS_ADDR (0xfffe3002)
#define CLOCK_ADDR (0xfffe07f8)
#define LOAD_ADDR 0x8000 

unsigned char myea[6];                /* my ether addr */
unsigned char myip[4];
unsigned char servip[4];
unsigned char servea[6];
u_short myport;
u_short servport;
unsigned char reboot;
