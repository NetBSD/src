/* -*-C++-*-	$NetBSD: console.cpp,v 1.8 2002/03/02 22:01:34 uch Exp $ */

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

#include <hpcmenu.h>
#include <console.h>

Console *Console::_instance = 0;

//
// Display console
//
Console *
Console::Instance()
{

	if (_instance == 0) {
		_instance = new Console;
	}

	return (_instance);
}

Console::Console()
{
	// set default builtin console. (bicons)
	setBootConsole(BI_CNUSE_BUILTIN);
}

void
Console::Destroy()
{

	if (_instance)
		delete _instance;
	_instance = 0;
}

void
Console::print(const TCHAR *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	wvsprintf(_bufw, fmt, ap);
	va_end(ap);

	// print to `Console Tab Window'
	HPC_MENU.print(_bufw);
}

//
// Serial console.
//
SerialConsole::SerialConsole()
{

	_handle = INVALID_HANDLE_VALUE;
	// set default serial console.
	setBootConsole(BI_CNUSE_SERIAL);
}

BOOL
SerialConsole::init()
{
	// always open COM1 to supply clock and power for the
	// sake of kernel serial driver 
	if (_handle == INVALID_HANDLE_VALUE)
		_handle = OpenCOM1(); 

	if (_handle == INVALID_HANDLE_VALUE) {
		Console::print(TEXT("couldn't open COM1\n"));
		return (FALSE);
	}

	// Print serial console status on LCD.
	DCB dcb;
	GetCommState(_handle, &dcb);
	Console::print(
		TEXT("BaudRate %d, ByteSize %#x, Parity %#x, StopBits %#x\n"),
		dcb.BaudRate, dcb.ByteSize, dcb.Parity, dcb.StopBits);

	return (TRUE);
}

BOOL
SerialConsole::setupMultibyteBuffer()
{
	size_t len = WideCharToMultiByte(CP_ACP, 0, _bufw, wcslen(_bufw),
	    0, 0, 0, 0);

	if (len + 1 > CONSOLE_BUFSIZE)
		return FALSE;
	if (!WideCharToMultiByte(CP_ACP, 0, _bufw, len, _bufm, len, 0, 0))
		return FALSE;
	_bufm[len] = '\0';

	return TRUE;
}

void
SerialConsole::print(const TCHAR *fmt, ...)
{

	SETUP_WIDECHAR_BUFFER();

	if (!setupMultibyteBuffer())
		return;

	genericPrint(_bufm);
}

HANDLE
SerialConsole::OpenCOM1()
{
	static HANDLE COM1handle = INVALID_HANDLE_VALUE;
	const char msg[] = "\r\n--------HPCBOOT--------\r\n";
	unsigned long wrote;
	int speed = HPC_PREFERENCE.serial_speed;
	HANDLE h;

	if (COM1handle != INVALID_HANDLE_VALUE)
		return (COM1handle);

	h = CreateFile(TEXT("COM1:"), 
	    GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,
	    NULL);
	if (h == INVALID_HANDLE_VALUE)
		return (h);

	DCB dcb;
	if (!GetCommState(h, &dcb))
		goto bad;
      
	dcb.BaudRate = speed;
	if (!SetCommState(h, &dcb))
		goto bad;

	// Print banner on serial console.
	WriteFile(h, msg, sizeof msg, &wrote, 0);

	COM1handle = h;

	return (h);
 bad:
	CloseHandle(h);
	return (INVALID_HANDLE_VALUE);
}

void
SerialConsole::genericPrint(const char *buf)
{
	unsigned long wrote;
	int i;
	
	for (i = 0; *buf != '\0'; buf++) {
		char c = *buf;
		if (c == '\n')
			WriteFile(_handle, "\r", 1, &wrote, 0);
		WriteFile(_handle, &c, 1, &wrote, 0);
	}
}
