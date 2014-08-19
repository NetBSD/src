/*	$NetBSD: sifbios.h,v 1.5.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * PlayStation 2 SIF BIOS Version 2.0 define.
 */

/* SIF BIOS uses SIFDMA_BASE as emulated IOP physical memory base */
#define SIFDMA_BASE		MIPS_PHYS_TO_KSEG1(0x1c000000)
#define EEKV_TO_IOPPHYS(a)	((u_int32_t)(a) - SIFDMA_BASE)
#define IOPPHYS_TO_EEKV(a)	((u_int32_t)(a) + SIFDMA_BASE)

void sifbios_init(void);

/* 
 * System
 */
int sifbios_getver(void);
void sifbios_halt(int);
void sifbios_setdve(int);
void sifbios_putchar(int);
int sifbios_getchar(void);

/*
 * SIFDMA
 */
struct sifdma_transfer {
	vaddr_t src;	/* (EE) 16byte align */
	vaddr_t dst;	/* (IOP) 4byte align */
	vsize_t sz;	/* multiple of 16 */
	u_int32_t mode;
#define SIFDMA_MODE_NOINTR		0x0
#define SIFDMA_MODE_INTR_SENDER		0x2
#define SIFDMA_MODE_INTR_RECEIVER	0x4
};
typedef int sifdma_id_t;

int sifdma_init(void);
void sifdma_exit(void);
sifdma_id_t sifdma_queue(struct sifdma_transfer *, int);
int sifdma_stat(sifdma_id_t);
void sifdma_reset(void);

/*
 * SIFCMD
 */
typedef u_int32_t sifcmd_sw_t;
typedef void (*sifcmd_callback_t)(void *, void *);
struct sifcmd_callback_holder {
	sifcmd_callback_t func;
	void *arg;
} __attribute__((__packed__, __aligned__(4)));

int sifcmd_init(void);
void sifcmd_exit(void);
sifdma_id_t sifcmd_queue(sifcmd_sw_t, vaddr_t, size_t, vaddr_t, vaddr_t,
    vsize_t);
int sifcmd_intr(void *);
void sifcmd_establish(sifcmd_sw_t, struct sifcmd_callback_holder *);
void sifcmd_disestablish(sifcmd_sw_t);
struct sifcmd_callback_holder *sifcmd_handler_init(
	struct sifcmd_callback_holder *, int);

/*
 * SIFRPC
 */
#define SIFRPC_NOWAIT			1
#define SIFRPC_NOCACHEFLUSH		2

#define SIFRPC_ERROR_GETPACKET		1
#define SIFRPC_ERROR_SENDPACKET		2

/* RPC identifier */
typedef u_int32_t sifrpc_id_t;
/* service # */
typedef u_int32_t sifrpc_callno_t;
/* server service function */
typedef void *(*sifrpc_rpcfunc_t)(sifrpc_callno_t, void *, size_t);
/* callback when RPC service done. */
typedef void (*sifrpc_endfunc_t)(void *);

/* sifrpc common header */
struct sifrpc_header {
	paddr_t packet_paddr;
	u_int32_t packet_id;
	void *wait_channel;
	u_int32_t call_mode;
} __attribute__((__packed__, __aligned__(4)));

/* receive RPC call */
struct sifrpc_receive {
	struct sifrpc_header hdr;
	void *src;
	void *dst;
	size_t sz;
	sifrpc_endfunc_t end_callback;
	void *end_callback_arg;
} __attribute__((__packed__, __aligned__(4)));

/* client */
struct sifrpc_server;

struct sifrpc_client {
	struct sifrpc_header hdr;
	u_int32_t command;
	void *buf;
	void *cbuf;
	sifrpc_endfunc_t end_callback;
	void *end_callback_arg;
	struct sifrpc_server *server;
} __attribute__((__packed__, __aligned__(4)));

/* server */
struct sifrpc_server_queue;

struct sifrpc_server {
	u_int32_t command;
	sifrpc_rpcfunc_t service_func;
	void *buf;
	size_t buf_sz;
	void *cbuf;
	size_t cbuf_sz;
	struct sifrpc_client *client;
	void *paddr;
	sifrpc_callno_t callno;
	void *recv;
	size_t recv_sz;
	int recv_mode;
	u_int32_t rid;
	struct sifrpc_server *link;
	struct sifrpc_server *next;
	struct sifrpc_server_receive_queue *head;
} __attribute__((__packed__, __aligned__(4)));

/* server request queue */
struct sifrpc_server_system {
	int active;
	struct sifrpc_server *link;
	struct sifrpc_server *head;
	struct sifrpc_server *last;
	struct sifrpc_server *next;
	void *wait_channel;
	sifrpc_endfunc_t end_callback;
	void *end_callback_arg;
} __attribute__((__packed__, __aligned__(4)));

int sifrpc_init(void);
void sifrpc_exit(void);
/*
 * Server side
 */
/* register/unregister receive queue to RPC system */
void sifrpc_establish(struct sifrpc_server_system *, sifrpc_endfunc_t,
    void *);
void sifrpc_disestablish(struct sifrpc_server_system *);
/* queue access */
struct sifrpc_server *sifrpc_dequeue(struct sifrpc_server_system *);

/* register/unregister service function to receive queue */
void sifrpc_register_service(struct sifrpc_server_system *,
    struct sifrpc_server *, sifrpc_id_t, sifrpc_rpcfunc_t, void *,
    sifrpc_rpcfunc_t, void *);
void sifrpc_unregister_service(struct sifrpc_server_system *,
    struct sifrpc_server *);
/* execute service */
void sifrpc_dispatch_service(struct sifrpc_server *);

/*
 * Client side
 */
int sifrpc_bind(struct sifrpc_client *, sifrpc_id_t, u_int32_t,
    sifrpc_endfunc_t, void *);
int sifrpc_call(struct sifrpc_client *, sifrpc_callno_t, u_int32_t,
    void *, size_t, void *, size_t, sifrpc_endfunc_t, void *);

/* simple transfer API */
int sifrpc_receive_buffer(struct sifrpc_receive *, void *, void *, size_t,
    u_int32_t, sifrpc_endfunc_t, void *);

/* check DMA state */
int sifrpc_stat(struct sifrpc_header *);

/*
 * IOP memory management
 */
int iopmem_init(void);
paddr_t iopmem_alloc(psize_t);
int iopmem_free(paddr_t);

