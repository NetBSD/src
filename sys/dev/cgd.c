/* $NetBSD: cgd.c,v 1.145 2022/04/01 21:09:24 pgoyette Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgd.c,v 1.145 2022/04/01 21:09:24 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/namei.h> /* for pathbuf */
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/workqueue.h>

#include <dev/cgd_crypto.h>
#include <dev/cgdvar.h>
#include <dev/dkvar.h>

#include <miscfs/specfs/specdev.h> /* for v_rdev */

#include "ioconf.h"

struct selftest_params {
	const char *alg;
	int encblkno8;
	int blocksize;	/* number of bytes */
	int secsize;
	daddr_t blkno;
	int keylen;	/* number of bits */
	int txtlen;	/* number of bytes */
	const uint8_t *key;
	const uint8_t *ptxt;
	const uint8_t *ctxt;
};

/* Entry Point Functions */

static dev_type_open(cgdopen);
static dev_type_close(cgdclose);
static dev_type_read(cgdread);
static dev_type_write(cgdwrite);
static dev_type_ioctl(cgdioctl);
static dev_type_strategy(cgdstrategy);
static dev_type_dump(cgddump);
static dev_type_size(cgdsize);

const struct bdevsw cgd_bdevsw = {
	.d_open = cgdopen,
	.d_close = cgdclose,
	.d_strategy = cgdstrategy,
	.d_ioctl = cgdioctl,
	.d_dump = cgddump,
	.d_psize = cgdsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE
};

const struct cdevsw cgd_cdevsw = {
	.d_open = cgdopen,
	.d_close = cgdclose,
	.d_read = cgdread,
	.d_write = cgdwrite,
	.d_ioctl = cgdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE
};

/*
 * Vector 5 from IEEE 1619/D16 truncated to 64 bytes, blkno 1.
 */
static const uint8_t selftest_aes_xts_256_ptxt[64] = {
	0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
	0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
	0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
	0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
	0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
	0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
	0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
	0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12,
};

static const uint8_t selftest_aes_xts_256_ctxt[512] = {
	0x26, 0x4d, 0x3c, 0xa8, 0x51, 0x21, 0x94, 0xfe,
	0xc3, 0x12, 0xc8, 0xc9, 0x89, 0x1f, 0x27, 0x9f,
	0xef, 0xdd, 0x60, 0x8d, 0x0c, 0x02, 0x7b, 0x60,
	0x48, 0x3a, 0x3f, 0xa8, 0x11, 0xd6, 0x5e, 0xe5,
	0x9d, 0x52, 0xd9, 0xe4, 0x0e, 0xc5, 0x67, 0x2d,
	0x81, 0x53, 0x2b, 0x38, 0xb6, 0xb0, 0x89, 0xce,
	0x95, 0x1f, 0x0f, 0x9c, 0x35, 0x59, 0x0b, 0x8b,
	0x97, 0x8d, 0x17, 0x52, 0x13, 0xf3, 0x29, 0xbb,
};

static const uint8_t selftest_aes_xts_256_key[33] = {
	0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
	0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26,
	0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
	0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95,
	0
};

/*
 * Vector 11 from IEEE 1619/D16 truncated to 64 bytes, blkno 0xffff.
 */
static const uint8_t selftest_aes_xts_512_ptxt[64] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

static const uint8_t selftest_aes_xts_512_ctxt[64] = {
	0x77, 0xa3, 0x12, 0x51, 0x61, 0x8a, 0x15, 0xe6,
	0xb9, 0x2d, 0x1d, 0x66, 0xdf, 0xfe, 0x7b, 0x50,
	0xb5, 0x0b, 0xad, 0x55, 0x23, 0x05, 0xba, 0x02,
	0x17, 0xa6, 0x10, 0x68, 0x8e, 0xff, 0x7e, 0x11,
	0xe1, 0xd0, 0x22, 0x54, 0x38, 0xe0, 0x93, 0x24,
	0x2d, 0x6d, 0xb2, 0x74, 0xfd, 0xe8, 0x01, 0xd4,
	0xca, 0xe0, 0x6f, 0x20, 0x92, 0xc7, 0x28, 0xb2,
	0x47, 0x85, 0x59, 0xdf, 0x58, 0xe8, 0x37, 0xc2,
};

static const uint8_t selftest_aes_xts_512_key[65] = {
	0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
	0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26,
	0x62, 0x49, 0x77, 0x57, 0x24, 0x70, 0x93, 0x69,
	0x99, 0x59, 0x57, 0x49, 0x66, 0x96, 0x76, 0x27,
	0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
	0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95,
	0x02, 0x88, 0x41, 0x97, 0x16, 0x93, 0x99, 0x37,
	0x51, 0x05, 0x82, 0x09, 0x74, 0x94, 0x45, 0x92,
	0
};

static const uint8_t selftest_aes_cbc_key[32] = {
	0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
	0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26,
	0x62, 0x49, 0x77, 0x57, 0x24, 0x70, 0x93, 0x69,
	0x99, 0x59, 0x57, 0x49, 0x66, 0x96, 0x76, 0x27,
};

static const uint8_t selftest_aes_cbc_128_ptxt[64] = {
	0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
	0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
	0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
	0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
	0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
	0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
	0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
	0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12,
};

static const uint8_t selftest_aes_cbc_128_ctxt[64] = { /* blkno=1 */
	0x93, 0x94, 0x56, 0x36, 0x83, 0xbc, 0xff, 0xa4,
	0xe0, 0x24, 0x34, 0x12, 0xbe, 0xfa, 0xb0, 0x7d,
	0x88, 0x1e, 0xc5, 0x57, 0x55, 0x23, 0x05, 0x0c,
	0x69, 0xa5, 0xc1, 0xda, 0x64, 0xee, 0x74, 0x10,
	0xc2, 0xc5, 0xe6, 0x66, 0xd6, 0xa7, 0x49, 0x1c,
	0x9d, 0x40, 0xb5, 0x0c, 0x9b, 0x6e, 0x1c, 0xe6,
	0xb1, 0x7a, 0x1c, 0xe7, 0x5a, 0xfe, 0xf9, 0x2a,
	0x78, 0xfa, 0xb7, 0x7b, 0x08, 0xdf, 0x8e, 0x51,
};

static const uint8_t selftest_aes_cbc_256_ptxt[64] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

static const uint8_t selftest_aes_cbc_256_ctxt[64] = { /* blkno=0xffff */
	0x6c, 0xa3, 0x15, 0x17, 0x51, 0x90, 0xe9, 0x69,
	0x08, 0x36, 0x7b, 0xa6, 0xbb, 0xd1, 0x0b, 0x9e,
	0xcd, 0x6b, 0x1e, 0xaf, 0xb6, 0x2e, 0x62, 0x7d,
	0x8e, 0xde, 0xf0, 0xed, 0x0d, 0x44, 0xe7, 0x31,
	0x26, 0xcf, 0xd5, 0x0b, 0x3e, 0x95, 0x59, 0x89,
	0xdf, 0x5d, 0xd6, 0x9a, 0x00, 0x66, 0xcc, 0x7f,
	0x45, 0xd3, 0x06, 0x58, 0xed, 0xef, 0x49, 0x47,
	0x87, 0x89, 0x17, 0x7d, 0x08, 0x56, 0x50, 0xe1,
};

static const uint8_t selftest_3des_cbc_key[24] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
};

static const uint8_t selftest_3des_cbc_ptxt[64] = {
	0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
	0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
	0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
	0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
	0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
	0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
	0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
	0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12,
};

static const uint8_t selftest_3des_cbc_ctxt[64] = {
	0xa2, 0xfe, 0x81, 0xaa, 0x10, 0x6c, 0xea, 0xb9,
	0x11, 0x58, 0x1f, 0x29, 0xb5, 0x86, 0x71, 0x56,
	0xe9, 0x25, 0x1d, 0x07, 0xb1, 0x69, 0x59, 0x6c,
	0x96, 0x80, 0xf7, 0x54, 0x38, 0xaa, 0xa7, 0xe4,
	0xe8, 0x81, 0xf5, 0x00, 0xbb, 0x1c, 0x00, 0x3c,
	0xba, 0x38, 0x45, 0x97, 0x4c, 0xcf, 0x84, 0x14,
	0x46, 0x86, 0xd9, 0xf4, 0xc5, 0xe2, 0xf0, 0x54,
	0xde, 0x41, 0xf6, 0xa1, 0xef, 0x1b, 0x0a, 0xea,
};

static const uint8_t selftest_bf_cbc_key[56] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
};

static const uint8_t selftest_bf_cbc_ptxt[64] = {
	0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
	0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
	0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
	0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
	0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
	0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
	0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
	0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12,
};

static const uint8_t selftest_bf_cbc_ctxt[64] = {
	0xec, 0xa2, 0xc0, 0x0e, 0xa9, 0x7f, 0x04, 0x1e,
	0x2e, 0x4f, 0x64, 0x07, 0x67, 0x3e, 0xf4, 0x58,
	0x61, 0x5f, 0xd3, 0x50, 0x5e, 0xd3, 0x4d, 0x34,
	0xa0, 0x53, 0xbe, 0x47, 0x75, 0x69, 0x3b, 0x1f,
	0x86, 0xf2, 0xae, 0x8b, 0xb7, 0x91, 0xda, 0xd4,
	0x2b, 0xa5, 0x47, 0x9b, 0x7d, 0x13, 0x30, 0xdd,
	0x7b, 0xad, 0x86, 0x57, 0x51, 0x11, 0x74, 0x42,
	0xb8, 0xbf, 0x69, 0x17, 0x20, 0x0a, 0xf7, 0xda,
};

static const uint8_t selftest_aes_cbc_encblkno8_zero64[64];
static const uint8_t selftest_aes_cbc_encblkno8_ctxt[64] = {
	0xa2, 0x06, 0x26, 0x26, 0xac, 0xdc, 0xe7, 0xcf,
	0x47, 0x68, 0x24, 0x0e, 0xfa, 0x40, 0x44, 0x83,
	0x07, 0xe1, 0xf4, 0x5d, 0x53, 0x47, 0xa0, 0xfe,
	0xc0, 0x6e, 0x4e, 0xf8, 0x9d, 0x98, 0x63, 0xb8,
	0x2c, 0x27, 0xfa, 0x3a, 0xd5, 0x40, 0xda, 0xdb,
	0xe6, 0xc3, 0xe4, 0xfb, 0x85, 0x53, 0xfb, 0x78,
	0x5d, 0xbd, 0x8f, 0x4c, 0x1a, 0x04, 0x9c, 0x88,
	0x85, 0xec, 0x3c, 0x56, 0x46, 0x1a, 0x6e, 0xf5,
};

const struct selftest_params selftests[] = {
	{
		.alg = "aes-xts",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 1,
		.keylen = 256,
		.txtlen = sizeof(selftest_aes_xts_256_ptxt),
		.key  = selftest_aes_xts_256_key,
		.ptxt = selftest_aes_xts_256_ptxt,
		.ctxt = selftest_aes_xts_256_ctxt
	},
	{
		.alg = "aes-xts",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 0xffff,
		.keylen = 512,
		.txtlen = sizeof(selftest_aes_xts_512_ptxt),
		.key  = selftest_aes_xts_512_key,
		.ptxt = selftest_aes_xts_512_ptxt,
		.ctxt = selftest_aes_xts_512_ctxt
	},
	{
		.alg = "aes-cbc",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 1,
		.keylen = 128,
		.txtlen = sizeof(selftest_aes_cbc_128_ptxt),
		.key  = selftest_aes_cbc_key,
		.ptxt = selftest_aes_cbc_128_ptxt,
		.ctxt = selftest_aes_cbc_128_ctxt,
	},
	{
		.alg = "aes-cbc",
		.blocksize = 16,
		.secsize = 512,
		.blkno = 0xffff,
		.keylen = 256,
		.txtlen = sizeof(selftest_aes_cbc_256_ptxt),
		.key  = selftest_aes_cbc_key,
		.ptxt = selftest_aes_cbc_256_ptxt,
		.ctxt = selftest_aes_cbc_256_ctxt,
	},
	{
		.alg = "3des-cbc",
		.blocksize = 8,
		.secsize = 512,
		.blkno = 1,
		.keylen = 192,	/* 168 + 3*8 parity bits */
		.txtlen = sizeof(selftest_3des_cbc_ptxt),
		.key  = selftest_3des_cbc_key,
		.ptxt = selftest_3des_cbc_ptxt,
		.ctxt = selftest_3des_cbc_ctxt,
	},
	{
		.alg = "blowfish-cbc",
		.blocksize = 8,
		.secsize = 512,
		.blkno = 1,
		.keylen = 448,
		.txtlen = sizeof(selftest_bf_cbc_ptxt),
		.key  = selftest_bf_cbc_key,
		.ptxt = selftest_bf_cbc_ptxt,
		.ctxt = selftest_bf_cbc_ctxt,
	},
	{
		.alg = "aes-cbc",
		.encblkno8 = 1,
		.blocksize = 16,
		.secsize = 512,
		.blkno = 0,
		.keylen = 128,
		.txtlen = sizeof(selftest_aes_cbc_encblkno8_zero64),
		.key = selftest_aes_cbc_encblkno8_zero64,
		.ptxt = selftest_aes_cbc_encblkno8_zero64,
		.ctxt = selftest_aes_cbc_encblkno8_ctxt,
	},
};

static int cgd_match(device_t, cfdata_t, void *);
static void cgd_attach(device_t, device_t, void *);
static int cgd_detach(device_t, int);
static struct cgd_softc	*cgd_spawn(int);
static struct cgd_worker *cgd_create_one_worker(void);
static void cgd_destroy_one_worker(struct cgd_worker *);
static struct cgd_worker *cgd_create_worker(void);
static void cgd_destroy_worker(struct cgd_worker *);
static int cgd_destroy(device_t);

/* Internal Functions */

static int	cgd_diskstart(device_t, struct buf *);
static void	cgd_diskstart2(struct cgd_softc *, struct cgd_xfer *);
static void	cgdiodone(struct buf *);
static void	cgd_iodone2(struct cgd_softc *, struct cgd_xfer *);
static void	cgd_enqueue(struct cgd_softc *, struct cgd_xfer *);
static void	cgd_process(struct work *, void *);
static int	cgd_dumpblocks(device_t, void *, daddr_t, int);

static int	cgd_ioctl_set(struct cgd_softc *, void *, struct lwp *);
static int	cgd_ioctl_clr(struct cgd_softc *, struct lwp *);
static int	cgd_ioctl_get(dev_t, void *, struct lwp *);
static int	cgdinit(struct cgd_softc *, const char *, struct vnode *,
			struct lwp *);
static void	cgd_cipher(struct cgd_softc *, void *, const void *,
			   size_t, daddr_t, size_t, int);

static void	cgd_selftest(void);

static const struct dkdriver cgddkdriver = {
        .d_minphys  = minphys,
        .d_open = cgdopen,
        .d_close = cgdclose,
        .d_strategy = cgdstrategy,
        .d_iosize = NULL,
        .d_diskstart = cgd_diskstart,
        .d_dumpblocks = cgd_dumpblocks,
        .d_lastclose = NULL
};

CFATTACH_DECL3_NEW(cgd, sizeof(struct cgd_softc),
    cgd_match, cgd_attach, cgd_detach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

/* DIAGNOSTIC and DEBUG definitions */

#if defined(CGDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
int cgddebug = 0;

#define CGDB_FOLLOW	0x1
#define CGDB_IO	0x2
#define CGDB_CRYPTO	0x4

#define IFDEBUG(x,y)		if (cgddebug & (x)) y
#define DPRINTF(x,y)		IFDEBUG(x, printf y)
#define DPRINTF_FOLLOW(y)	DPRINTF(CGDB_FOLLOW, y)

static void	hexprint(const char *, void *, int);

#else
#define IFDEBUG(x,y)
#define DPRINTF(x,y)
#define DPRINTF_FOLLOW(y)
#endif

/* Global variables */

static kmutex_t cgd_spawning_mtx;
static kcondvar_t cgd_spawning_cv;
static bool cgd_spawning;
static struct cgd_worker *cgd_worker;
static u_int cgd_refcnt;	/* number of users of cgd_worker */

/* Utility Functions */

#define CGDUNIT(x)		DISKUNIT(x)

/* The code */

static int
cgd_lock(bool intr)
{
	int error = 0;

	mutex_enter(&cgd_spawning_mtx);
	while (cgd_spawning) {
		if (intr)
			error = cv_wait_sig(&cgd_spawning_cv, &cgd_spawning_mtx);
		else
			cv_wait(&cgd_spawning_cv, &cgd_spawning_mtx);
	}
	if (error == 0)
		cgd_spawning = true;
	mutex_exit(&cgd_spawning_mtx);
	return error;
}

static void
cgd_unlock(void)
{
	mutex_enter(&cgd_spawning_mtx);
	cgd_spawning = false;
	cv_broadcast(&cgd_spawning_cv);
	mutex_exit(&cgd_spawning_mtx);
}

static struct cgd_softc *
getcgd_softc(dev_t dev)
{
	return device_lookup_private(&cgd_cd, CGDUNIT(dev));
}

static int
cgd_match(device_t self, cfdata_t cfdata, void *aux)
{

	return 1;
}

static void
cgd_attach(device_t parent, device_t self, void *aux)
{
	struct cgd_softc *sc = device_private(self);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_cv, "cgdcv");
	dk_init(&sc->sc_dksc, self, DKTYPE_CGD);
	disk_init(&sc->sc_dksc.sc_dkdev, sc->sc_dksc.sc_xname, &cgddkdriver);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self,
		    "unable to register power management hooks\n");
}


static int
cgd_detach(device_t self, int flags)
{
	int ret;
	struct cgd_softc *sc = device_private(self);
	struct dk_softc *dksc = &sc->sc_dksc;

	if (DK_BUSY(dksc, 0))
		return EBUSY;

	if (DK_ATTACHED(dksc) &&
	    (ret = cgd_ioctl_clr(sc, curlwp)) != 0)
		return ret;

	disk_destroy(&dksc->sc_dkdev);
	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

void
cgdattach(int num)
{
#ifndef _MODULE
	int error;

	mutex_init(&cgd_spawning_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&cgd_spawning_cv, "cgspwn");

	error = config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
	if (error != 0)
		aprint_error("%s: unable to register cfattach\n",
		    cgd_cd.cd_name);
#endif

	cgd_selftest();
}

static struct cgd_softc *
cgd_spawn(int unit)
{
	cfdata_t cf;
	struct cgd_worker *cw;
	struct cgd_softc *sc;

	cf = kmem_alloc(sizeof(*cf), KM_SLEEP);
	cf->cf_name = cgd_cd.cd_name;
	cf->cf_atname = cgd_cd.cd_name;
	cf->cf_unit = unit;
	cf->cf_fstate = FSTATE_STAR;

	cw = cgd_create_one_worker();
	if (cw == NULL) {
		kmem_free(cf, sizeof(*cf));
		return NULL;
	}

	sc = device_private(config_attach_pseudo(cf));
	if (sc == NULL) {
		cgd_destroy_one_worker(cw);
		return NULL;
	}

	sc->sc_worker = cw;

	return sc;
}

static int
cgd_destroy(device_t dev)
{
	struct cgd_softc *sc = device_private(dev);
	struct cgd_worker *cw = sc->sc_worker;
	cfdata_t cf;
	int error;

	cf = device_cfdata(dev);
	error = config_detach(dev, DETACH_QUIET);
	if (error)
		return error;

	cgd_destroy_one_worker(cw);

	kmem_free(cf, sizeof(*cf));
	return 0;
}

static void
cgd_busy(struct cgd_softc *sc)
{

	mutex_enter(&sc->sc_lock);
	while (sc->sc_busy)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	sc->sc_busy = true;
	mutex_exit(&sc->sc_lock);
}

static void
cgd_unbusy(struct cgd_softc *sc)
{

	mutex_enter(&sc->sc_lock);
	sc->sc_busy = false;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
}

static struct cgd_worker *
cgd_create_one_worker(void)
{
	KASSERT(cgd_spawning);

	if (cgd_refcnt++ == 0) {
		KASSERT(cgd_worker == NULL);
		cgd_worker = cgd_create_worker();
	}

	KASSERT(cgd_worker != NULL);
	return cgd_worker;
}

static void
cgd_destroy_one_worker(struct cgd_worker *cw)
{
	KASSERT(cgd_spawning);
	KASSERT(cw == cgd_worker);

	if (--cgd_refcnt == 0) {
		cgd_destroy_worker(cgd_worker);
		cgd_worker = NULL;
	}
}

static struct cgd_worker *
cgd_create_worker(void)
{
	struct cgd_worker *cw;
	struct workqueue *wq;
	struct pool *cp;
	int error;

	cw = kmem_alloc(sizeof(struct cgd_worker), KM_SLEEP);
	cp = kmem_alloc(sizeof(struct pool), KM_SLEEP);

	error = workqueue_create(&wq, "cgd", cgd_process, NULL,
	    PRI_BIO, IPL_BIO, WQ_FPU|WQ_MPSAFE|WQ_PERCPU);
	if (error) {
		kmem_free(cp, sizeof(struct pool));
		kmem_free(cw, sizeof(struct cgd_worker));
		return NULL;
	}

	cw->cw_cpool = cp;
	cw->cw_wq = wq;
	pool_init(cw->cw_cpool, sizeof(struct cgd_xfer), 0,
	    0, 0, "cgdcpl", NULL, IPL_BIO);
	mutex_init(&cw->cw_lock, MUTEX_DEFAULT, IPL_BIO);

	return cw;
}

static void
cgd_destroy_worker(struct cgd_worker *cw)
{

	/*
	 * Wait for all worker threads to complete before destroying
	 * the rest of the cgd_worker.
	 */
	if (cw->cw_wq)
		workqueue_destroy(cw->cw_wq);

	mutex_destroy(&cw->cw_lock);

	if (cw->cw_cpool) {
		pool_destroy(cw->cw_cpool);
		kmem_free(cw->cw_cpool, sizeof(struct pool));
	}

	kmem_free(cw, sizeof(struct cgd_worker));
}

static int
cgdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	cgd_softc *sc;
	int error;

	DPRINTF_FOLLOW(("cgdopen(0x%"PRIx64", %d)\n", dev, flags));

	error = cgd_lock(true);
	if (error)
		return error;
	sc = getcgd_softc(dev);
	if (sc == NULL)
		sc = cgd_spawn(CGDUNIT(dev));
	cgd_unlock();
	if (sc == NULL)
		return ENXIO;

	return dk_open(&sc->sc_dksc, dev, flags, fmt, l);
}

static int
cgdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	cgd_softc *sc;
	struct	dk_softc *dksc;
	int error;

	DPRINTF_FOLLOW(("cgdclose(0x%"PRIx64", %d)\n", dev, flags));

	error = cgd_lock(false);
	if (error)
		return error;
	sc = getcgd_softc(dev);
	if (sc == NULL) {
		error = ENXIO;
		goto done;
	}

	dksc = &sc->sc_dksc;
	if ((error =  dk_close(dksc, dev, flags, fmt, l)) != 0)
		goto done;

	if (!DK_ATTACHED(dksc)) {
		if ((error = cgd_destroy(sc->sc_dksc.sc_dev)) != 0) {
			device_printf(dksc->sc_dev,
			    "unable to detach instance\n");
			goto done;
		}
	}

done:
	cgd_unlock();

	return error;
}

static void
cgdstrategy(struct buf *bp)
{
	struct	cgd_softc *sc = getcgd_softc(bp->b_dev);

	DPRINTF_FOLLOW(("cgdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));

	/*
	 * Reject unaligned writes.
	 */
	if (((uintptr_t)bp->b_data & 3) != 0) {
		bp->b_error = EINVAL;
		goto bail;
	}

	dk_strategy(&sc->sc_dksc, bp);
	return;

bail:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
}

static int
cgdsize(dev_t dev)
{
	struct cgd_softc *sc = getcgd_softc(dev);

	DPRINTF_FOLLOW(("cgdsize(0x%"PRIx64")\n", dev));
	if (!sc)
		return -1;
	return dk_size(&sc->sc_dksc, dev);
}

/*
 * cgd_{get,put}data are functions that deal with getting a buffer
 * for the new encrypted data.
 * We can no longer have a buffer per device, we need a buffer per
 * work queue...
 */

static void *
cgd_getdata(struct cgd_softc *sc, unsigned long size)
{
	void *data = NULL;

	mutex_enter(&sc->sc_lock);
	if (!sc->sc_data_used) {
		sc->sc_data_used = true;
		data = sc->sc_data;
	}
	mutex_exit(&sc->sc_lock);

	if (data)
		return data;

	return kmem_intr_alloc(size, KM_NOSLEEP);
}

static void
cgd_putdata(struct cgd_softc *sc, void *data, unsigned long size)
{

	if (data == sc->sc_data) {
		mutex_enter(&sc->sc_lock);
		sc->sc_data_used = false;
		mutex_exit(&sc->sc_lock);
	} else
		kmem_intr_free(data, size);
}

static int
cgd_diskstart(device_t dev, struct buf *bp)
{
	struct	cgd_softc *sc = device_private(dev);
	struct	cgd_worker *cw = sc->sc_worker;
	struct	dk_softc *dksc = &sc->sc_dksc;
	struct	disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	struct	cgd_xfer *cx;
	struct	buf *nbp;
	void *	newaddr;
	daddr_t	bn;

	DPRINTF_FOLLOW(("cgd_diskstart(%p, %p)\n", dksc, bp));

	bn = bp->b_rawblkno;

	/*
	 * We attempt to allocate all of our resources up front, so that
	 * we can fail quickly if they are unavailable.
	 */
	nbp = getiobuf(sc->sc_tvn, false);
	if (nbp == NULL)
		return EAGAIN;

	cx = pool_get(cw->cw_cpool, PR_NOWAIT);
	if (cx == NULL) {
		putiobuf(nbp);
		return EAGAIN;
	}

	cx->cx_sc = sc;
	cx->cx_obp = bp;
	cx->cx_nbp = nbp;
	cx->cx_srcv = cx->cx_dstv = bp->b_data;
	cx->cx_blkno = bn;
	cx->cx_secsize = dg->dg_secsize;

	/*
	 * If we are writing, then we need to encrypt the outgoing
	 * block into a new block of memory.
	 */
	if ((bp->b_flags & B_READ) == 0) {
		newaddr = cgd_getdata(sc, bp->b_bcount);
		if (!newaddr) {
			pool_put(cw->cw_cpool, cx);
			putiobuf(nbp);
			return EAGAIN;
		}

		cx->cx_dstv = newaddr;
		cx->cx_len = bp->b_bcount;
		cx->cx_dir = CGD_CIPHER_ENCRYPT;

		cgd_enqueue(sc, cx);
		return 0;
	}

	cgd_diskstart2(sc, cx);
	return 0;
}

static void
cgd_diskstart2(struct cgd_softc *sc, struct cgd_xfer *cx)
{
	struct	vnode *vp;
	struct	buf *bp;
	struct	buf *nbp;

	bp = cx->cx_obp;
	nbp = cx->cx_nbp;

	nbp->b_data = cx->cx_dstv;
	nbp->b_flags = bp->b_flags;
	nbp->b_oflags = bp->b_oflags;
	nbp->b_cflags = bp->b_cflags;
	nbp->b_iodone = cgdiodone;
	nbp->b_proc = bp->b_proc;
	nbp->b_blkno = btodb(cx->cx_blkno * cx->cx_secsize);
	nbp->b_bcount = bp->b_bcount;
	nbp->b_private = cx;

	BIO_COPYPRIO(nbp, bp);

	if ((nbp->b_flags & B_READ) == 0) {
		vp = nbp->b_vp;
		mutex_enter(vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(vp->v_interlock);
	}
	VOP_STRATEGY(sc->sc_tvn, nbp);
}

static void
cgdiodone(struct buf *nbp)
{
	struct	cgd_xfer *cx = nbp->b_private;
	struct	buf *obp = cx->cx_obp;
	struct	cgd_softc *sc = getcgd_softc(obp->b_dev);
	struct	dk_softc *dksc = &sc->sc_dksc;
	struct	disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	daddr_t	bn;

	KDASSERT(sc);

	DPRINTF_FOLLOW(("cgdiodone(%p)\n", nbp));
	DPRINTF(CGDB_IO, ("cgdiodone: bp %p bcount %d resid %d\n",
	    obp, obp->b_bcount, obp->b_resid));
	DPRINTF(CGDB_IO, (" dev 0x%"PRIx64", nbp %p bn %" PRId64
	    " addr %p bcnt %d\n", nbp->b_dev, nbp, nbp->b_blkno, nbp->b_data,
		nbp->b_bcount));
	if (nbp->b_error != 0) {
		obp->b_error = nbp->b_error;
		DPRINTF(CGDB_IO, ("%s: error %d\n", dksc->sc_xname,
		    obp->b_error));
	}

	/* Perform the decryption if we are reading.
	 *
	 * Note: use the blocknumber from nbp, since it is what
	 *       we used to encrypt the blocks.
	 */

	if (nbp->b_flags & B_READ) {
		bn = dbtob(nbp->b_blkno) / dg->dg_secsize;

		cx->cx_obp     = obp;
		cx->cx_nbp     = nbp;
		cx->cx_dstv    = obp->b_data;
		cx->cx_srcv    = obp->b_data;
		cx->cx_len     = obp->b_bcount;
		cx->cx_blkno   = bn;
		cx->cx_secsize = dg->dg_secsize;
		cx->cx_dir     = CGD_CIPHER_DECRYPT;

		cgd_enqueue(sc, cx);
		return;
	}

	cgd_iodone2(sc, cx);
}

static void
cgd_iodone2(struct cgd_softc *sc, struct cgd_xfer *cx)
{
	struct cgd_worker *cw = sc->sc_worker;
	struct buf *obp = cx->cx_obp;
	struct buf *nbp = cx->cx_nbp;
	struct dk_softc *dksc = &sc->sc_dksc;

	pool_put(cw->cw_cpool, cx);

	/* If we allocated memory, free it now... */
	if (nbp->b_data != obp->b_data)
		cgd_putdata(sc, nbp->b_data, nbp->b_bcount);

	putiobuf(nbp);

	/* Request is complete for whatever reason */
	obp->b_resid = 0;
	if (obp->b_error != 0)
		obp->b_resid = obp->b_bcount;

	dk_done(dksc, obp);
	dk_start(dksc, NULL);
}

static int
cgd_dumpblocks(device_t dev, void *va, daddr_t blkno, int nblk)
{
	struct cgd_softc *sc = device_private(dev);
	struct dk_softc *dksc = &sc->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	size_t nbytes, blksize;
	void *buf;
	int error;

	/*
	 * dk_dump gives us units of disklabel sectors.  Everything
	 * else in cgd uses units of diskgeom sectors.  These had
	 * better agree; otherwise we need to figure out how to convert
	 * between them.
	 */
	KASSERTMSG((dg->dg_secsize == dksc->sc_dkdev.dk_label->d_secsize),
	    "diskgeom secsize %"PRIu32" != disklabel secsize %"PRIu32,
	    dg->dg_secsize, dksc->sc_dkdev.dk_label->d_secsize);
	blksize = dg->dg_secsize;

	/*
	 * Compute the number of bytes in this request, which dk_dump
	 * has `helpfully' converted to a number of blocks for us.
	 */
	nbytes = nblk*blksize;

	/* Try to acquire a buffer to store the ciphertext.  */
	buf = cgd_getdata(sc, nbytes);
	if (buf == NULL)
		/* Out of memory: give up.  */
		return ENOMEM;

	/* Encrypt the caller's data into the temporary buffer.  */
	cgd_cipher(sc, buf, va, nbytes, blkno, blksize, CGD_CIPHER_ENCRYPT);

	/* Pass it on to the underlying disk device.  */
	error = bdev_dump(sc->sc_tdev, blkno, buf, nbytes);

	/* Release the buffer.  */
	cgd_putdata(sc, buf, nbytes);

	/* Return any error from the underlying disk device.  */
	return error;
}

/* XXX: we should probably put these into dksubr.c, mostly */
static int
cgdread(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *sc;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdread(0x%llx, %p, %d)\n",
	    (unsigned long long)dev, uio, flags));
	sc = getcgd_softc(dev);
	if (sc == NULL)
		return ENXIO;
	dksc = &sc->sc_dksc;
	if (!DK_ATTACHED(dksc))
		return ENXIO;
	return physio(cgdstrategy, NULL, dev, B_READ, minphys, uio);
}

/* XXX: we should probably put these into dksubr.c, mostly */
static int
cgdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct	cgd_softc *sc;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("cgdwrite(0x%"PRIx64", %p, %d)\n", dev, uio, flags));
	sc = getcgd_softc(dev);
	if (sc == NULL)
		return ENXIO;
	dksc = &sc->sc_dksc;
	if (!DK_ATTACHED(dksc))
		return ENXIO;
	return physio(cgdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
cgdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct	cgd_softc *sc;
	struct	dk_softc *dksc;
	int	part = DISKPART(dev);
	int	pmask = 1 << part;
	int	error;

	DPRINTF_FOLLOW(("cgdioctl(0x%"PRIx64", %ld, %p, %d, %p)\n",
	    dev, cmd, data, flag, l));

	switch (cmd) {
	case CGDIOCGET:
		return cgd_ioctl_get(dev, data, l);
	case CGDIOCSET:
	case CGDIOCCLR:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* FALLTHROUGH */
	default:
		sc = getcgd_softc(dev);
		if (sc == NULL)
			return ENXIO;
		dksc = &sc->sc_dksc;
		break;
	}

	switch (cmd) {
	case CGDIOCSET:
		cgd_busy(sc);
		if (DK_ATTACHED(dksc))
			error = EBUSY;
		else
			error = cgd_ioctl_set(sc, data, l);
		cgd_unbusy(sc);
		break;
	case CGDIOCCLR:
		cgd_busy(sc);
		if (DK_BUSY(&sc->sc_dksc, pmask))
			error = EBUSY;
		else
			error = cgd_ioctl_clr(sc, l);
		cgd_unbusy(sc);
		break;
	case DIOCGCACHE:
	case DIOCCACHESYNC:
		cgd_busy(sc);
		if (!DK_ATTACHED(dksc)) {
			cgd_unbusy(sc);
			error = ENOENT;
			break;
		}
		/*
		 * We pass this call down to the underlying disk.
		 */
		error = VOP_IOCTL(sc->sc_tvn, cmd, data, flag, l->l_cred);
		cgd_unbusy(sc);
		break;
	case DIOCGSECTORALIGN: {
		struct disk_sectoralign *dsa = data;

		cgd_busy(sc);
		if (!DK_ATTACHED(dksc)) {
			cgd_unbusy(sc);
			error = ENOENT;
			break;
		}

		/* Get the underlying disk's sector alignment.  */
		error = VOP_IOCTL(sc->sc_tvn, cmd, data, flag, l->l_cred);
		if (error) {
			cgd_unbusy(sc);
			break;
		}

		/* Adjust for the disklabel partition if necessary.  */
		if (part != RAW_PART) {
			struct disklabel *lp = dksc->sc_dkdev.dk_label;
			daddr_t offset = lp->d_partitions[part].p_offset;
			uint32_t r = offset % dsa->dsa_alignment;

			if (r < dsa->dsa_firstaligned)
				dsa->dsa_firstaligned = dsa->dsa_firstaligned
				    - r;
			else
				dsa->dsa_firstaligned = (dsa->dsa_firstaligned
				    + dsa->dsa_alignment) - r;
		}
		cgd_unbusy(sc);
		break;
	}
	case DIOCGSTRATEGY:
	case DIOCSSTRATEGY:
		if (!DK_ATTACHED(dksc)) {
			error = ENOENT;
			break;
		}
		/*FALLTHROUGH*/
	default:
		error = dk_ioctl(dksc, dev, cmd, data, flag, l);
		break;
	case CGDIOCGET:
		KASSERT(0);
		error = EINVAL;
	}

	return error;
}

static int
cgddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct	cgd_softc *sc;

	DPRINTF_FOLLOW(("cgddump(0x%"PRIx64", %" PRId64 ", %p, %lu)\n",
	    dev, blkno, va, (unsigned long)size));
	sc = getcgd_softc(dev);
	if (sc == NULL)
		return ENXIO;
	return dk_dump(&sc->sc_dksc, dev, blkno, va, size, DK_DUMP_RECURSIVE);
}

/*
 * XXXrcd:
 *  for now we hardcode the maximum key length.
 */
#define MAX_KEYSIZE	1024

static const struct {
	const char *n;
	int v;
	int d;
} encblkno[] = {
	{ "encblkno",  CGD_CIPHER_CBC_ENCBLKNO8, 1 },
	{ "encblkno8", CGD_CIPHER_CBC_ENCBLKNO8, 1 },
	{ "encblkno1", CGD_CIPHER_CBC_ENCBLKNO1, 8 },
};

/* ARGSUSED */
static int
cgd_ioctl_set(struct cgd_softc *sc, void *data, struct lwp *l)
{
	struct	 cgd_ioctl *ci = data;
	struct	 vnode *vp;
	int	 ret;
	size_t	 i;
	size_t	 keybytes;			/* key length in bytes */
	const char *cp;
	struct pathbuf *pb;
	char	 *inbuf;
	struct dk_softc *dksc = &sc->sc_dksc;

	cp = ci->ci_disk;

	ret = pathbuf_copyin(ci->ci_disk, &pb);
	if (ret != 0) {
		return ret;
	}
	ret = vn_bdev_openpath(pb, &vp, l);
	pathbuf_destroy(pb);
	if (ret != 0) {
		return ret;
	}

	inbuf = kmem_alloc(MAX_KEYSIZE, KM_SLEEP);

	if ((ret = cgdinit(sc, cp, vp, l)) != 0)
		goto bail;

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyinstr(ci->ci_alg, inbuf, 256, NULL);
	if (ret)
		goto bail;
	sc->sc_cfuncs = cryptfuncs_find(inbuf);
	if (!sc->sc_cfuncs) {
		ret = EINVAL;
		goto bail;
	}

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyinstr(ci->ci_ivmethod, inbuf, MAX_KEYSIZE, NULL);
	if (ret)
		goto bail;

	for (i = 0; i < __arraycount(encblkno); i++)
		if (strcmp(encblkno[i].n, inbuf) == 0)
			break;

	if (i == __arraycount(encblkno)) {
		ret = EINVAL;
		goto bail;
	}

	keybytes = ci->ci_keylen / 8 + 1;
	if (keybytes > MAX_KEYSIZE) {
		ret = EINVAL;
		goto bail;
	}

	(void)memset(inbuf, 0, MAX_KEYSIZE);
	ret = copyin(ci->ci_key, inbuf, keybytes);
	if (ret)
		goto bail;

	sc->sc_cdata.cf_blocksize = ci->ci_blocksize;
	sc->sc_cdata.cf_mode = encblkno[i].v;

	/*
	 * Print a warning if the user selected the legacy encblkno8
	 * mistake, and reject it altogether for ciphers that it
	 * doesn't apply to.
	 */
	if (encblkno[i].v != CGD_CIPHER_CBC_ENCBLKNO1) {
		if (strcmp(sc->sc_cfuncs->cf_name, "aes-cbc") &&
		    strcmp(sc->sc_cfuncs->cf_name, "3des-cbc") &&
		    strcmp(sc->sc_cfuncs->cf_name, "blowfish-cbc")) {
			log(LOG_WARNING, "cgd: %s only makes sense for cbc,"
			    " not for %s; ignoring\n",
			    encblkno[i].n, sc->sc_cfuncs->cf_name);
			sc->sc_cdata.cf_mode = CGD_CIPHER_CBC_ENCBLKNO1;
		} else {
			log(LOG_WARNING, "cgd: enabling legacy encblkno8\n");
		}
	}

	sc->sc_cdata.cf_keylen = ci->ci_keylen;
	sc->sc_cdata.cf_priv = sc->sc_cfuncs->cf_init(ci->ci_keylen, inbuf,
	    &sc->sc_cdata.cf_blocksize);
	if (sc->sc_cdata.cf_blocksize > CGD_MAXBLOCKSIZE) {
	    log(LOG_WARNING, "cgd: Disallowed cipher with blocksize %zu > %u\n",
		sc->sc_cdata.cf_blocksize, CGD_MAXBLOCKSIZE);
	    sc->sc_cdata.cf_priv = NULL;
	}

	/*
	 * The blocksize is supposed to be in bytes. Unfortunately originally
	 * it was expressed in bits. For compatibility we maintain encblkno
	 * and encblkno8.
	 */
	sc->sc_cdata.cf_blocksize /= encblkno[i].d;
	(void)explicit_memset(inbuf, 0, MAX_KEYSIZE);
	if (!sc->sc_cdata.cf_priv) {
		ret = EINVAL;		/* XXX is this the right error? */
		goto bail;
	}
	kmem_free(inbuf, MAX_KEYSIZE);

	bufq_alloc(&dksc->sc_bufq, "fcfs", 0);

	sc->sc_data = kmem_alloc(MAXPHYS, KM_SLEEP);
	sc->sc_data_used = false;

	/* Attach the disk. */
	dk_attach(dksc);
	disk_attach(&dksc->sc_dkdev);

	disk_set_info(dksc->sc_dev, &dksc->sc_dkdev, NULL);

	/* Discover wedges on this disk. */
	dkwedge_discover(&dksc->sc_dkdev);

	return 0;

bail:
	kmem_free(inbuf, MAX_KEYSIZE);
	(void)vn_close(vp, FREAD|FWRITE, l->l_cred);
	return ret;
}

/* ARGSUSED */
static int
cgd_ioctl_clr(struct cgd_softc *sc, struct lwp *l)
{
	struct	dk_softc *dksc = &sc->sc_dksc;

	if (!DK_ATTACHED(dksc))
		return ENXIO;

	/* Delete all of our wedges. */
	dkwedge_delall(&dksc->sc_dkdev);

	/* Kill off any queued buffers. */
	dk_drain(dksc);
	bufq_free(dksc->sc_bufq);

	(void)vn_close(sc->sc_tvn, FREAD|FWRITE, l->l_cred);
	sc->sc_cfuncs->cf_destroy(sc->sc_cdata.cf_priv);
	kmem_free(sc->sc_tpath, sc->sc_tpathlen);
	kmem_free(sc->sc_data, MAXPHYS);
	sc->sc_data_used = false;
	dk_detach(dksc);
	disk_detach(&dksc->sc_dkdev);

	return 0;
}

static int
cgd_ioctl_get(dev_t dev, void *data, struct lwp *l)
{
	struct cgd_softc *sc;
	struct cgd_user *cgu;
	int unit, error;

	unit = CGDUNIT(dev);
	cgu = (struct cgd_user *)data;

	DPRINTF_FOLLOW(("cgd_ioctl_get(0x%"PRIx64", %d, %p, %p)\n",
			   dev, unit, data, l));

	/* XXX, we always return this units data, so if cgu_unit is
	 * not -1, that field doesn't match the rest
	 */
	if (cgu->cgu_unit == -1)
		cgu->cgu_unit = unit;

	if (cgu->cgu_unit < 0)
		return EINVAL;	/* XXX: should this be ENXIO? */

	error = cgd_lock(false);
	if (error)
		return error;

	sc = device_lookup_private(&cgd_cd, unit);
	if (sc == NULL || !DK_ATTACHED(&sc->sc_dksc)) {
		cgu->cgu_dev = 0;
		cgu->cgu_alg[0] = '\0';
		cgu->cgu_blocksize = 0;
		cgu->cgu_mode = 0;
		cgu->cgu_keylen = 0;
	}
	else {
		mutex_enter(&sc->sc_lock);
		cgu->cgu_dev = sc->sc_tdev;
		strncpy(cgu->cgu_alg, sc->sc_cfuncs->cf_name,
		    sizeof(cgu->cgu_alg));
		cgu->cgu_blocksize = sc->sc_cdata.cf_blocksize;
		cgu->cgu_mode = sc->sc_cdata.cf_mode;
		cgu->cgu_keylen = sc->sc_cdata.cf_keylen;
		mutex_exit(&sc->sc_lock);
	}

	cgd_unlock();
	return 0;
}

static int
cgdinit(struct cgd_softc *sc, const char *cpath, struct vnode *vp,
	struct lwp *l)
{
	struct	disk_geom *dg;
	int	ret;
	char	*tmppath;
	uint64_t psize;
	unsigned secsize;
	struct dk_softc *dksc = &sc->sc_dksc;

	sc->sc_tvn = vp;
	sc->sc_tpath = NULL;

	tmppath = kmem_alloc(MAXPATHLEN, KM_SLEEP);
	ret = copyinstr(cpath, tmppath, MAXPATHLEN, &sc->sc_tpathlen);
	if (ret)
		goto bail;
	sc->sc_tpath = kmem_alloc(sc->sc_tpathlen, KM_SLEEP);
	memcpy(sc->sc_tpath, tmppath, sc->sc_tpathlen);

	sc->sc_tdev = vp->v_rdev;

	if ((ret = getdisksize(vp, &psize, &secsize)) != 0)
		goto bail;

	if (psize == 0) {
		ret = ENODEV;
		goto bail;
	}

	/*
	 * XXX here we should probe the underlying device.  If we
	 *     are accessing a partition of type RAW_PART, then
	 *     we should populate our initial geometry with the
	 *     geometry that we discover from the device.
	 */
	dg = &dksc->sc_dkdev.dk_geom;
	memset(dg, 0, sizeof(*dg));
	dg->dg_secperunit = psize;
	dg->dg_secsize = secsize;
	dg->dg_ntracks = 1;
	dg->dg_nsectors = 1024 * 1024 / dg->dg_secsize;
	dg->dg_ncylinders = dg->dg_secperunit / dg->dg_nsectors;

bail:
	kmem_free(tmppath, MAXPATHLEN);
	if (ret && sc->sc_tpath)
		kmem_free(sc->sc_tpath, sc->sc_tpathlen);
	return ret;
}

/*
 * Our generic cipher entry point.  This takes care of the
 * IV mode and passes off the work to the specific cipher.
 * We implement here the IV method ``encrypted block
 * number''.
 *
 * XXXrcd: for now we rely on our own crypto framework defined
 *         in dev/cgd_crypto.c.  This will change when we
 *         get a generic kernel crypto framework.
 */

static void
blkno2blkno_buf(char *sbuf, daddr_t blkno)
{
	int	i;

	/* Set up the blkno in blkno_buf, here we do not care much
	 * about the final layout of the information as long as we
	 * can guarantee that each sector will have a different IV
	 * and that the endianness of the machine will not affect
	 * the representation that we have chosen.
	 *
	 * We choose this representation, because it does not rely
	 * on the size of buf (which is the blocksize of the cipher),
	 * but allows daddr_t to grow without breaking existing
	 * disks.
	 *
	 * Note that blkno2blkno_buf does not take a size as input,
	 * and hence must be called on a pre-zeroed buffer of length
	 * greater than or equal to sizeof(daddr_t).
	 */
	for (i=0; i < sizeof(daddr_t); i++) {
		*sbuf++ = blkno & 0xff;
		blkno >>= 8;
	}
}

static struct cpu_info *
cgd_cpu(struct cgd_softc *sc)
{
	struct cgd_worker *cw = sc->sc_worker;
	struct cpu_info *ci = NULL;
	u_int cidx, i;

	if (cw->cw_busy == 0) {
		cw->cw_last = cpu_index(curcpu());
		return NULL;
	}

	for (i=0, cidx = cw->cw_last+1; i<maxcpus; ++i, ++cidx) {
		if (cidx >= maxcpus)
			cidx = 0;
		ci = cpu_lookup(cidx);
		if (ci) {
			cw->cw_last = cidx;
			break;
		}
	}

	return ci;
}

static void
cgd_enqueue(struct cgd_softc *sc, struct cgd_xfer *cx)
{
	struct cgd_worker *cw = sc->sc_worker;
	struct cpu_info *ci;

	mutex_enter(&cw->cw_lock);
	ci = cgd_cpu(sc);
	cw->cw_busy++;
	mutex_exit(&cw->cw_lock);

	workqueue_enqueue(cw->cw_wq, &cx->cx_work, ci);
}

static void
cgd_process(struct work *wk, void *arg)
{
	struct cgd_xfer *cx = (struct cgd_xfer *)wk;
	struct cgd_softc *sc = cx->cx_sc;
	struct cgd_worker *cw = sc->sc_worker;

	cgd_cipher(sc, cx->cx_dstv, cx->cx_srcv, cx->cx_len,
	    cx->cx_blkno, cx->cx_secsize, cx->cx_dir);

	if (cx->cx_dir == CGD_CIPHER_ENCRYPT) {
		cgd_diskstart2(sc, cx);
	} else {
		cgd_iodone2(sc, cx);
	}

	mutex_enter(&cw->cw_lock);
	if (cw->cw_busy > 0)
		cw->cw_busy--;
	mutex_exit(&cw->cw_lock);
}

static void
cgd_cipher(struct cgd_softc *sc, void *dstv, const void *srcv,
    size_t len, daddr_t blkno, size_t secsize, int dir)
{
	char		*dst = dstv;
	const char	*src = srcv;
	cfunc_cipher	*cipher = sc->sc_cfuncs->cf_cipher;
	size_t		blocksize = sc->sc_cdata.cf_blocksize;
	size_t		todo;
	char		blkno_buf[CGD_MAXBLOCKSIZE] __aligned(CGD_BLOCKALIGN);

	DPRINTF_FOLLOW(("cgd_cipher() dir=%d\n", dir));

	if (sc->sc_cdata.cf_mode == CGD_CIPHER_CBC_ENCBLKNO8)
		blocksize /= 8;

	KASSERT(len % blocksize == 0);
	/* ensure that sizeof(daddr_t) <= blocksize (for encblkno IVing) */
	KASSERT(sizeof(daddr_t) <= blocksize);
	KASSERT(blocksize <= CGD_MAXBLOCKSIZE);

	for (; len > 0; len -= todo) {
		todo = MIN(len, secsize);

		memset(blkno_buf, 0x0, blocksize);
		blkno2blkno_buf(blkno_buf, blkno);
		IFDEBUG(CGDB_CRYPTO, hexprint("step 1: blkno_buf",
		    blkno_buf, blocksize));

		/*
		 * Handle bollocksed up encblkno8 mistake.  We used to
		 * compute the encryption of a zero block with blkno as
		 * the CBC IV -- except in an early mistake arising
		 * from bit/byte confusion, we actually computed the
		 * encryption of the last of _eight_ zero blocks under
		 * CBC as the CBC IV.
		 *
		 * Encrypting the block number is handled inside the
		 * cipher dispatch now (even though in practice, both
		 * CBC and XTS will do the same thing), so we have to
		 * simulate the block number that would yield the same
		 * result.  So we encrypt _six_ zero blocks -- the
		 * first one and the last one are handled inside the
		 * cipher dispatch.
		 */
		if (sc->sc_cdata.cf_mode == CGD_CIPHER_CBC_ENCBLKNO8) {
			static const uint8_t zero[CGD_MAXBLOCKSIZE];
			uint8_t iv[CGD_MAXBLOCKSIZE];

			memcpy(iv, blkno_buf, blocksize);
			cipher(sc->sc_cdata.cf_priv, blkno_buf, zero,
			    6*blocksize, iv, CGD_CIPHER_ENCRYPT);
			memmove(blkno_buf, blkno_buf + 5*blocksize, blocksize);
		}

		cipher(sc->sc_cdata.cf_priv, dst, src, todo, blkno_buf, dir);

		dst += todo;
		src += todo;
		blkno++;
	}
}

#ifdef DEBUG
static void
hexprint(const char *start, void *buf, int len)
{
	char	*c = buf;

	KASSERTMSG(len >= 0, "hexprint: called with len < 0");
	printf("%s: len=%06d 0x", start, len);
	while (len--)
		printf("%02x", (unsigned char) *c++);
}
#endif

static void
cgd_selftest(void)
{
	struct cgd_softc sc;
	void *buf;

	for (size_t i = 0; i < __arraycount(selftests); i++) {
		const char *alg = selftests[i].alg;
		int encblkno8 = selftests[i].encblkno8;
		const uint8_t *key = selftests[i].key;
		int keylen = selftests[i].keylen;
		int txtlen = selftests[i].txtlen;

		aprint_debug("cgd: self-test %s-%d%s\n", alg, keylen,
		    encblkno8 ? " (encblkno8)" : "");

		memset(&sc, 0, sizeof(sc));

		sc.sc_cfuncs = cryptfuncs_find(alg);
		if (sc.sc_cfuncs == NULL)
			panic("%s not implemented", alg);

		sc.sc_cdata.cf_blocksize = 8 * selftests[i].blocksize;
		sc.sc_cdata.cf_mode = encblkno8 ? CGD_CIPHER_CBC_ENCBLKNO8 :
		    CGD_CIPHER_CBC_ENCBLKNO1;
		sc.sc_cdata.cf_keylen = keylen;

		sc.sc_cdata.cf_priv = sc.sc_cfuncs->cf_init(keylen,
		    key, &sc.sc_cdata.cf_blocksize);
		if (sc.sc_cdata.cf_priv == NULL)
			panic("cf_priv is NULL");
		if (sc.sc_cdata.cf_blocksize > CGD_MAXBLOCKSIZE)
			panic("bad block size %zu", sc.sc_cdata.cf_blocksize);

		if (!encblkno8)
			sc.sc_cdata.cf_blocksize /= 8;

		buf = kmem_alloc(txtlen, KM_SLEEP);
		memcpy(buf, selftests[i].ptxt, txtlen);

		cgd_cipher(&sc, buf, buf, txtlen, selftests[i].blkno,
				selftests[i].secsize, CGD_CIPHER_ENCRYPT);
		if (memcmp(buf, selftests[i].ctxt, txtlen) != 0) {
			hexdump(printf, "was", buf, txtlen);
			hexdump(printf, "exp", selftests[i].ctxt, txtlen);
			panic("cgd %s-%d encryption is broken [%zu]",
			    selftests[i].alg, keylen, i);
		}

		cgd_cipher(&sc, buf, buf, txtlen, selftests[i].blkno,
				selftests[i].secsize, CGD_CIPHER_DECRYPT);
		if (memcmp(buf, selftests[i].ptxt, txtlen) != 0) {
			hexdump(printf, "was", buf, txtlen);
			hexdump(printf, "exp", selftests[i].ptxt, txtlen);
			panic("cgd %s-%d decryption is broken [%zu]",
			    selftests[i].alg, keylen, i);
		}

		kmem_free(buf, txtlen);
		sc.sc_cfuncs->cf_destroy(sc.sc_cdata.cf_priv);
	}

	aprint_debug("cgd: self-tests passed\n");
}

MODULE(MODULE_CLASS_DRIVER, cgd, "adiantum,blowfish,des,dk_subr,bufq_fcfs");

#ifdef _MODULE
CFDRIVER_DECL(cgd, DV_DISK, NULL);

devmajor_t cgd_bmajor = -1, cgd_cmajor = -1;
#endif

static int
cgd_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		mutex_init(&cgd_spawning_mtx, MUTEX_DEFAULT, IPL_NONE);
		cv_init(&cgd_spawning_cv, "cgspwn");

		/*
		 * Attach the {b,c}devsw's
		 */
		error = devsw_attach("cgd", &cgd_bdevsw, &cgd_bmajor,
		    &cgd_cdevsw, &cgd_cmajor);
		if (error) {
			aprint_error("%s: unable to attach %s devsw, "
			    "error %d", __func__, cgd_cd.cd_name, error);
			break;
		}

		/*
		 * Attach to autoconf database
		 */
		error = config_cfdriver_attach(&cgd_cd);
		if (error) {
			devsw_detach(&cgd_bdevsw, &cgd_cdevsw);
			aprint_error("%s: unable to register cfdriver for"
			    "%s, error %d\n", __func__, cgd_cd.cd_name, error);
			break;
		}

		error = config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
	        if (error) {
			config_cfdriver_detach(&cgd_cd);
			devsw_detach(&cgd_bdevsw, &cgd_cdevsw);
			aprint_error("%s: unable to register cfattach for"
			    "%s, error %d\n", __func__, cgd_cd.cd_name, error);
			break;
		}
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		/*
		 * Remove device from autoconf database
		 */
		error = config_cfattach_detach(cgd_cd.cd_name, &cgd_ca);
		if (error) {
			aprint_error("%s: failed to detach %s cfattach, "
			    "error %d\n", __func__, cgd_cd.cd_name, error);
 			break;
		}
		error = config_cfdriver_detach(&cgd_cd);
		if (error) {
			(void)config_cfattach_attach(cgd_cd.cd_name, &cgd_ca);
			aprint_error("%s: failed to detach %s cfdriver, "
			    "error %d\n", __func__, cgd_cd.cd_name, error);
			break;
		}

		/*
		 * Remove {b,c}devsw's
		 */
		devsw_detach(&cgd_bdevsw, &cgd_cdevsw);

		cv_destroy(&cgd_spawning_cv);
		mutex_destroy(&cgd_spawning_mtx);
#endif
		break;

	case MODULE_CMD_STAT:
		error = ENOTTY;
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
