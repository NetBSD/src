/* -*-C++-*-	$NetBSD: tabwindow.cpp,v 1.2 2001/05/08 18:51:24 uch Exp $	*/

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
#include <menu/tabwindow.h>
#include <res/resource.h>

//
// TabControl
//
BOOL
TabWindowBase::create(LPCREATESTRUCT aux)
{
	int cx = _rect.right - _rect.left;
	int cy = _rect.bottom - _rect.top;
	_window = CreateWindow(WC_TABCONTROL, L"",
	    WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	    WS_CLIPSIBLINGS | TCS_MULTILINE | TCS_VERTICAL,
	    _rect.left, _rect.top, cx, cy, _parent_window,
	    reinterpret_cast <HMENU>(_id), aux->hInstance,
	    NULL); // this is system window class

	if (!IsWindow(_window))
		return FALSE;

	// set tab image.
	HIMAGELIST img = ImageList_Create(TABCTRL_TAB_IMAGE_WIDTH,
	    TABCTRL_TAB_IMAGE_HEIGHT,
	    ILC_COLOR, 3, 0);
	_load_bitmap(img, L"IDI_HPCMENU_MAIN");
	_load_bitmap(img, L"IDI_HPCMENU_OPTION");
	_load_bitmap(img, L"IDI_HPCMENU_CONSOLE");

	TabCtrl_SetPadding(_window, 1, 1);
	TabCtrl_SetImageList(_window, img);

	return TRUE;
}

void
TabWindowBase::_load_bitmap(HIMAGELIST img, const TCHAR *name)
{
	HBITMAP bmp = LoadBitmap(_app._instance, name);
	ImageList_Add(img, bmp, 0);
	DeleteObject(bmp);
}

//
// Child of TabControl(Dialog)
//
BOOL
TabWindow::create(LPCREATESTRUCT unused)
{
	_window = CreateDialogParam 
	    (_app._instance, _name, _base._window,
		reinterpret_cast <DLGPROC>(Window::_dlg_proc),
		reinterpret_cast <LPARAM>(this));

	return _window ? TRUE : FALSE;
}

BOOL
TabWindow::proc(HWND w, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg) {
	default:
		return FALSE;
	case WM_INITDIALOG:
		init(w);
		break;
	case WM_COMMAND:
		command(LOWORD(wparam), HIWORD(wparam));
		break;
	}
	return TRUE;
}

void
TabWindow::init(HWND w)
{
	TC_ITEM item;

	item.mask = TCIF_PARAM | TCIF_IMAGE;
	item.iImage =(int)_id;
	item.lParam = reinterpret_cast <LPARAM>(this);
	// register myself to parent tab-control.
	_base.insert(_id, item);    
	// fit my dialog size to tab-control window.
	_base.adjust(_rect);
	hide();
}

BOOL
TabWindow::_is_checked(int id)
{
	return SendDlgItemMessage(_window, id, BM_GETCHECK, 0, 0)
	    ? TRUE : FALSE;
}

void
TabWindow::_set_check(int id, BOOL onoff)
{
	SendDlgItemMessage(_window, id, BM_SETCHECK,
	    onoff ? BST_CHECKED : BST_UNCHECKED, 0);
}
