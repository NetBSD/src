/* -*-C++-*-	$NetBSD: menu.cpp,v 1.3 2001/05/17 01:50:36 enami Exp $	*/

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
#include <hpcboot.h>
#include <res/resource.h>
#include <menu/window.h>
#include <menu/tabwindow.h>
#include <menu/rootwindow.h>
#include <machine/bootinfo.h>
#include <framebuffer.h>
#include <console.h>

#include <menu/menu.h>

TabWindow *
TabWindowBase::boot(int id)
{
	TabWindow *w = NULL;
	HpcMenuInterface &menu = HPC_MENU;

	switch(id) {
	default:
		break;
	case IDC_BASE_MAIN:
		menu._main = new MainTabWindow(*this, IDC_BASE_MAIN);
		w = menu._main;
		break;
	case IDC_BASE_OPTION:
		menu._option = new OptionTabWindow(*this, IDC_BASE_OPTION);
		w = menu._option;
		break;
	case IDC_BASE_CONSOLE:
		menu._console = new ConsoleTabWindow(*this, IDC_BASE_CONSOLE);
		w = menu._console;
		break;
	}

	if (w)
		w->create(0);

	return w;
}

//
// Main window
//
void
MainTabWindow::_insert_item(HWND w, TCHAR *name, int id)
{
	int idx = SendDlgItemMessage(w, id, CB_ADDSTRING, 0,
	    reinterpret_cast <LPARAM>(name));
	if (idx != CB_ERR)
		SendDlgItemMessage(w, IDC_MAIN_DIR, CB_SETITEMDATA,
		    idx, _item_idx++);
}

void
MainTabWindow::init(HWND w)
{
	HpcMenuInterface &menu = HPC_MENU;
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	_window = w;
	// insert myself to tab-control
	TabWindow::init(w);

	// setup child.
	TCHAR *entry;
	int i;
	// kernel directory path
	for (i = 0; entry = menu.dir(i); i++)
		_insert_item(w, entry, IDC_MAIN_DIR);
	SendDlgItemMessage(w, IDC_MAIN_DIR, CB_SETCURSEL, menu.dir_default(),
	    0);
	// platform
	for (i = 0; entry = menu.platform_get(i); i++)
		_insert_item(w, entry, IDC_MAIN_PLATFORM);
	SendDlgItemMessage(w, IDC_MAIN_PLATFORM, CB_SETCURSEL,
	    menu.platform_default(), 0);
	// kernel file name.
	Edit_SetText(GetDlgItem(w, IDC_MAIN_KERNEL), pref.kernel_user ?
	    pref.kernel_user_file : TEXT("netbsd.gz"));

	// root file system.
	int fs = pref.rootfs + IDC_MAIN_ROOT_;
	_set_check(fs, TRUE);

	_edit_md_root = GetDlgItem(w, IDC_MAIN_ROOT_MD_OPS);
	Edit_SetText(_edit_md_root, pref.rootfs_file);
	EnableWindow(_edit_md_root, fs == IDC_MAIN_ROOT_MD ? TRUE : FALSE);

	// kernel boot options.
	_set_check(IDC_MAIN_OPTION_A, pref.boot_ask_for_name);
	_set_check(IDC_MAIN_OPTION_D, pref.boot_debugger);
	_set_check(IDC_MAIN_OPTION_S, pref.boot_single_user);
	_set_check(IDC_MAIN_OPTION_V, pref.boot_verbose);
	_set_check(IDC_MAIN_OPTION_H, pref.boot_serial);

	// serial console speed.
	TCHAR *speed_tab[] = { L"9600", L"19200", L"115200", 0 };
	int sel = 0;
	i = 0;
	for (TCHAR **speed = speed_tab; *speed; speed++, i++) {
		_insert_item(w, *speed, IDC_MAIN_OPTION_H_SPEED);
		if (_wtoi(*speed) == pref.serial_speed)
			sel = i;
	}
	_combobox_serial_speed = GetDlgItem(_window, IDC_MAIN_OPTION_H_SPEED);
	SendDlgItemMessage(w, IDC_MAIN_OPTION_H_SPEED, CB_SETCURSEL, sel, 0);
	EnableWindow(_combobox_serial_speed, pref.boot_serial);
}

void
MainTabWindow::get()
{
	HpcMenuInterface &menu = HPC_MENU;
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	HWND w = GetDlgItem(_window, IDC_MAIN_DIR);
	ComboBox_GetText(w, pref.dir_user_path, MAX_PATH);
	pref.dir_user = TRUE;
	w = GetDlgItem(_window, IDC_MAIN_KERNEL);
	Edit_GetText(w, pref.kernel_user_file, MAX_PATH);
	pref.kernel_user = TRUE;

	int i = ComboBox_GetCurSel(GetDlgItem(_window, IDC_MAIN_PLATFORM));
	menu.platform_set(i);

	if (_is_checked(IDC_MAIN_ROOT_WD))
		pref.rootfs = 0;
	else if (_is_checked(IDC_MAIN_ROOT_SD))
		pref.rootfs = 1;
	else if (_is_checked(IDC_MAIN_ROOT_MD))
		pref.rootfs = 2;
	else if (_is_checked(IDC_MAIN_ROOT_NFS))
		pref.rootfs = 3;

	pref.boot_ask_for_name	= _is_checked(IDC_MAIN_OPTION_A);
	pref.boot_debugger	= _is_checked(IDC_MAIN_OPTION_D);
	pref.boot_verbose	= _is_checked(IDC_MAIN_OPTION_V);
	pref.boot_single_user	= _is_checked(IDC_MAIN_OPTION_S);
	pref.boot_serial	= _is_checked(IDC_MAIN_OPTION_H);
	Edit_GetText(_edit_md_root, pref.rootfs_file, MAX_PATH);

	TCHAR tmpbuf[8];
	ComboBox_GetText(_combobox_serial_speed, tmpbuf, 8);
	pref.serial_speed = _wtoi(tmpbuf);
}

void
MainTabWindow::command(int id, int msg)
{
	switch (id) {
	case IDC_MAIN_OPTION_H:
		EnableWindow(_combobox_serial_speed,
		    _is_checked(IDC_MAIN_OPTION_H));
		break;
	case IDC_MAIN_ROOT_WD:
		/* FALLTHROUGH */
	case IDC_MAIN_ROOT_SD:
		/* FALLTHROUGH */
	case IDC_MAIN_ROOT_MD:
		/* FALLTHROUGH */
	case IDC_MAIN_ROOT_NFS:
		EnableWindow(_edit_md_root, _is_checked(IDC_MAIN_ROOT_MD));
	}
}

//
// Option window
//
void
OptionTabWindow::init(HWND w)
{
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	_window = w;

	TabWindow::init(_window);
	_spin_edit = GetDlgItem(_window, IDC_OPT_AUTO_INPUT);
	_spin = CreateUpDownControl(WS_CHILD | WS_BORDER | WS_VISIBLE |
	    UDS_SETBUDDYINT | UDS_ALIGNRIGHT, 80, 0, 50, 50, _window,
	    IDC_OPT_AUTO_UPDOWN, _app._instance, _spin_edit, 60, 1, 30);
	BOOL onoff = pref.auto_boot ? TRUE : FALSE;
	EnableWindow(_spin_edit, onoff);
	EnableWindow(_spin, onoff);

	SET_CHECK(AUTO, pref.auto_boot);
	if (pref.auto_boot) {
		TCHAR tmp[32];
		wsprintf(tmp, TEXT("%d"), pref.auto_boot);
		Edit_SetText(_spin_edit, tmp);
	}
	SET_CHECK(VIDEO,	pref.reverse_video);
	SET_CHECK(PAUSE,	pref.pause_before_boot);
	SET_CHECK(DEBUG,	pref.load_debug_info);
	SET_CHECK(SAFETY,	pref.safety_message);
}

void
OptionTabWindow::command(int id, int msg)
{
	switch (id) {
	case IDC_OPT_AUTO:
		if (IS_CHECKED(AUTO)) {
			EnableWindow(_spin_edit, TRUE);
			EnableWindow(_spin, TRUE);
		} else {
			EnableWindow(_spin_edit, FALSE);
			EnableWindow(_spin, FALSE);
		}
		break;
	}
}

void
OptionTabWindow::get()
{
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;
	if (IS_CHECKED(AUTO)) {
		TCHAR tmp[32];
		Edit_GetText(_spin_edit, tmp, 32);
		pref.auto_boot = _wtoi(tmp);
	} else
		pref.auto_boot = 0;
	pref.reverse_video	= IS_CHECKED(VIDEO);
	pref.pause_before_boot	= IS_CHECKED(PAUSE);
	pref.load_debug_info	= IS_CHECKED(DEBUG);
	pref.safety_message	= IS_CHECKED(SAFETY);
}
#undef IS_CHECKED
#undef SET_CHECK


//
// Console window
//
void
ConsoleTabWindow::print(TCHAR *buf, BOOL force_display)
{
	int cr;
	TCHAR *p;

	if (force_display)
		goto display;

	if (_filesave) {
		if (_logfile == INVALID_HANDLE_VALUE && !_open_log_file()) {
			_filesave = FALSE;
			_set_check(IDC_CONS_FILESAVE, _filesave);
			EnableWindow(_filename_edit, _filesave);
			goto display;
		}
		DWORD cnt;
		char c;
		for (int i = 0; *buf != TEXT('\0'); buf++) {
			c = *buf & 0x7f;
			WriteFile(_logfile, &c, 1, &cnt, 0);
		}
		FlushFileBuffers(_logfile);
		return;
	}

 display:
	// count # of '\n'
	for (cr = 0, p = buf; p = wcschr(p, TEXT('\n')); cr++, p++)
		;
	// total length of new buffer('\n' -> "\r\n" + '\0')
	int ln = wcslen(buf) + cr + 1;

	// get old buffer.
	int lo = Edit_GetTextLength(_edit);
	size_t sz =(lo  + ln) * sizeof(TCHAR);

	p = reinterpret_cast <TCHAR *>(malloc(sz));
	if (p == NULL)
		return;

	memset(p, 0, sz);
	Edit_GetText(_edit, p, lo + 1);

	// put new buffer to end of old buffer.
	TCHAR *d = p + lo;
	while (*buf != TEXT('\0')) {
		TCHAR c = *buf++;
		if (c == TEXT('\n'))
			*d++ = TEXT('\r');
		*d++ = c;
	}
	*d = TEXT('\0');
    
	// display total buffer.
	Edit_SetText(_edit, p);
//  Edit_Scroll(_edit, Edit_GetLineCount(_edit), 0);
	UpdateWindow(_edit);

	free(p);
}

void
ConsoleTabWindow::init(HWND w)
{
	// at this time _window is NULL.
	// use argument of window procedure.
	TabWindow::init(w);
	_edit = GetDlgItem(w, IDC_CONS_EDIT);
	MoveWindow(_edit, 5, 60, _rect.right - _rect.left - 10,
	    _rect.bottom - _rect.top - 60, TRUE);
	Edit_FmtLines(_edit, TRUE);
	
	// log file.
	_filename_edit = GetDlgItem(w, IDC_CONS_FILENAME);
	_filesave = FALSE;
	Edit_SetText(_filename_edit, L"bootlog.txt");
	EnableWindow(_filename_edit, _filesave);
}

void
ConsoleTabWindow::command(int id, int msg)
{
	HpcMenuInterface &menu = HPC_MENU;
	struct HpcMenuInterface::cons_hook_args *hook = 0;
	int bit;

	switch(id) {
	case IDC_CONS_FILESAVE:
		_filesave = _is_checked(IDC_CONS_FILESAVE);
		EnableWindow(_filename_edit, _filesave);
		break;
	case IDC_CONS_CHK0:
		/* FALLTHROUGH */
	case IDC_CONS_CHK1:
		/* FALLTHROUGH */
	case IDC_CONS_CHK2:
		/* FALLTHROUGH */
	case IDC_CONS_CHK3:
		/* FALLTHROUGH */
	case IDC_CONS_CHK4:
		/* FALLTHROUGH */
	case IDC_CONS_CHK5:
		/* FALLTHROUGH */
	case IDC_CONS_CHK6:
		/* FALLTHROUGH */
	case IDC_CONS_CHK7:
		bit = 1 << (id - IDC_CONS_CHK_);
		if (SendDlgItemMessage(_window, id, BM_GETCHECK, 0, 0))
			menu._cons_parameter |= bit;
		else
			menu._cons_parameter &= ~bit;
		break;
	case IDC_CONS_BTN0:
		/* FALLTHROUGH */
	case IDC_CONS_BTN1:
		/* FALLTHROUGH */
	case IDC_CONS_BTN2:
		/* FALLTHROUGH */
	case IDC_CONS_BTN3:
		hook = &menu._cons_hook[id - IDC_CONS_BTN_];
		if (hook->func)
			hook->func(hook->arg, menu._cons_parameter);
			
		break;
	}
}

BOOL
ConsoleTabWindow::_open_log_file()
{
	TCHAR path[MAX_PATH];
	TCHAR filename[MAX_PATH];
	TCHAR filepath[MAX_PATH];
	
	if (!_find_pref_dir(path)) {
		print(L"couldn't find temporary directory.\n", TRUE);
		return FALSE;
	}

	Edit_GetText(_filename_edit, filename, MAX_PATH);
	wsprintf(filepath, TEXT("\\%s\\%s"), path, filename);
	_logfile = CreateFile(filepath, GENERIC_WRITE, 0, 0,
	    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (_logfile == INVALID_HANDLE_VALUE)
		return FALSE;

	wsprintf(path, TEXT("log file is %s\n"), filepath);
	print(path, TRUE);
	
	return TRUE;
}

//
// Common utility
//
BOOL
_find_pref_dir(TCHAR *path)
{
	WIN32_FIND_DATA fd;
	HANDLE find;

	lstrcpy(path, TEXT("\\*.*"));
	find = FindFirstFile(path, &fd);

	if (find != INVALID_HANDLE_VALUE) {
		do {
			int attr = fd.dwFileAttributes;
			if ((attr & FILE_ATTRIBUTE_DIRECTORY) &&
			    (attr & FILE_ATTRIBUTE_TEMPORARY)) {
				wcscpy(path, fd.cFileName);
				FindClose(find);
				return TRUE;
			}
		} while (FindNextFile(find, &fd));
	}
	FindClose(find);

	return FALSE;
}
