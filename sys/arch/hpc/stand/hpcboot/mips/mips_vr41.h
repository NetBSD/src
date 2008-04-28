/* -*-C++-*-	$NetBSD: mips_vr41.h,v 1.4 2008/04/28 20:23:20 martin Exp $	*/

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

#include <hpcboot.h>
#include <mips/mips_arch.h>

class VR41XX : public MIPSArchitecture {
private:

public:
	VR41XX(Console *&, MemoryManager *&);
	~VR41XX(void);

	virtual BOOL init(void);
	virtual void systemInfo(void);
	virtual void cacheFlush(void);
	static void boot_func(struct BootArgs *, struct PageTag *);
};

#define	MIPS_VR41XX_CACHE_FLUSH()					\
__asm(									\
	".set	noreorder;"						\
	/* Flush I-cache */						\
	"li	t0, 0x80000000;"					\
	"addu	t1, t0, 1024*128;"					\
	"subu	t1, t1, 128;"						\
"1:"									\
	"cache	0, 0(t0);"						\
	"cache	0, 16(t0);"						\
	"cache	0, 32(t0);"						\
	"cache	0, 48(t0);"						\
	"cache	0, 64(t0);"						\
	"cache	0, 80(t0);"						\
	"cache	0, 96(t0);"						\
	"cache	0, 112(t0);"						\
	"bne	t0, t1, 1b;"						\
	"addu	t0, t0, 128;"						\
									\
	/* Flush D-cache */						\
	"li	t0, 0x80000000;"					\
	"addu	t1, t0, 1024*128;"					\
	"subu	t1, t1, 128;"						\
"2:"									\
	"cache   1, 0(t0);"						\
	"cache   1, 16(t0);"						\
	"cache   1, 32(t0);"						\
	"cache   1, 48(t0);"						\
	"cache   1, 64(t0);"						\
	"cache   1, 80(t0);"						\
	"cache   1, 96(t0);"						\
	"cache   1, 112(t0);"						\
	"bne     t0, t1, 2b;"						\
	"addu    t0, t0, 128;"						\
	".set reorder;"							\
)
