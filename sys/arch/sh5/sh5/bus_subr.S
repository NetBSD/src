/*	$NetBSD: bus_subr.S,v 1.1 2002/07/05 13:32:03 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ##########
 *
 * This is not a standalone file.
 * To use it, put #include <sh5/sh5/bus_subr.S> in your port's locore.S
 *
 * ##########
 */

/******************************************************************************
 *
 * u_int8_t
 * _bus_space_read_1(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_1)
	ptabs/l	r18, tr0
	ldx.ub	r3, r4, r2
	blink	tr0, r63

/******************************************************************************
 *
 * u_int16_t
 * _bus_space_read_2(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_2)
	ptabs/l	r18, tr0
	ldx.uw	r3, r4, r2
	blink	tr0, r63

/******************************************************************************
 *
 * u_int32_t
 * _bus_space_read_4(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_4)
	ldx.l	r3, r4, r2
	ptabs/l	r18, tr0
	addz.l	r2, r63, r2
	blink	tr0, r63

/******************************************************************************
 *
 * u_int64_t
 * _bus_space_read_8(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_8)
	ptabs/l	r18, tr0
	ldx.q	r3, r4, r2
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_1(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int8_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_1)
	ptabs/l	r18, tr0
	stx.b	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_2(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int16_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_2)
	ptabs/l	r18, tr0
	stx.w	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_4(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int32_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_4)
	ptabs/l	r18, tr0
	stx.l	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_8(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int64_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_8)
	ptabs/l	r18, tr0
	stx.q	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * u_int16_t
 * _bus_space_read_stream_2(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_stream_2)
	ptabs/l	r18, tr0
	ldx.uw	r3, r4, r2
	blink	tr0, r63

/******************************************************************************
 *
 * u_int32_t
 * _bus_space_read_stream_4(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_stream_4)
	ldx.l	r3, r4, r2
	ptabs/l	r18, tr0
	addz.l	r2, r63, r2
	blink	tr0, r63

/******************************************************************************
 *
 * u_int64_t
 * _bus_space_read_stream_8(void *arg, bus_space_handle_t h, bus_size_t off)
 */
ENTRY_NOPROFILE(_bus_space_read_stream_8)
	ptabs/l	r18, tr0
	ldx.q	r3, r4, r2
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_stream_2(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int16_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_stream_2)
	ptabs/l	r18, tr0
	stx.w	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_stream_4(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int32_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_stream_4)
	ptabs/l	r18, tr0
	stx.l	r3, r4, r5
	synco
	blink	tr0, r63

/******************************************************************************
 *
 * void
 * _bus_space_write_stream_8(void *arg, bus_space_handle_t h,
 *		      bus_size_t off, u_int64_t val)
 */
ENTRY_NOPROFILE(_bus_space_write_stream_8)
	ptabs/l	r18, tr0
	stx.q	r3, r4, r5
	synco
	blink	tr0, r63
