/*	$NetBSD: sifbios.c,v 1.5 2005/06/26 19:57:30 he Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * PlayStation 2 SIF BIOS Version 2.0 interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sifbios.c,v 1.5 2005/06/26 19:57:30 he Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <playstation2/playstation2/sifbios.h>
#include <playstation2/playstation2/interrupt.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#define SIFBIOS_ENTRY_PTR	MIPS_PHYS_TO_KSEG0(0x00001000)
#define SIFBIOS_SIGNATURE_PTR	MIPS_PHYS_TO_KSEG1(0x00001004)
#define SIFBIOS_SIGNATURE	(('P' << 0)|('S' << 8)|('2' << 16)|('b' << 24))

STATIC int (*__sifbios_call)(int, void *);
#define CALL(t, n, a)	((t)(*__sifbios_call)((n), (void *)(a)))

STATIC void sifbios_rpc_callback(void *, int);
STATIC int sifbios_rpc_call(int, void *, int *);

void
sifbios_init()
{
	/* check BIOS exits */
	if (*(u_int32_t *)SIFBIOS_SIGNATURE_PTR != SIFBIOS_SIGNATURE)
		panic("SIFBIOS not found");
	
	__sifbios_call = *((int (**)(int, void*))SIFBIOS_ENTRY_PTR);
}

int
sifbios_rpc_call(int callno, void *arg, int *result)
{
	__volatile__ int done = 0;
	int retry;
	struct {
		int result;
		void *arg;
		void (*callback)(void *, int);
		__volatile__ void *callback_arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		arg:		arg,
		callback:	sifbios_rpc_callback,
		callback_arg:	(__volatile__ void *)&done,
	};
	
	/* call SIF BIOS */
	retry = 100;
	while (CALL(int, callno, &sifbios_arg) != 0 && --retry > 0)
		delay(20000);	/* .02 sec. for slow IOP */

	if (retry == 0) {
		printf("SIF BIOS call %d failed\n", callno);
		goto error;
	}

	/* wait IOP response (1 sec.) */
	_sif_call_start();
	retry = 10000;
	while (!done && --retry > 0)
		delay(100);
	_sif_call_end();

	if (retry == 0) {
		printf("IOP not respond (callno = %d)\n", callno);
		goto error;
	}
	
	*result = sifbios_arg.result;

	return (0);

 error:
	return (-1);
}

void
sifbios_rpc_callback(void *arg, int result)
{
	int *done = (int *)arg;
	
	*done = 1;
}

/*
 * System misc.
 */
int
sifbios_getver()
{

	return CALL(int, 0, 0);
}

void
sifbios_halt(int mode)
{
	int sifbios_arg = mode;

	CALL(void, 1, &sifbios_arg);
}

void
sifbios_setdve(int mode)
{
	int sifbios_arg = mode;

	CALL(void, 2, &sifbios_arg);
}

void
sifbios_putchar(int c)
{
	int sifbios_arg = c;

	CALL(void, 3, &sifbios_arg);
}

int
sifbios_getchar()
{

	return CALL(int, 4, 0);
}

/*
 * SIF DMA
 */
int
sifdma_init()
{

	return CALL(int, 16, 0);
}

void
sifdma_exit()
{

	CALL(void, 17, 0);
}

/* queue DMA request to SIFBIOS. returns queue identifier. */
sifdma_id_t
sifdma_queue(struct sifdma_transfer *arg, int n)
{
	struct {
		void *arg;	/* pointer to sifdma_transfer array */
		int n;		/* # of elements */
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		arg:	arg,
		n:	n
	};

	return CALL(sifdma_id_t, 18, &sifbios_arg);
}

/* 
 * status of DMA 
 *	>0 ... queued. not kicked.
 *	 0 ... DMA executing.
 *	<0 ... DMA done.
 */
int
sifdma_stat(sifdma_id_t id)
{
	u_int32_t sifbios_arg = id;

	return CALL(int, 19, &sifbios_arg);
}

/* reset DMA channel */
void
sifdma_reset()
{

	CALL(void, 20, 0);
}

/*
 * SIF CMD
 */
int
sifcmd_init()
{
	
	return CALL(int, 32, 0);
}

void
sifcmd_exit()
{

	CALL(void, 33, 0);
}

sifdma_id_t
sifcmd_queue(sifcmd_sw_t sw, vaddr_t cmd_pkt_addr, size_t cmd_pkt_sz,
    vaddr_t src_addr, vaddr_t dst_addr, vsize_t buf_sz)
{
	struct {
		sifcmd_sw_t sw;
		vaddr_t cmd_pkt_addr;	/* command buffer */
		size_t cmd_pkt_sz;
		vaddr_t src_addr;	/* data buffer */
		vaddr_t dst_addr;
		vsize_t buf_sz;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		sw:		sw,
		cmd_pkt_addr:	cmd_pkt_addr,
		cmd_pkt_sz:	cmd_pkt_sz,
		src_addr:	src_addr,
		dst_addr:	dst_addr,
		buf_sz:		buf_sz,
	};

	return CALL(sifdma_id_t, 34, &sifbios_arg);
}

/* interrupt handler of DMAC channel 5 (SIF0) */
int
sifcmd_intr(void *arg)
{

	CALL(void, 35, 0);

	return (1);
}

void
sifcmd_establish(sifcmd_sw_t sw, struct sifcmd_callback_holder *holder)
{
	struct {
		sifcmd_sw_t sw;
		sifcmd_callback_t func;
		void *arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		sw:	sw,
		func:	holder->func,
		arg:	holder->arg,
	};
	
	CALL(void, 36, &sifbios_arg);
}

void
sifcmd_disestablish(sifcmd_sw_t sw)
{
	u_int32_t sifbios_arg = sw;

	CALL(void, 37, &sifbios_arg);
}

struct sifcmd_callback_holder *
sifcmd_handler_init(struct sifcmd_callback_holder *holder, int n)
{
	struct {
		void *holder;
		int n;		/* # of slot */
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		holder:	holder,
		n:	n,
	};

	/* returns old holder */
	return CALL(struct sifcmd_callback_holder *, 38, &sifbios_arg);
}

/*
 * SIF RPC
 */
int
sifrpc_init()
{

	return CALL(int, 48, 0);
}

void
sifrpc_exit()
{

	CALL(void, 49, 0);
}

int
sifrpc_receive_buffer(struct sifrpc_receive *_cookie, void *src_iop,
    void *dst_ee, size_t sz, u_int32_t rpc_mode, void (*end_func)(void *),
    void *end_arg)
{
	struct {
		void *_cookie;
		void *src_iop;
		void *dst_ee;
		size_t sz;
		u_int32_t rpc_mode;
		sifrpc_endfunc_t end_func;
		void *end_arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		_cookie:	_cookie,
		src_iop:	src_iop,
		dst_ee:		dst_ee,
		sz:		sz,
		rpc_mode:	rpc_mode,
		end_func:	end_func,
		end_arg:	end_arg,
	};

	return CALL(int, 50, &sifbios_arg);
}

int
sifrpc_bind(struct sifrpc_client *_cookie, sifrpc_id_t rpc_id,
    u_int32_t rpc_mode, void (*end_func)(void *), void *end_arg)
{
	struct {
		void *_cookie;		/* filled by this call */
		sifrpc_id_t rpc_id;	/* specify server RPC id */
		u_int32_t rpc_mode;
		sifrpc_endfunc_t end_func;
		void *end_arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		_cookie:	_cookie,
		rpc_id:		rpc_id,
		rpc_mode:	rpc_mode,
		end_func:	end_func,
		end_arg:	end_arg,
	};

	return CALL(int, 51, &sifbios_arg);
}

int
sifrpc_call(struct sifrpc_client *_cookie, sifrpc_callno_t call_no,
    u_int32_t rpc_mode, void *sendbuf, size_t sendbuf_sz, void *recvbuf,
    size_t recvbuf_sz, void (*end_func)(void *), void *end_arg)
{
	struct {
		struct sifrpc_client *_cookie;	/* binded client cookie */
		sifrpc_callno_t call_no; /* passed to service function arg. */
		u_int32_t rpc_mode;
		void *sendbuf;
		size_t sendbuf_sz;
		void *recvbuf;
		size_t recvbuf_sz;
		sifrpc_endfunc_t end_func;
		void *end_arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		_cookie:	_cookie,
		call_no:	call_no,
		rpc_mode:	rpc_mode,
		sendbuf:	sendbuf,
		sendbuf_sz:	sendbuf_sz,
		recvbuf:	recvbuf,
		recvbuf_sz:	recvbuf_sz,
		end_func:	end_func,
		end_arg:	end_arg,
	};

	return CALL(int, 52, &sifbios_arg);
}

int
sifrpc_stat(struct sifrpc_header *_cookie)
{
	void *sifbios_arg = _cookie;

	return CALL(int, 53, &sifbios_arg);
}

void
sifrpc_establish(struct sifrpc_server_system *queue, void (*end_func)(void *),
    void *end_arg)
{
	struct {
		struct sifrpc_server_system *queue;
		sifrpc_endfunc_t end_func;
		void *end_arg;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		queue:		queue,
		end_func:	end_func,
		end_arg:	end_arg,
	};

	CALL(void, 54, &sifbios_arg);
}

void
sifrpc_register_service(struct sifrpc_server_system *queue,
    struct sifrpc_server *server, sifrpc_id_t rpc_id,
    void *(*service_func)(sifrpc_callno_t, void *, size_t), void *service_arg,
    void *(*cancel_func)(sifrpc_callno_t, void *, size_t), void *cancel_arg)
{
	struct {
		void *server;
		sifrpc_id_t rpc_id;
		sifrpc_rpcfunc_t service_func;
		void *service_arg;
		sifrpc_rpcfunc_t cancel_func;
		void *cancel_arg;
		void *receive_queue;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		server:		server,
		rpc_id:		rpc_id,
		service_func:	service_func,
		service_arg:	service_arg,
		cancel_func:	cancel_func,
		cancel_arg:	cancel_arg,
		receive_queue:	queue,
	};
	
	CALL(void, 55, &sifbios_arg);
}

void
sifrpc_unregister_service(struct sifrpc_server_system *queue,
    struct sifrpc_server *server)
{
	struct {
		void *server;
		void *receive_queue;
	} __attribute__((__packed__, __aligned__(4))) sifbios_arg = {
		server:		server,
		receive_queue:	queue,
	};

	CALL(void, 56, &sifbios_arg);
}

void
sifrpc_disestablish(struct sifrpc_server_system *queue)
{
	void *sifbios_arg = queue;

	CALL(void, 57, &sifbios_arg);
}

struct sifrpc_server *
sifrpc_dequeue(struct sifrpc_server_system *queue)
{
	void *sifbios_arg = queue;

	return CALL(struct sifrpc_server *, 58, &sifbios_arg);
}

void
sifrpc_dispatch_service(struct sifrpc_server *server)
{
	void *sifbios_arg = server;

	CALL(void, 59, &sifbios_arg);
}

/*
 * IOP memory
 */
int
iopmem_init()
{
	int result;

	sifbios_rpc_call(64, 0, &result);

	return (0);
}

paddr_t
iopmem_alloc(psize_t sz)
{
	int result, sifbios_arg = sz;

	if (sifbios_rpc_call(65, (void *)&sifbios_arg, &result) < 0)
		return (paddr_t)0;
	/* returns allocated IOP physical addr */
	return (paddr_t)result;
}

int
iopmem_free(paddr_t addr)
{
	int result, sifbios_arg = addr;

	sifbios_rpc_call(66, (void *)&sifbios_arg, &result);

	return (result);
}
