/* -*-C++-*-	$NetBSD: sh_console.h,v 1.7 2002/02/04 17:38:27 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_SH_CONSOLE_H_
#define _HPCBOOT_SH_CONSOLE_H_

#include <hpcboot.h>
#include <console.h>
#include <memory.h>
#include <sh3/sh3.h>

class SHConsole : public SerialConsole {
private:
	typedef SerialConsole super;

public:
	typedef void (*print_func_t)(const char *);
	struct console_info {
		u_int32_t cpu, machine;
		print_func_t print;
		int16_t serial_console;
		int16_t video_console;
	};
	static struct console_info _console_info[];
	enum consoleSelect { VIDEO, SERIAL };
	static struct console_info *selectBootConsole(Console &,
	    enum consoleSelect);
	static void SCIPrint(const char *);
	static void SCIFPrint(const char *);
	static void HD64461COMPrint(const char *);
	static void HD64465COMPrint(const char *);

private:
	static SHConsole *_instance;
	int _kmode;
	print_func_t _print;

	SHConsole(void);

public:
	virtual ~SHConsole();
	static SHConsole *Instance(void);

	virtual BOOL init(void);
	virtual void print(const TCHAR *fmt, ...);
};
#endif //_HPCBOOT_SH_CONSOLE_H_
