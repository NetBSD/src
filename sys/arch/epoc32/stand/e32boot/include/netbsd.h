/*	$NetBSD: netbsd.h,v 1.1 2013/04/28 12:11:27 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "elf.h"

#define PAGE_SIZE		4096
#define ALIGN_SAFE_PAGE_SIZE	(PAGE_SIZE * 2)
#define PAGE_ALIGN(p)	((int)((char *)(p) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* Load kernel to PA==VA area. */
#define VTOP(addr)	(addr)

struct loaddesc {
	Elf32_Addr addr;
	Elf32_Off offset;
	TUint size;
};

class NetBSD {
public:
	static NetBSD *New(const TDesC &);

	virtual void ParseHeader(void);
	TUint8 *getBuffer(void) { return Buffer; };
	Elf32_Off getEntryPoint(void) { return EntryPoint; };
	struct loaddesc *getLoadDescriptor(void) { return LoadDescriptor; };

protected:
	TUint8 *Buffer;
	struct loaddesc *LoadDescriptor;	/* Must page aligned */
	Elf32_Off EntryPoint;
};

class ELF : public NetBSD {
public:
	ELF(TInt size) { Buffer = new TUint8[size]; };

	void ParseHeader(void);
};
