/*	$NetBSD: adbvar.h,v 1.14.22.1 2012/11/20 03:01:31 tls Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/adbsys.h>

/*
 * Arguments used to attach a device to the Apple Desktop Bus
 */
struct adb_attach_args {
	int	origaddr;
	int	adbaddr;
	int	handler_id;
};

typedef struct adb_trace_xlate_s {
	int     params;
	char   *string;
}       adb_trace_xlate_t;

extern adb_trace_xlate_t adb_trace_xlations[];

#ifdef DEBUG
#ifndef ADB_DEBUG
#define ADB_DEBUG
#endif
#endif

#ifdef ADB_DEBUG
extern int	adb_debug;
#endif

typedef void *Ptr;
typedef void **Handle;

/* ADB Manager */
typedef struct {
	Ptr siServiceRtPtr;
	Ptr siDataAreaAddr;
} ADBSetInfoBlock;
typedef struct {
	unsigned char	devType;
	unsigned char	origADBAddr;
	Ptr		dbServiceRtPtr;
	Ptr		dbDataAreaAddr;
} ADBDataBlock;

struct adb_softc {
	char *sc_regbase;
};

typedef void (adbComp)(void *, volatile int *, int);

/* adb_direct.c */
extern int adbHardware;

/* types of adb hardware that we (will eventually) support */
#define ADB_HW_UNKNOWN		0x01	/* don't know */
#define ADB_HW_II		0x02	/* Mac II series */
#define ADB_HW_IISI		0x03	/* Mac IIsi series */
#define ADB_HW_PMU		0x04	/* PowerBook series */
#define ADB_HW_CUDA		0x05	/* Machines with a Cuda chip */

#define ADB_CMDADDR(cmd)	((u_int8_t)((cmd) & 0xf0) >> 4)
#define ADBFLUSH(dev)		((((u_int8_t)(dev) & 0x0f) << 4) | 0x01)
#define ADBLISTEN(dev, reg)	((((u_int8_t)(dev) & 0x0f) << 4) | 0x08 | (reg))
#define ADBTALK(dev, reg)	((((u_int8_t)(dev) & 0x0f) << 4) | 0x0c | (reg))

int	adb_intr(void *);
int	adb_poweroff(void);
void	adb_restart(void);
void	adb_cuda_autopoll(void);
int	adb_intr_cuda(void *);
int	CountADBs(void);
void	ADBReInit(void);
int	GetIndADB(ADBDataBlock * info, int index);
int	GetADBInfo(ADBDataBlock * info, int adbAddr);
int	SetADBInfo(ADBSetInfoBlock * info, int adbAddr);
int	ADBOp(Ptr buffer, adbComp *compRout, Ptr data, short commandNum);
int	adb_read_date_time(unsigned long *t);
int	adb_set_date_time(unsigned long t);

int	adb_op_sync(Ptr, adbComp *, Ptr, short);
void	adb_op_comprout(void *, volatile int *, int);
