/*	$NetBSD: ttyin.c,v 1.1.1.2 1997/04/22 13:45:39 mrg Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.h"
#if MSDOS_COMPILER==WIN32C
#include "windows.h"
extern char WIN32getch();
static DWORD console_mode;
#endif

static int tty;

/*
 * Open keyboard for input.
 */
	public void
open_getchr()
{
#if MSDOS_COMPILER==WIN32C
	/* Need this to let child processes inherit our console handle */
	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	tty = (int) CreateFile("CONIN$", GENERIC_READ, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, 
			OPEN_EXISTING, 0L, NULL);
	GetConsoleMode((HANDLE)tty, &console_mode);
	/* Make sure we get Ctrl+C events. */
	SetConsoleMode((HANDLE)tty, 0); /* doesn't work for some reason --jdp */
#else
#if MSDOS_COMPILER || OS2
	extern int fd0;
	/*
	 * Open a new handle to CON: in binary mode 
	 * for unbuffered keyboard read.
	 */
	 fd0 = dup(0);
	 close(0);
	 tty = OPEN_TTYIN();
#else
	/*
	 * Try /dev/tty.
	 * If that doesn't work, use file descriptor 2,
	 * which in Unix is usually attached to the screen,
	 * but also usually lets you read from the keyboard.
	 */
	tty = OPEN_TTYIN();
	if (tty < 0)
		tty = 2;
#endif
#endif
}

/*
 * Close the keyboard.
 */
	public void
close_getchr()
{
#if MSDOS_COMPILER==WIN32C
	SetConsoleMode((HANDLE)tty, console_mode);
	CloseHandle((HANDLE)tty);
#endif
}

/*
 * Get a character from the keyboard.
 */
	public int
getchr()
{
	char c;
	int result;

	do
	{
#if MSDOS_COMPILER
		/*
		 * In raw read, we don't see ^C so look here for it.
		 */
		flush();
#if MSDOS_COMPILER==WIN32C
		c = WIN32getch(tty);
#else
		c = getch();
#endif
		result = 1;
		if (c == '\003')
			return (READ_INTR);
#else
#if OS2
	{
		static int scan = -1;
		flush();
		if (scan >= 0)
		{
			c = scan;
			scan = -1;
		} else
		{
			if ((c = _read_kbd(0, 1, 0)) == -1)
				return (READ_INTR);
			if (c == '\0')
			{
				/*
				 * Zero is usually followed by another byte,
				 * since certain keys send two bytes.
				 */
				scan = _read_kbd(0, 0, 0);
			}
		}
		result = 1;
	}
#else
		result = iread(tty, &c, sizeof(char));
		if (result == READ_INTR)
			return (READ_INTR);
		if (result < 0)
		{
			/*
			 * Don't call error() here,
			 * because error calls getchr!
			 */
			quit(QUIT_ERROR);
		}
#endif
#endif
		/*
		 * Various parts of the program cannot handle
		 * an input character of '\0'.
		 * If a '\0' was actually typed, convert it to '\340' here.
		 */
		if (c == '\0')
			c = '\340';
	} while (result != 1);

	return (c & 0377);
}
