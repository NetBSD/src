/* $NetBSD: mipssimreg.h,v 1.4 2022/12/28 11:50:25 he Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 *	Memory Map
 *
 *	0000.0000	128MB	RAM (max ~500MB)
 *	1fd0.0000	 64kB	ISA IO space
 *	1fd1.0000	 64kB	'ISA' VirtIO IO space (non standard)
 *
 *	Additionally, mips64 probes for memory up to 16G:
 *
 *	  2000.0000	memory, probed, up to
 *	4.0000.0000	16GB somewhat arbitrarily, could probably be higher
 *
 *	CPU interrupts
 *
 *	 0	mipsnet
 *	 1	virtio
 *	 2	16450 UART
 */

#define	MIPSSIM_UART0_ADDR	0x003f8
#define	MIPSSIM_MIPSNET0_ADDR	0x04200
#define MIPSSIM_VIRTIO_ADDR	0x10000

#define	MIPSSIM_ISA_IO_BASE	0x1fd00000  /* ISA IO memory:	*/
#define	MIPSSIM_ISA_IO_SIZE	0x00010000  /*    64 kByte	*/
#define MIPSSIM_VIRTIO_IO_SIZE	0x00010000  /*    64 kByte      */

#define MIPSSIM_DMA_BASE	0x00000000
#define MIPSSIM_DMA_PHYSBASE	0x00000000
#define MIPSSIM_DMA_SIZE	(MIPSSIM_ISA_IO_BASE - MIPSSIM_DMA_BASE)

#define MIPSSIM_MORE_MEM_BASE	0x20000000
#define MIPSSIM_MORE_MEM_END	(16ULL * 1024 * 1024 * 1024) /* 16GB */

#define VIRTIO_NUM_TRANSPORTS	32
#define VIRTIO_STRIDE		512
