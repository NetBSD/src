/* -*-C++-*-	$NetBSD: sh_console.h,v 1.6.8.2 2002/02/28 04:09:46 nathanw Exp $	*/

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

#include <console.h>

class SHConsole : public SerialConsole {
private:
	typedef SerialConsole super;

	typedef void (*print_func_t)(const char *);
	print_func_t _print;	// SHConsole native method folder.

public:
	enum consoleSelect { VIDEO, SERIAL };
	struct console_info {
		u_int32_t cpu, machine;
		print_func_t print;
		int16_t serial_console;
		int16_t video_console;
	};
	static const struct console_info _console_info[];
	static const struct console_info *selectBootConsole(Console &,
	    enum consoleSelect);

public:
	SHConsole(void);	//XXX
	virtual ~SHConsole();
	virtual BOOL init(void);
	virtual void print(const TCHAR *fmt, ...);

	static void SCIPrint(const char *);
	static void SCIFPrint(const char *);
	static void HD64461COMPrint(const char *);
	static void HD64465COMPrint(const char *);
};
#endif //_HPCBOOT_SH_CONSOLE_H_
