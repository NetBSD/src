/* -*-C++-*-	$NetBSD: console.h,v 1.6.8.3 2002/04/01 07:40:13 nathanw Exp $	*/

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

#ifndef _HPCBOOT_CONSOLE_H_
#define _HPCBOOT_CONSOLE_H_

#include <hpcboot.h>
#include <hpcmenu.h>

class Console {
private:
	int16_t _boot_console;

protected:
	enum { CONSOLE_BUFSIZE = 256 };
	TCHAR _bufw[CONSOLE_BUFSIZE];	// wide char buffer.
	BOOL _on;

protected:
	Console(void);
	~Console(void) { /* NO-OP */ }

public:
	static Console *_instance;
	static Console *Instance(void);
	static void Destroy(void);
	virtual void print(const TCHAR *fmt, ...);
	virtual BOOL init(void) { return TRUE; }
	BOOL &on(void) { return _on; }

	void setBootConsole(u_int16_t cnuse) { _boot_console = cnuse; }
	int16_t getBootConsole(void) const { return _boot_console; }
};

class SerialConsole : public Console
{
private:
	HANDLE _handle;

protected:
	char _bufm[CONSOLE_BUFSIZE];	// multibyte char buffer.

protected:
	SerialConsole(void);
	~SerialConsole(void) { /* NO-OP */ };
	BOOL setupMultibyteBuffer(void);

public:
	void genericPrint(const char *);
	virtual BOOL init(void);
	virtual void print(const TCHAR *fmt, ...);

	static HANDLE OpenCOM1(void);
};

#define SETUP_WIDECHAR_BUFFER()						\
__BEGIN_MACRO								\
	va_list ap;							\
	va_start(ap, fmt);						\
	wvsprintf(_bufw, fmt, ap);					\
	va_end(ap);							\
__END_MACRO

#define DPRINTF_SETUP()		Console *_cons = Console::Instance()
#define DPRINTFN(level, x)						\
	if (_debug > level) _cons->print x
#define DPRINTF(x)							\
	_cons->print x

#endif // _HPCBOOT_CONSOLE_H_
