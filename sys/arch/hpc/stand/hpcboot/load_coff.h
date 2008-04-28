/* -*-C++-*-	$NetBSD: load_coff.h,v 1.4 2008/04/28 20:23:20 martin Exp $	*/

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

#ifndef _HPCBOOT_LOAD_COFF_H_
#define	_HPCBOOT_LOAD_COFF_H_

#include <exec_coff.h>

#define	FILHSZ	(sizeof(struct coff_filehdr))
#define	SCNHSZ	(sizeof(struct coff_scnhdr))

#define	N_TXTOFF(f, a)							\
	((a)->a_vstamp < 23 ?						\
	((FILHSZ +(f)->f_opthdr +(f)->f_nscns * SCNHSZ + 7) & ~7) :	\
	((FILHSZ +(f)->f_opthdr +(f)->f_nscns * SCNHSZ + 15) & ~15))

class CoffLoader : public Loader {
private:
	struct coff_exechdr _eh;
	struct coff_filehdr *_fh;
	struct coff_aouthdr *_ah;

	BOOL read_header(void);
	struct PageTag *load_page(vaddr_t, off_t, size_t, struct PageTag *);

public:
	CoffLoader(Console *&, MemoryManager *&);
	virtual ~CoffLoader(void);

	BOOL setFile(File *&);
	size_t memorySize(void);
	BOOL load(void);
	kaddr_t jumpAddr(void);
};

#endif //_HPCBOOT_LOAD_COFF_H_
