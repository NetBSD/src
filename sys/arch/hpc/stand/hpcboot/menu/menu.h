/* -*-C++-*-	$NetBSD: menu.h,v 1.3 2004/03/28 15:32:35 uwe Exp $	*/

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

class MainTabWindow : public TabWindow
{
private:
	HWND _edit_md_root;
	HWND _combobox_serial_speed;

	struct PlatMap {
		int id;		// index in platid_name_table
		TCHAR *name;	// platform name
	};

	struct PlatMap *_platmap;
	void _sort_platids(HWND w);
	int _item_to_platid(int idx);
	static int _platcmp(const void *, const void *);

	int _item_idx;
	void _insert_item(HWND w, TCHAR *name, int id);
public:
	explicit MainTabWindow(TabWindowBase &base, int id)
		: TabWindow(base, id, TEXT("WMain")) {
		_item_idx = 0;
	}
	virtual ~MainTabWindow(void) { /* NO-OP */ }
	virtual void init(HWND w);
	virtual void command(int id, int msg);
	void get(void);

	// control layouter.
	void layout(void);
};

class OptionTabWindow : public TabWindow
{
public:
	HWND _spin_edit;
	HWND _spin;
#define IS_CHECKED(x)	_is_checked(IDC_OPT_##x)
#define SET_CHECK(x, b)	_set_check(IDC_OPT_##x,(b))

public:
	explicit OptionTabWindow(TabWindowBase &base, int id)
		: TabWindow(base, id, TEXT("WOption")) {
		_spin_edit = NULL;
		_spin = NULL;
	}
	virtual ~OptionTabWindow(void) { /* NO-OP */ }
	virtual void init(HWND w);
	virtual void command(int id, int msg);
	void get(void);
};

class ConsoleTabWindow : public TabWindow
{
private:
	HWND _filename_edit;
	BOOL _filesave;
	HANDLE _logfile;
	BOOL _open_log_file(void);
public:
	HWND _edit;

public:
	explicit ConsoleTabWindow(TabWindowBase &base, int id)
		: TabWindow(base, id, TEXT("WConsole")) {
		_edit = NULL;
		_logfile = INVALID_HANDLE_VALUE;
	}
	virtual ~ConsoleTabWindow(void) {
		if (_logfile != INVALID_HANDLE_VALUE)
			CloseHandle(_logfile);
	}
	virtual void init(HWND);
	virtual void command(int, int);

	void print(TCHAR *buf, BOOL = FALSE);
};

__BEGIN_DECLS
BOOL _find_pref_dir(TCHAR *);
__END_DECLS
