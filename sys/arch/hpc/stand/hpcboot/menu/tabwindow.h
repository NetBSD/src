/* -*-C++-*-	$NetBSD: tabwindow.h,v 1.2 2001/05/08 18:51:24 uch Exp $	*/

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

#ifndef _HPCBOOT_TABWINDOW_H_
#define _HPCBOOT_TABWINDOW_H_

class TabWindow;

class TabWindowBase : public Window {
public:
	RECT	_rect;
	int	_id;

private:
	void _load_bitmap(HIMAGELIST, const TCHAR *);

public:
	explicit TabWindowBase(HpcBootApp &app, HWND parent,
	    RECT &rect, int id)
		: Window(app, parent) {
		_rect = rect;
		_id = id;
	}
	virtual ~TabWindowBase(void) { /* NO-OP */ }

	static LRESULT CALLBACK
	_tab_proc(HWND h, UINT msg, WPARAM param, LPARAM lparam);
	virtual BOOL create(LPCREATESTRUCT aux);

	// setup child instance.
	TabWindow *boot(int id);

	// insert child dialog to me.
	void insert(int id, TC_ITEM &item) {
		TabCtrl_InsertItem(_window, id, &item);
	}
	// return tab-control region.
	void adjust(RECT &rect) {
		TabCtrl_AdjustRect(_window, FALSE, &rect);
	}
};

class TabWindow : public Window
{
public:
	TabWindowBase &_base;
	RECT	_rect;
	int _id;

protected:
	const TCHAR *_name;

	explicit TabWindow(TabWindowBase &base, int id, const TCHAR *name)
		: Window(base._app, base._window), _base(base), _name(name) {
		_rect = _base._rect;
		_id = id;
	}

	// utility for check box and radio button.
	BOOL _is_checked(int id);
	void _set_check(int id, BOOL onoff);

public:
	virtual ~TabWindow(void) { /* NO-OP */ }

	virtual BOOL proc(HWND w, UINT msg, WPARAM wparam, LPARAM lparam);
	virtual BOOL create(LPCREATESTRUCT unused);
	virtual void init(HWND w);
	virtual void command(int id, int msg) { /* NO-OP */ }

	// adjust my dialog size to tab-control
	void adjust(void) {
		MoveWindow(_window, _rect.left, 0, _rect.right - _rect.left,
		    _rect.bottom - _rect.top, TRUE);
	}
	virtual void hide(void) {
		ShowWindow(_window, SW_HIDE);
	}
	virtual void show(void) {
		adjust();
		ShowWindow(_window, SW_SHOW);
	}
	void update(void) {
		InvalidateRect(_window, &_rect, TRUE);
	}
};
#endif // _HPCBOOT_TABWINDOW_H_
