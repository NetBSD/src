/* -*-C++-*-	$NetBSD: rootwindow.cpp,v 1.17 2004/04/27 00:04:38 uwe Exp $	*/

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
#include <menu/rootwindow.h>
#include <res/resource.h>
#include "../binary/build_number.h"

//
// root window
//
RootWindow::RootWindow(HpcBootApp &app)
	: Window(app)
{
	_boot_button	= 0;
	_base		= 0;
	_main		= 0;
	_option	= 0;
	_console	= 0;
}

RootWindow::~RootWindow()
{
	if (_boot_button)
		delete _boot_button;
	if (_cancel_button)
		delete _cancel_button;
	if (_progress_bar)
		delete _progress_bar;
	if (_main)
		delete _main;
	if (_option)
		delete _option;
	if (_console)
		delete _console;
	if (_base)
		delete _base;
}

BOOL
RootWindow::create(LPCREATESTRUCT aux)
{
	TCHAR app_name[32];
	// Root window's create don't called by Window Procedure.
	// so aux is NULL
	HINSTANCE inst = _app._instance;
	TCHAR *wc_name = reinterpret_cast <TCHAR *>
	    (LoadString(inst, IDS_HPCMENU, 0, 0));
	wsprintf(app_name, TEXT("%s Build %d"), wc_name, HPCBOOT_BUILD_NUMBER);

	_window = CreateWindow(wc_name, app_name, WS_VISIBLE,
	    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    0, 0, inst, this);
	if (!_window)
		return FALSE;

	HpcMenuInterface &menu = HpcMenuInterface::Instance();
	if (menu._pref.auto_boot > 0)
		SetTimer(_window, IDD_TIMER, menu._pref.auto_boot * 1000, 0);

	ShowWindow(_window, SW_SHOW);
	UpdateWindow(_window);

	return TRUE;
}

BOOL
RootWindow::proc(HWND w, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LPCREATESTRUCT aux = reinterpret_cast <LPCREATESTRUCT>(lparam);
	HpcMenuInterface &menu = HpcMenuInterface::Instance();

	switch(msg) {
	default: // message can't handle.
		return FALSE;
	case WM_CREATE:
		WMCreate(w, aux);
		break;
	case WM_PAINT:
		WMPaint(w, aux);
		break;
	case WM_ENTERMENULOOP:
		SaveFocus();
		break;
	case WM_EXITMENULOOP:
		RestoreFocus();
		break;
	case WM_ACTIVATE:
		if ((UINT)LOWORD(wparam) == WA_INACTIVE)
			SaveFocus();
		else
			RestoreFocus();
		break;
	case WM_NOTIFY:
	{
		NMHDR *notify = reinterpret_cast <NMHDR *>(lparam);
		// get current selected tab id
		int tab_id = TabCtrl_GetCurSel(_base->_window);
		// get context
		TC_ITEM tc_item;
		tc_item.mask = TCIF_PARAM;
		TabCtrl_GetItem(_base->_window, tab_id, &tc_item);
		TabWindow *tab = reinterpret_cast <TabWindow *>
		    (tc_item.lParam);
		switch(notify->code) {
		case TCN_SELCHANGING:
			tab->hide();
			break;
		case TCN_SELCHANGE:
			tab->show();
			break;
		case TCN_KEYDOWN: {
			NMTCKEYDOWN *key = reinterpret_cast
				<NMTCKEYDOWN *>(lparam);
			return _base->focusManagerHook(key->wVKey, key->flags,
						_cancel_button->_window);
		    }
		}
	}
	break;
	case WM_TIMER:
		disableTimer();
		goto boot;
	case WM_COMMAND:
		switch(wparam)
		{
		case IDC_BOOTBUTTON:
			// inquire current options.
			menu.get_options();
			if (menu._pref.safety_message) {
				UINT mb_icon = menu._pref.pause_before_boot ?
					MB_ICONQUESTION : MB_ICONWARNING;
				if (MessageBox(_window,
				    TEXT("Data in memory will be lost.\nAre you sure?"),
				    TEXT("WARNING"),
				    mb_icon | MB_YESNO) != IDYES)
					break;
				UpdateWindow(_window);
			}
		boot:
			SendMessage(_progress_bar->_window, PBM_SETPOS, 0, 0);
			menu.print(TEXT("BOOT START\n"));
			// inquire current options.
			menu.get_options();
			// save options to `hpcboot.cnf'
			menu.save();
			// start boot sequence.
			menu.boot();
			// NOTREACHED
			break;
		case IDC_PROGRESSBAR:
			break;
		case IDC_CANCELBUTTON:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return TRUE;
}

void
RootWindow::SaveFocus() {
	_saved_focus = GetFocus();
}

void
RootWindow::RestoreFocus() {
	SetFocus(IsWindowEnabled(_saved_focus) ?
		 _saved_focus : _boot_button->_window);
}

void
RootWindow::WMPaint(HWND w, LPCREATESTRUCT aux)
{
	PAINTSTRUCT ps;
	BeginPaint(w, &ps);
	EndPaint(w, &ps);
}

void
RootWindow::WMCreate(HWND w, LPCREATESTRUCT aux)
{
	int cmdbar_height;

	_window = w;
	// Command bar.
	_app._cmdbar = CommandBar_Create(aux->hInstance, w, IDC_CMDBAR);
	CommandBar_AddAdornments(_app._cmdbar, 0, 0);
	cmdbar_height = CommandBar_Height(_app._cmdbar);

	_button_height = cmdbar_height;
	_button_width = BOOT_BUTTON_WIDTH;

	HDC hdc = GetDC(0);
	if (GetDeviceCaps(hdc, HORZRES) > 320)
	    _button_width += _button_width/2;
	ReleaseDC(0, hdc);

	RECT rect;
	GetClientRect(w, &rect);
	rect.top += cmdbar_height;

	// BOOT button.
	_boot_button = new BootButton(_app, *this, rect);
	_boot_button->create(aux);
	// CANCEL button.
	_cancel_button = new CancelButton(_app, *this, rect);
	_cancel_button->create(aux);
	// Progress bar
	_progress_bar = new ProgressBar(_app, *this, rect);
	_progress_bar->create(aux);

 	// regsiter myself to menu
	HpcMenuInterface::Instance()._root = this;

	rect.top += _button_height;
	// Tab control.
	_base =  new TabWindowBase(_app, w, rect, IDC_BASE);
	_base->create(aux);
	// main/option/console dialog.(register to Menu)
	_main = _base->boot(IDC_BASE_MAIN);
	_option = _base->boot(IDC_BASE_OPTION);
	_console = _base->boot(IDC_BASE_CONSOLE);

	_main->show();
	SetFocus(_boot_button->_window);

	return;
}

void
RootWindow::disableTimer()
{
	KillTimer(_window, IDD_TIMER);
}

BOOL
RootWindow::isDialogMessage(MSG &msg)
{
	HWND tab_window;

	if (_main && IsWindowVisible(_main->_window))
		tab_window = _main->_window;
	else if (_option && IsWindowVisible(_option->_window))
		tab_window = _option->_window;
	else if (_console && IsWindowVisible(_console->_window))
		tab_window = _console->_window;

	if (focusManagerHook(msg, tab_window))
		return TRUE;

	return IsDialogMessage(tab_window, &msg);
}

//
// XXX !!! XXX !!! XXX !!! XXX !!!
//
// WinCE 2.11 doesn't support keyboard focus traversal for nested
// dialogs, so implement poor man focus manager for our root window.
// This function handles focus transition from boot/cancel buttons.
// Transition from the tab-control is done on WM_NOTIFY/TCN_KEYDOWN
// above.
//
// XXX: This is a very smplistic implementation that doesn't handle
// <TAB> auto-repeat count in LOWORD(msg.lParam), WS_GROUP, etc...
//
BOOL
RootWindow::focusManagerHook(MSG &msg, HWND tab_window)
{
	HWND next, prev;
	HWND dst = 0;
	LRESULT dlgcode = 0;

	if (msg.message != WM_KEYDOWN)
		return FALSE;

	if (msg.hwnd == _boot_button->_window) {
		next = _cancel_button->_window;
		prev = _base->_window;
	} else if (msg.hwnd == _cancel_button->_window) {
		next = _base->_window;
		prev = _boot_button->_window;
	} else if (tab_window == 0) {
		return FALSE;
	} else {
		// last focusable control in the tab_window (XXX: WS_GROUP?)
		HWND last = GetNextDlgTabItem(tab_window, NULL, TRUE);
		if (last == NULL ||
		    !(last == msg.hwnd || IsChild(last, msg.hwnd)))
			return FALSE;
		dlgcode = SendMessage(last, WM_GETDLGCODE, NULL, (LPARAM)&msg);
		next = _base->_window; // out of the tab window
		prev = 0;	// let IsDialogMessage handle it
	}
 
#if 0 // XXX: breaks tabbing out of the console window
	if (dlgcode & DLGC_WANTALLKEYS)
		return FALSE;
#endif
	switch (msg.wParam) {
	case VK_RIGHT:
	case VK_DOWN:
		if (dlgcode & DLGC_WANTARROWS)
			return FALSE;
		dst = next;
		break;

	case VK_LEFT:
	case VK_UP:
		if (dlgcode & DLGC_WANTARROWS)
			return FALSE;
		dst = prev;
		break;

	case VK_TAB:
		if (dlgcode & DLGC_WANTTAB)
			return FALSE;
		if (GetKeyState(VK_SHIFT) & 0x8000) // Shift-Tab
			dst = prev;
		else
			dst = next;
		break;
	}

	if (dst == 0)
		return FALSE;

	SetFocus(dst);
	return TRUE;
}

void
RootWindow::progress()
{
	SendMessage(_progress_bar->_window, PBM_STEPIT, 0, 0);
}

void
RootWindow::unprogress()
{
	SendMessage(_progress_bar->_window, PBM_SETPOS, 0, 0);
}

//
// BOOT button
//
BOOL
BootButton::create(LPCREATESTRUCT aux)
{
	int cx = _root._button_width;
	int cy = _root._button_height;

	_window = CreateWindow(TEXT("BUTTON"), TEXT("Boot"),
	    BS_PUSHBUTTON | BS_NOTIFY |
	    WS_VISIBLE | WS_CHILD | WS_TABSTOP,
	    _rect.left, _rect.top, cx, cy, _parent_window,
	    reinterpret_cast <HMENU>(IDC_BOOTBUTTON),
	    aux->hInstance,
	    NULL);

	return IsWindow(_window) ? TRUE : FALSE;
}

//
// CANCEL button
//
BOOL
CancelButton::create(LPCREATESTRUCT aux)
{
	int cx = _root._button_width;
	int cy = _root._button_height;
	int x = _rect.right - _root._button_width;

	_window = CreateWindow(TEXT("BUTTON"), TEXT("Cancel"),
	    BS_PUSHBUTTON | BS_NOTIFY | WS_TABSTOP |
	    WS_VISIBLE | WS_CHILD,
	    x, _rect.top, cx, cy, _parent_window,
	    reinterpret_cast <HMENU>(IDC_CANCELBUTTON),
	    aux->hInstance,
	    NULL);

	return IsWindow(_window) ? TRUE : FALSE;
}

//
// PROGRESS BAR
//
BOOL
ProgressBar::create(LPCREATESTRUCT aux)
{
	int cx = _rect.right - _rect.left - _root._button_width * 2;
	int cy = _root._button_height;
	int x = _rect.left + _root._button_width;
	_window = CreateWindowEx(WS_EX_CLIENTEDGE,
	    PROGRESS_CLASS, TEXT(""),
	    PBS_SMOOTH | WS_VISIBLE | WS_CHILD,
	    x, _rect.top, cx, cy, _parent_window,
	    reinterpret_cast <HMENU>(IDC_PROGRESSBAR),
	    aux->hInstance, NULL);
	SendMessage(_window, PBM_SETRANGE, 0, MAKELPARAM(0, 11));
	SendMessage(_window, PBM_SETSTEP, 1, 0);
	SendMessage(_window, PBM_SETPOS, 0, 0);

	return IsWindow(_window) ? TRUE : FALSE;
}
