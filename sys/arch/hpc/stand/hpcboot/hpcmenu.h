/* -*-C++-*-	$NetBSD: hpcmenu.h,v 1.7.8.2 2002/04/01 07:40:15 nathanw Exp $	*/

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

#ifndef _HPCBOOT_MENU_H_
#define _HPCBOOT_MENU_H_

#include <hpcdefs.h>

// forward declaration.
class Console;
class HpcBootApp;	
class RootWindow;
class BootButton;
class CancelButton;
class ProgressBar;
class TabWindowBase;
class MainTabWindow;
class OptionTabWindow;
class ConsoleTabWindow;
struct bootinfo;

// Application
class HpcBootApp {
public:
	HINSTANCE	_instance;
	HWND		_cmdbar;
	RootWindow	*_root;
	Console		*_cons;
	int		_cx_char, _cy_char; // 5, 14

private:
	void _get_font_size(void) {
		HDC hdc = GetDC(0);
		TEXTMETRIC tm;
		SelectObject(hdc, GetStockObject(SYSTEM_FONT));
		GetTextMetrics(hdc, &tm);
		_cx_char = tm.tmAveCharWidth;
		_cy_char = tm.tmHeight + tm.tmExternalLeading;
		ReleaseDC(0, hdc);
	}

public:
	explicit HpcBootApp(HINSTANCE instance) : _instance(instance) {
		_root	= 0;
		_cmdbar	= 0;
		_get_font_size();
	}
	virtual ~HpcBootApp(void) { /* NO-OP */ }

	BOOL registerClass(WNDPROC proc);
	int run(void);
};

// internal representation of user input.
class HpcMenuInterface
{
public:
	struct HpcMenuPreferences {
#define HPCBOOT_MAGIC		0x177d5753
		int		_magic;
		int		_version;
		size_t	_size;	// size of HpcMenuPreferences structure.
		int		dir;
		BOOL	dir_user;
		TCHAR	dir_user_path[MAX_PATH];
		BOOL	kernel_user;
		TCHAR	kernel_user_file[MAX_PATH];
		unsigned	platid_hi;
		unsigned	platid_lo;
		int		rootfs;
		TCHAR	rootfs_file[MAX_PATH];
		// kernel options.
		BOOL	boot_serial;
		BOOL	boot_verbose;
		BOOL	boot_single_user;
		BOOL	boot_ask_for_name;
		BOOL	boot_debugger;
		// boot loader options.
		int		auto_boot;
		BOOL	reverse_video;
		BOOL	pause_before_boot;
		BOOL	load_debug_info;
		BOOL	safety_message;
		// serial console speed
		int	serial_speed;
#define MAX_BOOT_STR 256
		TCHAR	boot_extra[MAX_BOOT_STR];
	};
	struct support_status {
		u_int32_t cpu, machine;
		const TCHAR *cause;
	};
	static struct support_status _unsupported[];

	RootWindow		*_root;
	MainTabWindow		*_main;
	OptionTabWindow		*_option;
	ConsoleTabWindow	*_console;
	struct HpcMenuPreferences _pref;

	struct boot_hook_args {
		void(*func)(void *);
		void *arg;
	} _boot_hook;

	struct cons_hook_args {
		void(*func)(void *, unsigned char);
		void *arg;
	} _cons_hook [4];
	int _cons_parameter; // Console tab window check buttons.

private:
	static HpcMenuInterface *_instance;

	void _set_default_pref(void);
	enum _platform_op {
		_PLATFORM_OP_GET,
		_PLATFORM_OP_SET,
		_PLATFORM_OP_DEFAULT
	};
	void *_platform(int, enum _platform_op);

protected:
	HpcMenuInterface(void);
	virtual ~HpcMenuInterface(void) { /* NO-OP */ }

public:
	static HpcMenuInterface &Instance(void);
	static void Destroy(void);

	// preferences.
	BOOL load(void);
	BOOL save(void);
  
	// Boot button
	// when user click `boot button' inquires all options.
	void get_options(void);
	enum { MAX_KERNEL_ARGS = 16 };
	int setup_kernel_args(vaddr_t, paddr_t, size_t);
	void setup_bootinfo(struct bootinfo &bi);
	void register_boot_hook(struct boot_hook_args &arg) {
		_boot_hook = arg;
	}
	// call architecture dependent boot function.
	void boot(void);
	// Progress bar.
	void progress(void);

	// Console window interface.
	void print(TCHAR *);
	void register_cons_hook(struct cons_hook_args &arg, int id) {
		if (id >= 0 && id < 4)
			_cons_hook[id] = arg;
	}

	// Main window options
	TCHAR *dir(int);
	int dir_default(void);
  
	// platform
	TCHAR *platform_get(int n) {
		return reinterpret_cast <TCHAR *>
		    (_platform(n, _PLATFORM_OP_GET));
	}
	int platform_default(void) {
		return reinterpret_cast <int>
		    (_platform(0, _PLATFORM_OP_DEFAULT));
	}
	void platform_set(int n) { _platform(n, _PLATFORM_OP_SET); }
};

/* Global access macro */
#define HPC_MENU	(HpcMenuInterface::Instance())
#define HPC_PREFERENCE	(HPC_MENU._pref)

#endif // _HPCBOOT_MENU_H_
