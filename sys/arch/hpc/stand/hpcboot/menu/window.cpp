/* -*-C++-*-	$NetBSD: window.cpp,v 1.1 2001/02/09 18:35:03 uch Exp $	*/

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
#include <menu/window.h>

// Windows CE callback function.
LRESULT CALLBACK
Window::_wnd_proc(HWND h, UINT msg, WPARAM param, LPARAM lparam)
{
	Window *w;
	LPCREATESTRUCT s = reinterpret_cast <LPCREATESTRUCT>(lparam);

	// get my context.
	switch(msg) {
	case WM_CREATE:
		w = reinterpret_cast <Window *>(s->lpCreateParams);
		// register this to extra data.
		SetWindowLong(h, 0, reinterpret_cast <LONG>(w));
		break;
	default:
		// refer extra data.
		w = reinterpret_cast <Window *>(GetWindowLong(h, 0));
		break;
	}

	// dispatch window procedure.
	if ((w == 0) || !w->proc(h, msg, param, lparam))
		return DefWindowProc(h, msg, param, lparam);

	return 0;
}

// Windows CE callback function.
BOOL CALLBACK
Window::_dlg_proc(HWND h, UINT msg, WPARAM param, LPARAM lparam)
{
	Window *w;

	if (msg == WM_INITDIALOG) {
		SetWindowLong(h, DWL_USER, lparam);
		w = reinterpret_cast <Window *>(lparam);
	} else {
		w = reinterpret_cast <Window *>(GetWindowLong(h, DWL_USER));
	}

	// dispatch dialog procedure.
	return w ? w->proc(h, msg, param, lparam) : FALSE;
}

// default procedure.
BOOL
Window::proc(HWND w, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}
