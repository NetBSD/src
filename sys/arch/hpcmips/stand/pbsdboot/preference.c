/*	$NetBSD: preference.c,v 1.2 2000/01/16 03:07:33 takemura Exp $	*/

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
#include <commctrl.h>
#include <res/resource.h>

struct preference_s pref;
TCHAR* where_pref_load_from = NULL;
static TCHAR filenamebuf[1024];

void
pref_init(struct preference_s* pref)
{
	memset(pref, 0, sizeof(*pref));
}


void
pref_dump(struct preference_s* pref)
{
	debug_printf(TEXT("    kernel_name: %s\n"), pref->kernel_name);
	debug_printf(TEXT("        options: %s\n"), pref->options);
	debug_printf(TEXT("  user def name: %s\n"), pref->setting_name);
	debug_printf(TEXT("  setting index: %d\n"), pref->setting_idx);
	debug_printf(TEXT("           type: %d\n"), pref->fb_type);
	debug_printf(TEXT("          width: %d\n"), pref->fb_width);
	debug_printf(TEXT("         height: %d\n"), pref->fb_height);
	debug_printf(TEXT("     bytes/line: %d\n"), pref->fb_linebytes);
	debug_printf(TEXT("           addr: %d\n"), pref->fb_addr);
	debug_printf(TEXT("            cpu: %08lx\n"), pref->platid_cpu);
	debug_printf(TEXT("        machine: %08lx\n"), pref->platid_machine);
	debug_printf(TEXT("    last chance: %S\n"), pref->check_last_chance ?
		     "TRUE" : "FALSE");
	debug_printf(TEXT("load debug info: %S\n"), pref->load_debug_info ?
		     "TRUE" : "FALSE");
	debug_printf(TEXT("    serial port: %S\n"), pref->serial_port ?
		     "ON" : "OFF");
}


int
pref_read(TCHAR* filename, struct preference_s* pref)
{
	HANDLE file;
	DWORD n;
	struct preference_s buf;

       	file = CreateFile(
	       	filename,      	/* file name */
	       	GENERIC_READ,	/* access (read-write) mode */
	       	FILE_SHARE_READ,/* share mode */
	       	NULL,		/* pointer to security attributes */
	       	OPEN_EXISTING,	/* how to create */
	       	FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
	       	NULL		/* handle to file with attributes to */
	       	);

	if (file == INVALID_HANDLE_VALUE) {
		return (-1);
	}

	if (!ReadFile(file, &buf, sizeof(buf), &n, NULL)) {
		msg_printf(MSG_ERROR, TEXT("pref_load()"),
			   TEXT("ReadFile(): error=%d"), GetLastError());
		debug_printf(TEXT("ReadFile(): error=%d\n"), GetLastError());
		CloseHandle(file);
		return (-1);
	}

	if (n != sizeof(buf)) {
		msg_printf(MSG_ERROR, TEXT("pref_load()"),
			   TEXT("ReadFile(): read %d bytes"), n);
		debug_printf(TEXT("ReadFile(): read %d bytes\n"), n);
		CloseHandle(file);
		return (-1);
	}

	CloseHandle(file);

	*pref = buf;

	return (0);
}


int
pref_load(TCHAR* load_path[], int pathlen)
{
	int i;

	where_pref_load_from = NULL;
	for (i = 0; i < pathlen; i++) {
		wsprintf(filenamebuf, TEXT("%s%s"),
			 load_path[i], PREFNAME);
		debug_printf(TEXT("pref_load: try to '%s'\n"), filenamebuf);
		if (pref_read(filenamebuf, &pref) == 0) {
			debug_printf(TEXT("pref_load: succeded, '%s'.\n"),
				     filenamebuf);
			pref_dump(&pref);
			where_pref_load_from = filenamebuf;
			return (0);
		}
	}

	return (-1);
}


int
pref_write(TCHAR* filename, struct preference_s* buf)
{
	HANDLE file;
	DWORD n;

	debug_printf(TEXT("pref_write('%s').\n"), filename);
	pref_dump(&pref);

       	file = CreateFile(
	       	filename,      	/* file name */
	       	GENERIC_WRITE,	/* access (read-write) mode */
	       	FILE_SHARE_WRITE,/* share mode */
	       	NULL,		/* pointer to security attributes */
	       	CREATE_ALWAYS,	/* how to create */
	       	FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
	       	NULL		/* handle to file with attributes to */
	       	);

	if (file == INVALID_HANDLE_VALUE) {
		msg_printf(MSG_ERROR, TEXT("pref_write()"),
			   TEXT("CreateFile(): error=%d"), GetLastError());
		debug_printf(TEXT("CreateFile(): error=%d\n"), GetLastError());
		return (-1);
	}

	if (!WriteFile(file, buf, sizeof(*buf), &n, NULL)) {
		msg_printf(MSG_ERROR, TEXT("pref_write()"),
			   TEXT("WriteFile(): error=%d"), GetLastError());
		debug_printf(TEXT("WriteFile(): error=%d\n"), GetLastError());
		CloseHandle(file);
		return (-1);
	}

	if (n != sizeof(*buf)) {
		msg_printf(MSG_ERROR, TEXT("pref_write()"),
			   TEXT("WriteFile(): write %d bytes"), n);
		debug_printf(TEXT("WriteFile(): write %d bytes\n"), n);
		CloseHandle(file);
		return (-1);
	}

	CloseHandle(file);
	return (0);
}

