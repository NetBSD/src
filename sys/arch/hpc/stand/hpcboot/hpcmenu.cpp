/* -*-C++-*-	$NetBSD: hpcmenu.cpp,v 1.8.2.1 2002/06/23 17:36:43 jdolecek Exp $	*/

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
#include <menu/menu.h>
#include <machine/bootinfo.h>
#include <framebuffer.h>
#include <console.h>
#include <string.h>

HpcMenuInterface *HpcMenuInterface::_instance = 0;

HpcMenuInterface &
HpcMenuInterface::Instance()
{
	if (!_instance)
		_instance = new HpcMenuInterface();
	return *_instance;
}

void
HpcMenuInterface::Destroy()
{
	if (_instance)
		delete _instance;
}

HpcMenuInterface::HpcMenuInterface()
{
	if (!load())
		_set_default_pref();
	_pref._version	= HPCBOOT_VERSION;
	_pref._size	= sizeof(HpcMenuPreferences);
    
	_cons_parameter = 0;
	memset(_cons_hook, 0, sizeof(struct cons_hook_args) * 4);
	memset(&_boot_hook, 0, sizeof(struct boot_hook_args));
}

void
HpcMenuInterface::print(TCHAR *buf)
{
	if (_console)
		_console->print(buf);
}

void
HpcMenuInterface::get_options()
{
	_main->get();
	_option->get();
}

TCHAR *
HpcMenuInterface::dir(int i)
{
	int res = IDS_DIR_RES(i);
	if (!IDS_DIR_RES_VALID(res))
		return 0;
  
	if (_pref.dir_user && res == IDS_DIR_USER_DEFINED) {
		return _pref.dir_user_path;
	}

	TCHAR *s = reinterpret_cast <TCHAR *>
	    (LoadString(_root->_app._instance, res, 0, 0));
  
	return s;
}

int
HpcMenuInterface::dir_default()
{
	return _pref.dir_user ? IDS_DIR_SEQ(IDS_DIR_USER_DEFINED) : 0;
}

void
HpcMenuInterface::_set_default_pref()
{
	_pref._magic		= HPCBOOT_MAGIC;
	_pref.dir		= 0;
	_pref.dir_user		= FALSE;
	_pref.kernel_user	= FALSE;
	_pref.platid_hi		= 0;
	_pref.platid_lo		= 0;
	_pref.rootfs		= 0;
	wsprintf(_pref.rootfs_file, TEXT("miniroot.fs"));
	_pref.boot_serial	= FALSE;
	_pref.boot_verbose	= FALSE;
	_pref.boot_single_user	= FALSE;
	_pref.boot_ask_for_name	= FALSE;
	_pref.boot_debugger	= FALSE;
	wsprintf(_pref.boot_extra, TEXT(""));
	_pref.auto_boot		= 0;
	_pref.reverse_video	= FALSE;
	_pref.pause_before_boot	= TRUE;
	_pref.safety_message	= TRUE;
#ifdef MIPS
	_pref.serial_speed	= 9600; // historical reason.
#else
	_pref.serial_speed	= 19200;
#endif
}

//
// load and save current menu status.
//
BOOL
HpcMenuInterface::load()
{
	TCHAR path[MAX_PATH];

	if (!_find_pref_dir(path))
		return FALSE;

	TCHAR filename[MAX_PATH];
	wsprintf(filename, TEXT("\\%s\\hpcboot.cnf"), path);
	HANDLE file = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL, 0);
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;
  
	DWORD cnt;
	// read header.
	if (!ReadFile(file, &_pref, 12, &cnt, 0))
		goto bad;
	if (_pref._magic != HPCBOOT_MAGIC)
		goto bad;
	// read all.
	SetFilePointer(file, 0, 0, FILE_BEGIN);
	if (!ReadFile(file, &_pref, _pref._size, &cnt, 0))
		goto bad;
	CloseHandle(file);

	return TRUE;
 bad:
	CloseHandle(file);
	return FALSE;
}

BOOL
HpcMenuInterface::save()
{
	TCHAR path[MAX_PATH];
  
	if (_find_pref_dir(path)) {
		TCHAR filename[MAX_PATH];
		wsprintf(filename, TEXT("\\%s\\hpcboot.cnf"), path);
		HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, 0,
		    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		DWORD cnt;
		WriteFile(file, &_pref, _pref._size, &cnt, 0);
		CloseHandle(file);
		return cnt == _pref._size;
	}

	return FALSE;
}

// arguments for kernel.
int
HpcMenuInterface::setup_kernel_args(vaddr_t v, paddr_t p, size_t sz)
{
	int argc = 0;
	kaddr_t *argv = reinterpret_cast <kaddr_t *>(v);
	char *loc = reinterpret_cast <char *>
	    (v + sizeof(char **) * MAX_KERNEL_ARGS);
	paddr_t locp = p + sizeof(char **) * MAX_KERNEL_ARGS;
	size_t len;
	TCHAR *w;
	char *ptr;

#define SETOPT(c)							\
__BEGIN_MACRO								\
	argv[argc++] = ptokv(locp);					\
	*loc++ =(c);							\
	*loc++ = '\0';							\
	locp += 2;							\
__END_MACRO
	// 1st arg is kernel name.
//	DPRINTF_SETUP();  //if you want to use debug print, enable this line.

	w = _pref.kernel_user_file;
	argv[argc++] = ptokv(locp);
	len = WideCharToMultiByte(CP_ACP, 0, w, wcslen(w), 0, 0, 0, 0);
	WideCharToMultiByte(CP_ACP, 0, w, len, loc, len, 0, 0); 
	loc[len] = '\0';
	loc += len + 1;
	locp += len + 1;

	if (_pref.boot_serial)		// serial console
		SETOPT('h');
	if (_pref.boot_verbose)		// boot verbosely
		SETOPT('v');
	if (_pref.boot_single_user)	// boot to single user
		SETOPT('s');
	if (_pref.boot_ask_for_name)	// ask for file name to boot from
		SETOPT('a');
	if (_pref.boot_debugger)	// break into the kernel debugger
		SETOPT('d');

	// boot from
	switch(_pref.rootfs) {
	case 0:	// wd0
		break;
	case 1:	// sd0
		argv[argc++] = ptokv(locp);
		strncpy(loc, "b=sd0", 6);
		loc += 6;
		locp += 6;
		break;
	case 2:	// memory disk
		w = _pref.rootfs_file;
		argv[argc++] = ptokv(locp);
		strncpy(loc, "m=", 2);
		loc += 2;
		len = WideCharToMultiByte(CP_ACP, 0, w, wcslen(w), 0, 0, 0, 0);
		WideCharToMultiByte(CP_ACP, 0, w, len, loc, len, 0, 0); 
		loc[len] = '\0';
		loc += len + 1;
		locp += 2 + len + 1;
		break;
	case 3:	// nfs
		argv[argc++] = ptokv(locp);
		strncpy(loc, "b=nfs", 6);
		loc += 6;
		locp += 6;
		break;
	}

	// Extra kernel options. (Option tab window)
	w = _pref.boot_extra;
	len = WideCharToMultiByte(CP_ACP, 0, w, wcslen(w), 0, 0, 0, 0);

	if ((ptr = (char *)malloc(len)) == NULL) {
		MessageBox(_root->_window,
		    L"Couldn't allocate memory for extra kernel options.",
		    TEXT("WARNING"), 0);

		return argc;  
	}
	WideCharToMultiByte(CP_ACP, 0, w, len, ptr, len, 0, 0); 
	ptr[len]='\0';

	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	while (*ptr != '\0') {
		len = strcspn(ptr, " \t");
		if (len == 0)
			len = strlen(ptr);

		if (argc == MAX_KERNEL_ARGS || locp + len + 1 > p + sz) {
			MessageBox(_root->_window,
			    L"Too many extra kernel options.",
			    TEXT("WARNING"), 0);
			break;
		}
		argv[argc++] = ptokv(locp);
		strncpy(loc, ptr, len);
		loc[len] = '\0';
		loc += len + 1;
		locp += len + 1;
	
		ptr += len;
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
	}

	return argc;
}

// kernel bootinfo.
void
HpcMenuInterface::setup_bootinfo(struct bootinfo &bi)
{
	FrameBufferInfo fb(_pref.platid_hi, _pref.platid_lo);
	TIME_ZONE_INFORMATION tz;
	GetTimeZoneInformation(&tz);

	memset(&bi, 0, sizeof(struct bootinfo));
	bi.length		= sizeof(struct bootinfo);
	bi.reserved		= 0;
	bi.magic		= BOOTINFO_MAGIC;
	bi.fb_addr		= fb.addr();
	bi.fb_type		= fb.type();
	bi.fb_line_bytes	= fb.linebytes();
	bi.fb_width		= fb.width();
	bi.fb_height		= fb.height();
	bi.platid_cpu		= _pref.platid_hi;
	bi.platid_machine	= _pref.platid_lo;
	bi.timezone		= tz.Bias;
}

// Progress bar
void
HpcMenuInterface::progress()
{
	SendMessage(_root->_progress_bar->_window, PBM_STEPIT, 0, 0);
}

// Boot kernel.
void
HpcMenuInterface::boot()
{
	struct support_status *tab = _unsupported;
	u_int32_t cpu = _pref.platid_hi;
	u_int32_t machine = _pref.platid_lo;

	if (_pref.safety_message)
		for (; tab->cpu; tab++) {
			if (tab->cpu == cpu && tab->machine == machine) {
				MessageBox(_root->_window,
				    tab->cause ? tab->cause :
				    L"not supported yet.",
				    TEXT("BOOT FAILED"), 0);
				return;
			}
		}

	if (_boot_hook.func)
		_boot_hook.func(_boot_hook.arg);
}
