/* -*-C++-*-	$NetBSD: sh_console.cpp,v 1.7.2.2 2002/03/16 15:57:52 jdolecek Exp $	*/

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
#include <sh3/sh_console.h>
#include <sh3/dev/sh.h>
#include <sh3/dev/hd64461.h>
#include <sh3/dev/hd64465.h>

// console switch
#include "../../../../hpcsh/include/console.h"

const struct SHConsole::console_info
SHConsole::_console_info[] = {
	{ PLATID_CPU_SH_3        , PLATID_MACH_HP                          , SCIFPrint       , BI_CNUSE_SCIF       , BI_CNUSE_HD64461VIDEO },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HITACHI                     , HD64461COMPrint , BI_CNUSE_HD64461COM , BI_CNUSE_HD64461VIDEO },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_CASIO_CASSIOPEIAA_A55V      , 0               , BI_CNUSE_BUILTIN    , BI_CNUSE_BUILTIN },
	{ PLATID_CPU_SH_4_7750   , PLATID_MACH_HITACHI_PERSONA_HPW650PA    , HD64465COMPrint , BI_CNUSE_HD64465COM , BI_CNUSE_BUILTIN },
	{ 0, 0, 0 } // terminator.
};

const struct SHConsole::console_info *
SHConsole::selectBootConsole(Console &cons, enum consoleSelect select)
{
	const struct console_info *tab = _console_info;
	platid_mask_t target, entry;

	target.dw.dw0 = HPC_PREFERENCE.platid_hi;
	target.dw.dw1 = HPC_PREFERENCE.platid_lo;

	// search apriori setting if any.
	for (; tab->cpu; tab++) {
		entry.dw.dw0 = tab->cpu;
		entry.dw.dw1 = tab->machine;
		if (platid_match(&target, &entry)) {
			switch (select) {
			case SERIAL:
				cons.setBootConsole(tab->serial_console);
				return tab;
			case VIDEO:
				cons.setBootConsole(tab->video_console);
				return tab;
			}
		}
	}

	return NULL;
}

SHConsole::SHConsole()
{

	_print = 0;
}

SHConsole::~SHConsole()
{
	// NO-OP
}

BOOL
SHConsole::init()
{
	
	if (!super::init())
		return FALSE;

	const struct console_info *tab = selectBootConsole(*this, SERIAL);
	if (tab != 0) {
		SetKMode(1);	// Native method access P4.
		_print = tab->print;
	} 
	
	// override default instance.
	Console::_instance = this;

	return TRUE;
}

void
SHConsole::print(const TCHAR *fmt, ...)
{

	SETUP_WIDECHAR_BUFFER();

	if (!setupMultibyteBuffer())
		return;

	if (_print == 0)
		super::genericPrint(_bufm);
	else
		_print(_bufm);
}

void
SHConsole::SCIPrint(const char *buf)
{

	SH3_SCI_PRINT(buf);
}

void
SHConsole::SCIFPrint(const char *buf)
{

	SH3_SCIF_PRINT(buf);
}

void
SHConsole::HD64461COMPrint(const char *buf)
{

	HD64461COM_PRINT(buf);
}

void
SHConsole::HD64465COMPrint(const char *buf)
{

	HD64465COM_PRINT(buf);
}
