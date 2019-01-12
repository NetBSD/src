/*	$NetBSD: iris_machdep.h,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include "iris_scsivar.h"

/* iris_boot.c */
void again(void);
void reboot(void);

/* iris_parse.c */
void parse(char **, char *);

/* iris_autoconf.c */
void find_devs(void);

/* iris_start.S */
void romrestart(void);

 /* iris_cons.c */
char *cninit(int *, int *);
int   cngetc(void);
void  cnputc(int);
int   cnscan(void);

/* iris_scsi.c */
void wd33c93_init(void *, void*);
int wd33c93_go(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *, size_t *);

/* iris_scsictl.c */
int scsi_test_unit_rdy(void);
int scsi_read_capacity(uint8_t *, size_t);
int scsi_read(uint8_t *, size_t, daddr_t, size_t);
int scsi_write(uint8_t *, size_t, daddr_t, size_t);

#define INDIGO_R3K_MODE

#ifdef INDIGO_R3K_MODE
#define ZS_ADDR		0x1fb80d10
#define SCSIA_ADDR	0x1FB80122
#define SCSID_ADDR	0x1FB80126

/* Target is Personal IRIS R3000 36MHz. */
#define CPUSPEED	36
#endif

#ifdef INDIGO_R4K_MODE
#define ZS_ADDR		0x1fb80d10
#define SCSIA_ADDR	0x1FB80122
#define SCSID_ADDR	0x1FB80126

/* Target is IRIS Indigo R4000 100MHz. */
#define CPUSPEED	100
#endif

#ifdef INDY_MODE
#define ZS_ADDR		0x1fbd9830
#define SCSIA_ADDR	0x1FBC0003
#define SCSID_ADDR	0x1FBC0007

/* Target is Indy 180MHz. */
#define CPUSPEED	180
#endif

#define DELAY(n)							\
do {									\
        register int __N = (CPUSPEED) / 2 * n;				\
        do {								\
                __asm("addiu %0,%1,-1" : "=r" (__N) : "0" (__N));	\
        } while (__N > 0);						\
} while (/* CONSTCOND */ 0)
