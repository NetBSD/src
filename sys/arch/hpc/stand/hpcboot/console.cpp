/* -*-C++-*-	$NetBSD: console.cpp,v 1.1.2.3 2001/03/27 15:30:47 bouyer Exp $ */

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

#include <hpcmenu.h>
#include <console.h>

Console *Console::_instance = 0;

Console *
Console::Instance()
{
	if (_instance == 0)
		_instance = new Console;
	return _instance;
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
	TCHAR tmp[CONSOLE_BUFSIZE];

	va_list ap;
	va_start(ap, fmt);
	wvsprintf(tmp, fmt, ap);
	va_end(ap);
	HpcMenuInterface::Instance().print(tmp);
}

BOOL
SerialConsole::setupBuffer()
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

BOOL
SerialConsole::openCOM1()
{
	HpcMenuInterface &menu = HpcMenuInterface::Instance();
	int speed = menu._pref.serial_speed;

	if (_handle == INVALID_HANDLE_VALUE) {
		_handle = CreateFile(TEXT("COM1:"),
				     GENERIC_READ | GENERIC_WRITE,
				     0, NULL, OPEN_EXISTING, 0, NULL);
		if (_handle == INVALID_HANDLE_VALUE) {
			Console::print(TEXT("couldn't open COM1\n"));
			return FALSE;
		}

		DCB dcb;
		if (!GetCommState(_handle, &dcb)) {
			Console::print(TEXT("couldn't get COM port status.\n"));
			goto bad;
		}
      
		dcb.BaudRate = speed;
		if (!SetCommState(_handle, &dcb)) {
			Console::print(TEXT("couldn't set baud rate to %s.\n"),
				       speed);
			goto bad;
		}

		Console::print(TEXT("BaudRate %d, ByteSize %#x, Parity %#x, StopBits %#x\n"),
			       dcb.BaudRate, dcb.ByteSize, dcb.Parity,
			       dcb.StopBits);
		const char msg[] = "--------HPCBOOT--------\r\n";
		unsigned long wrote;
		WriteFile(_handle, msg, sizeof msg, &wrote, 0);
	}

	return TRUE;
 bad:
	CloseHandle(_handle);
	_handle = INVALID_HANDLE_VALUE;
	return FALSE;
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
