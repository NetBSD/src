/*	$NetBSD: hpcboot.cpp,v 1.4.8.2 2002/02/28 04:09:44 nathanw Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <menu/window.h>
#include <menu/rootwindow.h>

#include <console.h>
#include <arch.h>
#include <memory.h>
#include <file.h>
#include <load.h>

#include <boot.h>

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prev_instance,
    LPTSTR cmd_line, int window_show)
{
	HpcMenuInterface::Instance();	// Menu System
	HpcBootApp *app = 0;		// Application body.
	int ret = 0;

	InitCommonControls();

	app = new HpcBootApp(instance);
	app->_cons = Console::Instance();
	app->_root = new RootWindow(*app);

	if (!app->registerClass(reinterpret_cast <WNDPROC>(Window::_wnd_proc)))
		goto failed;

	if (!app->_root->create(0))
		goto failed;

	Boot::Instance();	// Boot loader

	ret = app->run();	// Main loop.
	// NOTREACHED

 failed:

	Boot::Destroy();
	if (app->_root)
		delete app->_root;
	delete app;
	Console::Destroy();
	HpcMenuInterface::Destroy();

	return ret;
}	 

//
// boot sequence. 
//
void
hpcboot(void *arg)
{
	size_t sz = 0;
	paddr_t p = 0;
	TCHAR *error_message = 0;

	HpcMenuInterface &menu = HPC_MENU;
	Boot &f = Boot::Instance();

	menu.progress();
	if (!f.setup()) {
		error_message = TEXT("architecture not supported.\n");
		goto failed_exit;
	}

	menu.progress();
	if (!f.create()) {
		error_message = TEXT("architexture ops. not found.\n");
		goto failed_exit;
	}

	menu.progress();
	if (!f._arch->init()) {
		error_message = TEXT("architecture initialize failed.\n");
		goto failed_exit;
	}

	f._arch->systemInfo();

	menu.progress();
	// kernel / file system image directory.
	if (!f._file->setRoot(f.args.fileRoot)) {
		error_message = TEXT("couldn't set root directory.\n");
		goto failed_exit;
	}

	// open file system image.
	if (f.args.loadmfs)
	{
		if (!f._file->open(f.args.mfsName)) {
			error_message =
			    TEXT("couldn't open file system image.\n");
			goto failed_exit;
		}
		sz = f._file->size();
		sz = f._mem->roundPage(sz);
		f._file->close();
	}

	menu.progress();
	if (!f._file->open(f.args.fileName)) {
		error_message = TEXT("couldn't open kernel image.\n");
		goto failed;
	}
  
	menu.progress();
	// put kernel to loader.
	if (!f.attachLoader())
		goto failed;
		
	if (!f._loader->setFile(f._file)) {
		error_message = TEXT("couldn't initialize loader.\n");
		goto failed;
	}

	menu.progress();
	sz += f._mem->roundPage(f._loader->memorySize());

	// allocate required memory.
	if (!f._arch->allocateMemory(sz)) {
		error_message = TEXT("couldn't allocate memory.\n");
		goto failed;
	}

	menu.progress();
	// load kernel to memory.
	if (!f._arch->setupLoader()) {
		error_message = TEXT("couldn't set up loader.\n");
		goto failed;
	}

	menu.progress();
	if (!f._loader->load()) {
		error_message = TEXT("couldn't load kernel image to memory.\n");
		goto failed;
	}
	menu.progress();
	f._file->close();

	// load file system image to memory
	if (f.args.loadmfs) {
		f._file->open(f.args.mfsName);
		if (!f._loader->loadExtData()) {
			error_message =
			    TEXT("couldn't load filesystem image to memory.\n");
			goto failed;
		}
		f._file->close();
	}
	f._loader->loadEnd();

	// setup arguments for kernel.
	p = f._arch->setupBootInfo(*f._loader);

	menu.progress();

	f._loader->tagDump(3); // dump page chain.(print first 3 links)

	// jump to kernel entry.
	if (HPC_PREFERENCE.pause_before_boot) {
		if (MessageBox(menu._root->_window, TEXT("Push OK to boot."),
		    TEXT("Last chance..."), MB_YESNO) != IDYES)
			goto failed;
	}

	f._arch->jump(p, f._loader->tagStart());
	// NOTREACHED

 failed:
	if (error_message == 0)
		error_message = TEXT("can't jump to kernel.\n");
	f._file->close();
 failed_exit:
	MessageBox(menu._root->_window, error_message,
	    TEXT("BOOT FAILED"), 0);
}

//
// HPCBOOT main loop
//
int
HpcBootApp::run(void)
{
	MSG msg;

	while (GetMessage(&msg, 0, 0, 0)) {
		// cancel auto-boot.
		if (HPC_PREFERENCE.auto_boot > 0 && _root &&
		    (msg.message == WM_KEYDOWN ||
			msg.message == WM_LBUTTONDOWN)) {
			_root->disableTimer();
		}
		if (!_root->isDialogMessage(msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

BOOL
HpcBootApp::registerClass(WNDPROC proc)
{
	TCHAR *wc_name;
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc_name		= reinterpret_cast <TCHAR *>
	    (LoadString(_instance, IDS_HPCMENU, 0, 0));
	wc.lpfnWndProc	= proc;
	wc.hInstance	= _instance;
	wc.hIcon	= LoadIcon(_instance, MAKEINTRESOURCE(IDI_ICON));
	wc.cbWndExtra	= 4;		// pointer of `this`
	wc.hbrBackground= static_cast <HBRUSH>(GetStockObject(LTGRAY_BRUSH));
	wc.lpszClassName= wc_name;

	return RegisterClass(&wc);
}


//
// Debug support.
//
void
_bitdisp(u_int32_t a, int s, int e, int m, int c)
{
	u_int32_t j, j1;
	int i, n;

	DPRINTF_SETUP();

	n = 31;	// 32bit only.
	j1 = 1 << n;
	e = e ? e : n;
	for (j = j1, i = n; j > 0; j >>=1, i--) {
		if (i > e || i < s) {
			DPRINTF((TEXT("%c"), a & j ? '+' : '-'));
		} else {
			DPRINTF((TEXT("%c"), a & j ? '|' : '.'));
		}
	}
	if (m) {
		DPRINTF((TEXT("[%s]"),(char*)m));
	}

	DPRINTF((TEXT(" [0x%08x]"), a));

	if (c) {
		for (j = j1, i = n; j > 0; j >>=1, i--) {
			if (!(i > e || i < s) &&(a & j)) {
				DPRINTF((TEXT(" %d"), i));
			}
		}
	}

	DPRINTF((TEXT(" %d\n"), a));
}

void
_dbg_bit_print(u_int32_t reg, u_int32_t mask, const char *name)
{
	static const char onoff[3] = "_x";

	DPRINTF_SETUP();

	DPRINTF((TEXT("%S[%c] "), name, onoff[reg & mask ? 1 : 0]));
}
