/*	$NetBSD: layout.c,v 1.2 2000/01/16 03:07:32 takemura Exp $	*/

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


int
CreateMainWindow(HINSTANCE hInstance, HWND hWnd, LPCTSTR name, int cmdbar_height)
{
	HRSRC res;
	unsigned char *mem;
	int i;
	DLGTEMPLATE dlg;
	DLGITEMTEMPLATE item;
	RECT rect;
	int ratio_x, ratio_y;

	res = FindResource(hInstance, name, RT_DIALOG);
	if (res == NULL) {
		debug_printf(TEXT("error=%d\n"), GetLastError());
	} 
	mem = (unsigned char*)LockResource(LoadResource(NULL, res));

	/*
	 *	DLGTEMPLATE structure
	 */
	dlg = *(DLGTEMPLATE*)mem;
	mem += sizeof(DLGTEMPLATE);
		
	GetClientRect(hWnd, &rect);
	rect.top += cmdbar_height; /* get client rect w/o command bar */
	ratio_x = (rect.right - rect.left) * 100 / dlg.cx;
	ratio_y = (rect.bottom - rect.top) * 100 / dlg.cy;

	/*
	 *  menu resource
	 */
	if (*(WORD*)mem == 0xffff) {
		/* predefined menu */
		mem += sizeof(WORD);
		debug_printf(TEXT("Dlg: menu=%04x\n"), *(WORD*)mem);
		mem += sizeof(WORD);
	} else
	if (*(WORD*)mem == 0x0000) {
		/* no menu */
		mem += sizeof(WORD);
		debug_printf(TEXT("Dlg: menu=none\n"));
	} else {
		/* menu resource name */
		debug_printf(TEXT("Dlg: menu=%s\n"), (TCHAR*)mem);
		while (*(WORD*)mem) {	/* zero terminated */
			mem += sizeof(WORD);
		}
		mem += sizeof(WORD);
	}

	/*
	 *  window class
	 */
	if (*(WORD*)mem == 0xffff) {
		/* predefined class */
		mem += sizeof(WORD);
		debug_printf(TEXT("Dlg: class=%04x\n"), *(WORD*)mem);
		mem += sizeof(WORD);
	} else
	if (*(WORD*)mem == 0x0000) {
		/* default class */
		mem += sizeof(WORD);
		debug_printf(TEXT("Dlg: class=none\n"));
	} else {
		/* class name */
		debug_printf(TEXT("Dlg: class=%s\n"), (TCHAR*)mem);
		while (*(WORD*)mem) {	/* zero terminated */
			mem += sizeof(WORD);
		}
		mem += sizeof(WORD);
	}

	/*
	 *  window title
	 */
	debug_printf(TEXT("Dlg: title=%s\n"), (TCHAR*)mem);
	while (*(WORD*)mem) {	/* zero terminated */
		mem += sizeof(WORD);
	}
	mem += sizeof(WORD);

	if (dlg.style & DS_SETFONT) {
		/* font size */
		debug_printf(TEXT("Dlg: font size=%d\n"), *(WORD*)mem);
		mem += sizeof(WORD);
		/* font name */
		debug_printf(TEXT("Dlg: font name=%s ("), (TCHAR*)mem);
		while (*(WORD*)mem) {	/* zero terminated */
			debug_printf(TEXT("%04x"), *(WORD*)mem);
			mem += sizeof(WORD);
		}
		debug_printf(TEXT(")\n"));
		mem += sizeof(WORD);
	}

	/*
	 *  for each control
	 */
	for (i = 0; i < dlg.cdit; i++) {
		TCHAR *class_name = NULL;
		TCHAR *window_text = NULL;

		/* DWORD alignment */
		if ((long)mem % sizeof(DWORD)) {
			mem = (unsigned char*)(((long)mem / sizeof(DWORD) + 1) * sizeof(DWORD));
		}

		/*
		 *	DLGITEMTEMPLATE structure
		 */
		item = *(DLGITEMTEMPLATE*)mem;
		mem += sizeof(DLGITEMTEMPLATE);

		/*
		 *  control class
		 */
		if (*(WORD*)mem == 0xffff) {
			/* predefined system class */
			mem += sizeof(WORD);
			switch (*(WORD*)mem) {
			case 0x0080:	class_name = TEXT("BUTTON");	break;
			case 0x0081:	class_name = TEXT("EDIT");		break;
			case 0x0082:	class_name = TEXT("STATIC");	break;
			case 0x0083:	class_name = TEXT("LISTBOX");	break;
			case 0x0084:	class_name = TEXT("SCROLLBAR");	break;
			case 0x0085:	class_name = TEXT("COMBOBOX");	break;
			default:
				debug_printf(TEXT("class=%04x "), *(WORD*)mem);
				break;
			}
			mem += sizeof(WORD);
		} else {
			/* class name */
			class_name = (TCHAR*)mem;
			while (*(WORD*)mem) {	/* zero terminated */
				mem += sizeof(WORD);
			}
			mem += sizeof(WORD);
		}

		/*
		 *  window contents
		 */
		if (*(WORD*)mem == 0xffff) {
			/* resource */
			mem += sizeof(WORD);
			debug_printf(TEXT("contents=%04x "), *(WORD*)mem);
			mem += sizeof(WORD);
		} else {
			/* text */
			window_text = (TCHAR*)mem;
			while (*(WORD*)mem) {	/* zero terminated */
				mem += sizeof(WORD);
			}
			mem += sizeof(WORD);
		}
		if (item.id == 0xffff) {
			item.id = i + 1;
		}

		if (class_name) {
			debug_printf(TEXT("Control: %04x "), item.id);
			debug_printf(TEXT("class=%s "), class_name);
			debug_printf(TEXT("contents=%s "), 
					window_text ? window_text : TEXT(""));

			CreateWindowEx(
				item.dwExtendedStyle,
				class_name,						// Class
				window_text,					// Title                
				item.style,						// Style                
				item.x * ratio_x / 100,
				item.y * ratio_y / 100 + cmdbar_height,
				item.cx * ratio_x / 100,
				item.cy * ratio_y / 100,
				hWnd,							// Parent handle
				(HMENU)item.id,					// Control ID
				hInstance,						// Instance handle
				NULL);							// Creation
		}

#if 0
		/* DWORD alignment */
		if ((long)mem % sizeof(DWORD)) {
			//mem = (unsigned char*)(((long)mem / sizeof(DWORD) + 1) * sizeof(DWORD));
		}
#endif

		/*
		 *  creation data
		 */
		debug_printf(TEXT("data=0x%x bytes\n"), *(WORD*)mem);
		mem += *(WORD*)mem;
		mem += sizeof(WORD);
	}

	return (0);
}
