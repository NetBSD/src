/* $NetBSD: au8522mod_qam64.h,v 1.1 2011/07/09 15:00:44 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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

static const struct au8522_modulation_table au8522_modulation_qam64[] = {
	{ 0x00a3, 0x09 },
	{ 0x00a4, 0x00 },
	{ 0x0081, 0xc4 },
	{ 0x00a5, 0x40 },
	{ 0x00aa, 0x77 },
	{ 0x00ad, 0x77 },
	{ 0x00a6, 0x67 },
	{ 0x0262, 0x20 },
	{ 0x021c, 0x30 },
	{ 0x00b8, 0x3e },
	{ 0x00b9, 0xf0 },
	{ 0x00ba, 0x01 },
	{ 0x00bb, 0x18 },
	{ 0x00bc, 0x50 },
	{ 0x00bd, 0x00 },
	{ 0x00be, 0xea },
	{ 0x00bf, 0xef },
	{ 0x00c0, 0xfc },
	{ 0x00c1, 0xbd },
	{ 0x00c2, 0x1f },
	{ 0x00c3, 0xfc },
	{ 0x00c4, 0xdd },
	{ 0x00c5, 0xaf },
	{ 0x00c6, 0x00 },
	{ 0x00c7, 0x38 },
	{ 0x00c8, 0x30 },
	{ 0x00c9, 0x05 },
	{ 0x00ca, 0x4a },
	{ 0x00cb, 0xd0 },
	{ 0x00cc, 0x01 },
	{ 0x00cd, 0xd9 },
	{ 0x00ce, 0x6f },
	{ 0x00cf, 0xf9 },
	{ 0x00d0, 0x70 },
	{ 0x00d1, 0xdf },
	{ 0x00d2, 0xf7 },
	{ 0x00d3, 0xc2 },
	{ 0x00d4, 0xdf },
	{ 0x00d5, 0x02 },
	{ 0x00d6, 0x9a },
	{ 0x00d7, 0xd0 },
	{ 0x0250, 0x0d },
	{ 0x0251, 0xcd },
	{ 0x0252, 0xe0 },
	{ 0x0253, 0x05 },
	{ 0x0254, 0xa7 },
	{ 0x0255, 0xff },
	{ 0x0256, 0xed },
	{ 0x0257, 0x5b },
	{ 0x0258, 0xae },
	{ 0x0259, 0xe6 },
	{ 0x025a, 0x3d },
	{ 0x025b, 0x0f },
	{ 0x025c, 0x0d },
	{ 0x025d, 0xea },
	{ 0x025e, 0xf2 },
	{ 0x025f, 0x51 },
	{ 0x0260, 0xf5 },
	{ 0x0261, 0x06 },
	{ 0x021a, 0x00 },
	{ 0x0546, 0x40 },
	{ 0x0210, 0xc7 },
	{ 0x0211, 0xaa },
	{ 0x0212, 0xab },
	{ 0x0213, 0x02 },
	{ 0x0502, 0x00 },
	{ 0x0121, 0x04 },
	{ 0x0122, 0x04 },
	{ 0x052e, 0x10 },
	{ 0x00a4, 0xca },
	{ 0x00a7, 0x40 },
	{ 0x0526, 0x01 },
};
