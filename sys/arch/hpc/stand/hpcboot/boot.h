/* -*-C++-*-	$NetBSD: boot.h,v 1.2 2001/04/24 19:27:59 uch Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _HPCBOOT_BOOT_H_
#define _HPCBOOT_BOOT_H_

#include <hpcboot.h>
#include <hpcmenu.h>

class Console;
class Architecture;
class MemoryManager;
class File;
class Loader;

class Boot {
private:
	static Boot *_instance;

protected:
	struct BootSetupArgs args;

protected:
	Boot(void);
	virtual ~Boot(void);

	Console		*_cons;		// LCD/Serial
	Architecture	*_arch;		// StrongARM/VR41XX/TX39XX/SH3/SH4
	MemoryManager	*_mem;		// VirtualAlloc/LockPage/MMU
	File		*_file;		// FAT/FFS/via HTTP
	Loader		*_loader;	// ELF/COFF

public:
	static Boot &Instance(void);
	static void Destroy(void);

	virtual BOOL create(void);
	virtual BOOL setup(void);
	friend void hpcboot(void *);

	BOOL Boot::attachLoader(void);
};

#endif //_HPCBOOT_BOOT_H_
