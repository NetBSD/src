/*	$NetBSD: print.c,v 1.4 2000/06/04 04:30:50 takemura Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>
#include <res/resource.h>	/* for IDC_STATUS, status area ID */

static HANDLE debug_log = INVALID_HANDLE_VALUE;

int 
debug_printf(LPWSTR lpszFmt, ...)
{
	int count;
	va_list ap;
	wchar_t buffer[1024];
	char ascbuf[2048];

	va_start(ap, lpszFmt);
	count = wvsprintf(buffer, lpszFmt, ap);
	va_end(ap);
	if (count > 0) {
		DWORD n;
		OutputDebugStringW(buffer);

		if (debug_log != INVALID_HANDLE_VALUE) {
			/* convert wide char string into multibyte
			   string (ascii) */
			n = wcstombs(ascbuf, buffer, sizeof(ascbuf));
			/* convert into DOS style new line code */
			if (ascbuf[n - 1] == 0x0a) {
				ascbuf[n - 1] = 0x0d;
				ascbuf[n + 0] = 0x0a;
				ascbuf[n + 1] = 0x00;
				n += 1;
			}
			WriteFile(debug_log, ascbuf, n, &n, NULL);
		}
	}
	return count;
}

int 
msg_printf(UINT type, LPWSTR caption, LPWSTR lpszFmt, ...)
{
	int count;
	va_list ap;
	TCHAR buffer[1024];

	va_start(ap, lpszFmt);
	count = wvsprintf(buffer, lpszFmt, ap);
	va_end(ap);
	return MessageBox(hWndMain, buffer, caption, type);
}

int 
stat_printf(LPWSTR lpszFmt, ...)
{
	int count;
	va_list ap;
	wchar_t buffer[1024];

	va_start(ap, lpszFmt);
	count = wvsprintf(buffer, lpszFmt, ap);
	va_end(ap);
	if (count > 0) {
		SetDlgItemText(hWndMain, IDC_STATUS, buffer);
	}
	return count;
}

int
set_debug_log(TCHAR* filename)
{

	/*
	 * Logging into file is dangerous. It may cause file system clash,
	 * because it try to write file until the last moment to boot and
	 * Windows can't flush file cache properly.
	 * And therefore the logging will be disanable unless you put a 
	 * dummy log file on a directory.
	 */
	debug_log = CreateFile(
		filename,      	/* file name */
		GENERIC_READ,	/* access (read-write) mode */
		FILE_SHARE_READ,/* share mode */
		NULL,		/* pointer to security attributes */
		OPEN_EXISTING,	/* how to create */
		FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
		NULL		/* handle to file with attributes to */
	    );
	if (debug_log == INVALID_HANDLE_VALUE) {
		return (-1);
	}
	CloseHandle(debug_log);

	debug_log = CreateFile(
		filename,      	/* file name */
		GENERIC_WRITE,	/* access (read-write) mode */
		FILE_SHARE_WRITE,/* share mode */
		NULL,		/* pointer to security attributes */
		CREATE_ALWAYS,	/* how to create */
		FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
		NULL		/* handle to file with attributes to */
	    );

	if (debug_log == INVALID_HANDLE_VALUE) {
		return (-1);
	}

	return (0);
}

void
close_debug_log()
{
	if (debug_log != INVALID_HANDLE_VALUE) {
		CloseHandle(debug_log);
		debug_log = INVALID_HANDLE_VALUE;
		/*
		 * I hope Windows flush file buffer properly...
		 */
		Sleep(2000);
	}
}
