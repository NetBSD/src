/* -*-C++-*-	$NetBSD: rootwindow.cpp,v 1.1.2.3 2001/03/12 13:28:17 bouyer Exp $	*/

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
	// Root window's create don't called by Window Procedure.
	// so aux is NULL
	HINSTANCE inst = _app._instance;
	TCHAR *wc_name = reinterpret_cast <TCHAR *>
		(LoadString(inst, IDS_HPCMENU, 0, 0));
    
	_window = CreateWindow(wc_name, wc_name, WS_VISIBLE,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       CW_USEDEFAULT, CW_USEDEFAULT,
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
				if (MessageBox
				    (_window,
				     TEXT("Data in memory will be lost.\n Are you sure?"),
				     TEXT("WARNING"), MB_YESNO) != IDYES)
					break;
			}
		boot:
			SendMessage(_progress_bar->_window, PBM_SETPOS, 0, 0);
			menu.print(TEXT("BOOT START\n"));
			// inquire current options.
			menu.get_options();
			// save options to `hpcboot.cnf'
			menu.save();
			// atart boot sequence.
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
	SendMessage(_progress_bar->_window, PBM_SETSTEP, 1, 0);
	SendMessage(_progress_bar->_window, PBM_SETPOS, 0, 0);

 	// regsiter myself to menu
	HpcMenuInterface::Instance()._root = this;

	rect.top += cmdbar_height;
	// Tab control.
	_base =  new TabWindowBase(_app, w, rect, IDC_BASE);
	_base->create(aux);
	// main/option/console dialog.(register to Menu)
	_main = _base->boot(IDC_BASE_MAIN);
	_option = _base->boot(IDC_BASE_OPTION);
	_console = _base->boot(IDC_BASE_CONSOLE);
  
	_main->show();

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
	if (_main && IsWindowVisible(_main->_window))
		return IsDialogMessage(_main->_window, &msg);
	if (_option && IsWindowVisible(_option->_window))
		return IsDialogMessage(_option->_window, &msg);
	if (_console && IsWindowVisible(_console->_window))
		return IsDialogMessage(_console->_window, &msg);
	return FALSE;
}

//
// BOOT button
//
BOOL
BootButton::create(LPCREATESTRUCT aux)
{
	int cx = BOOT_BUTTON_WIDTH;
	int cy = _root._button_height;

	_window = CreateWindow(TEXT("BUTTON"), TEXT("BOOT"),
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
	int cx = BOOT_BUTTON_WIDTH;
	int cy = _root._button_height;
	int x = _rect.right - BOOT_BUTTON_WIDTH;

	_window = CreateWindow(TEXT("BUTTON"), TEXT("CANCEL"),
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
	int cx = _rect.right - _rect.left
		- TABCTRL_TAB_WIDTH - BOOT_BUTTON_WIDTH * 2;
	int cy = _root._button_height;
	int x = _rect.left + BOOT_BUTTON_WIDTH;
	_window = CreateWindow(PROGRESS_CLASS, TEXT(""),
			       WS_VISIBLE | WS_CHILD,
			       x, _rect.top, cx, cy, _parent_window,
			       reinterpret_cast <HMENU>(IDC_PROGRESSBAR),
			       aux->hInstance, NULL);
	SendMessage(_window, PBM_SETRANGE, 0, MAKELPARAM(0, 12));

	return IsWindow(_window) ? TRUE : FALSE;
}
