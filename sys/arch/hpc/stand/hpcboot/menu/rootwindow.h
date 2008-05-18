/* -*-C++-*-	$NetBSD: rootwindow.h,v 1.8.76.1 2008/05/18 12:32:01 yamt Exp $	*/

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

#ifndef _HPCBOOT_ROOTWINDOW_H_
#define	_HPCBOOT_ROOTWINDOW_H_

class TabWindow;
class TabBaseWindow;

class RootWindow : public Window {
public:
	BootButton	*_boot_button;
	CancelButton	*_cancel_button;
	ProgressBar	*_progress_bar;
	TabWindowBase	*_base;
	TabWindow	*_main;
	TabWindow	*_option;
	TabWindow	*_console;

	int _button_width;
	int _button_height;

private:
	HWND _saved_focus;
	void SaveFocus();
	void RestoreFocus();
	void WMCreate(HWND, LPCREATESTRUCT);
	void WMPaint(HWND, LPCREATESTRUCT);

public:
	RootWindow(HpcBootApp &);
	virtual ~RootWindow(void);
	virtual BOOL create(LPCREATESTRUCT);
	virtual BOOL proc(HWND, UINT, WPARAM, LPARAM);

	void disableTimer(void);
	BOOL isDialogMessage(MSG &);
	BOOL focusManagerHook(MSG &, HWND);

	void progress(const char * = NULL);
	void unprogress();
};

class BootButton : public Window
{
private:
	RootWindow &_root;
public:
	BootButton(HpcBootApp &app, RootWindow &root, RECT &rect)
		: Window(app, root._window), _root(root) {
		_rect = rect;
	}
	virtual ~BootButton(void) { /* NO-OP */ }
	virtual BOOL create(LPCREATESTRUCT aux);
};

class CancelButton : public Window
{
private:
	RootWindow &_root;
public:
	CancelButton(HpcBootApp &app, RootWindow &root, RECT &rect)
		: Window(app, root._window), _root(root) {
		_rect = rect;
	}
	virtual ~CancelButton(void) { /* NO-OP */ }
	virtual BOOL create(LPCREATESTRUCT aux);
};

class ProgressBar : public Window
{
private:
	RootWindow &_root;
public:
	ProgressBar(HpcBootApp &app, RootWindow &root, RECT &rect)
		: Window(app, root._window), _root(root) {
		_rect = rect;
	}
	virtual ~ProgressBar(void) { /* NO-OP */ }
	virtual BOOL create(LPCREATESTRUCT aux);
};

#endif // _HPCBOOT_TABWINDOW_H_
