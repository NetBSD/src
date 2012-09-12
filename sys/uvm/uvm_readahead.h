/*	$NetBSD: uvm_readahead.h,v 1.4.22.1 2012/09/12 06:15:36 tls Exp $	*/

/*-
 * Copyright (c)2003, 2005, 2009 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if !defined(_UVM_UVM_READAHEAD_H_)
#define _UVM_UVM_READAHEAD_H_

struct uvm_object;

/*
 * uvm_ractx: read-ahead context.
 */

struct uvm_ractx {
        int ra_flags;
#define UVM_RA_VALID        1
        size_t ra_iochunk;
        size_t ra_winsize;      /* window size */
        off_t ra_winstart;      /* window start offset */
        off_t ra_next;          /* next offset to read-ahead */
};

#define UVM_RA_WINSIZE_INIT	MAXPHYS		/* initial window size */
#define UVM_RA_WINSIZE_MAX	(MAXPHYS * 8)	/* max window size */
#define UVM_RA_WINSIZE_SEQUENTIAL	UVM_RA_WINSIZE_MAX
#define UVM_RA_MINSIZE		(MAXPHYS * 2)	/* min size to start i/o */

void uvm_ra_init(void);
struct uvm_ractx *uvm_ra_allocctx(void);
void uvm_ra_freectx(struct uvm_ractx *);
void uvm_ra_request(struct uvm_ractx *, int, struct uvm_object *, off_t,
    size_t);
int uvm_readahead(struct uvm_object *, off_t, off_t, struct uvm_ractx *);

#endif /* defined(_UVM_UVM_READAHEAD_H_) */
