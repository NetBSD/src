/*	$NetBSD: sh_console.cpp,v 1.2 2001/03/13 16:31:31 uch Exp $	*/

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
#include <sh3/sh_console.h>

SHConsole *SHConsole::_instance = 0;

struct SHConsole::console_info
SHConsole::_console_info[] = {
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HP_LX_620                   , SCIFPrint },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HP_LX_620JP                 , SCIFPrint },
	{ PLATID_CPU_SH_3_7709A  , PLATID_MACH_HP_JORNADA_680              , SCIFPrint },
	{ PLATID_CPU_SH_3_7709A  , PLATID_MACH_HP_JORNADA_680JP            , SCIFPrint },
	{ PLATID_CPU_SH_3_7709A  , PLATID_MACH_HP_JORNADA_690              , SCIFPrint },
	{ PLATID_CPU_SH_3_7709A  , PLATID_MACH_HP_JORNADA_690JP            , SCIFPrint },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HITACHI_PERSONA_HPW230JC    , 0 }, // HD64461 Serial module
	{ 0, 0, 0 } // terminator.
};

SHConsole::SHConsole()
{
	_print = 0;
}

SHConsole::~SHConsole()
{
}

SHConsole *
SHConsole::Instance()
{
	if (!_instance)
		_instance = new SHConsole();

	return _instance;
}

BOOL
SHConsole::init()
{
	HpcMenuInterface &menu = HpcMenuInterface::Instance();
	struct console_info *tab = _console_info;
	u_int32_t cpu, machine;
	
	_kmode = SetKMode(1);
	
	cpu = menu._pref.platid_hi;
	machine = menu._pref.platid_lo;

	for (; tab->cpu; tab++) {
		if (tab->cpu == cpu && tab->machine == machine) {
			_print = tab->print;
			break;
		}
	}

	/* 
	 * always open COM1 to supply clock and power for the
	 * sake of kernel serial driver 
	 */
	return openCOM1();
}

void
SHConsole::print(const TCHAR *fmt, ...)
{
	if (_print == 0)
		return;

	va_list ap;
	va_start(ap, fmt);
	wvsprintf(_bufw, fmt, ap);
	va_end(ap);

	if (!setupBuffer())
		return;

	_print(_bufm);
}

void
SHConsole::SCIFPrint(const char *buf)
{
	SCIF_PRINT(buf);
}

