/*	$NetBSD: dtf_comms.h,v 1.1 2002/07/05 13:32:04 scw Exp $	*/

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

#ifndef _SH5_DTF_COMMS_H
#define _SH5_DTF_COMMS_H

extern void dtf_init(paddr_t, paddr_t, paddr_t, vaddr_t);
extern int dtf_transaction(void *, int *);

extern u_int8_t *dtf_packbytes(u_int8_t *, const void *, int);
extern u_int8_t *dtf_packword(u_int8_t *, u_int16_t);
extern u_int8_t *dtf_packdword(u_int8_t *, u_int32_t);
extern u_int8_t *dtf_unpackbytes(u_int8_t *, void *, int);
extern u_int8_t *dtf_unpackword(u_int8_t *, u_int16_t *);
extern u_int8_t *dtf_unpackdword(u_int8_t *, u_int32_t *);

#define	DTF_MAX_PACKET_LEN	1024

#define	DTF_POSIX_WRITE		7
#define	DTF_POSIX_POLLKEY	24

#endif /* _SH5_DTF_COMMS_H */
