/*	$NetBSD: netbsd.cpp,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
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
#include <e32cons.h>
#include <e32std.h>
#include <f32file.h>

#include "e32boot.h"
#include "elf.h"
#include "netbsd.h"


NetBSD *
NetBSD::New(const TDesC &aFilename)
{
	NetBSD *netbsd = NULL;
	RFs fsSession;
	RFile file;
	TInt size;
	union {
		Elf32_Ehdr elf;
	} hdr;

	/* Connect to FileServer Session */
	User::LeaveIfError(fsSession.Connect());

	User::LeaveIfError(file.Open(fsSession, aFilename, EFileRead));
	User::LeaveIfError(file.Size(size));

	TPtr hdrBuf((TUint8 *)&hdr, sizeof(hdr));
	User::LeaveIfError(file.Read(hdrBuf));
	/* Currently support is ELF only. */
	if (Mem::Compare((TUint8 *)hdr.elf.e_ident, SELFMAG,
					(TUint8 *)ELFMAG, SELFMAG) == 0) {
		netbsd = new (ELeave) ELF(size);
	} else
		User::Leave(KErrNotSupported);
	TInt pos = 0;
	User::LeaveIfError(file.Seek(ESeekStart, pos));

	TPtr fileBuf(netbsd->Buffer, size);
	User::LeaveIfError(file.Read(fileBuf));
	if (fileBuf.Length() != size)
		User::Leave(KErrNotFound);
	file.Close();

	/* Close FileServer Session */
	fsSession.Close();

	netbsd->LoadDescriptor =
	    (struct loaddesc *)PAGE_ALIGN(User::AllocL(ALIGN_SAFE_PAGE_SIZE));

	return netbsd;
}


void
ELF::ParseHeader(void)
{
	struct loaddesc *loaddesc = LoadDescriptor;
	Elf32_Ehdr ehdr;
	Elf32_Phdr *phdr = NULL;
	int PhdrSize, i;

	Mem::Copy(&ehdr, Buffer, sizeof(ehdr));
	PhdrSize = sizeof(Elf32_Phdr) * ehdr.e_phnum;
	phdr = (Elf32_Phdr *) new (ELeave) TUint8[PhdrSize];
	Mem::Copy(phdr, Buffer + ehdr.e_phoff, PhdrSize);

	for (i = 0; i < ehdr.e_phnum; i++) {
		if (phdr[i].p_type != PT_LOAD ||
		    (phdr[i].p_flags & (PF_W | PF_X)) == 0)
			continue;

		loaddesc->addr = VTOP(phdr[i].p_vaddr);
		loaddesc->offset = phdr[i].p_offset;
		loaddesc->size = phdr[i].p_filesz;
		loaddesc++;
	}
	/* Terminate loaddesc. */
	loaddesc->addr = 0xffffffff;
	loaddesc->offset = 0xffffffff;
	loaddesc->size = 0xffffffff;

	EntryPoint = ehdr.e_entry;
}
