/* -*-C++-*-	$NetBSD: platform.cpp,v 1.4 2005/07/12 23:21:54 uwe Exp $	*/

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
#include <machine/platid.h>
#include <machine/platid_mask.h>

void *
HpcMenuInterface::_platform(int n, enum _platform_op op)
{
	int i, cnt;
	struct platid_name *name;

	for (i = cnt = 0, name = platid_name_table;
	    i < platid_name_table_size; i++, name++) {
		platid_t *mask = name->mask;
		if (mask->dw.dw0 == PLATID_WILD || mask->dw.dw1 == PLATID_WILD)
			continue;
		switch(op) {
		case _PLATFORM_OP_GET:
			if (n == cnt)
				// XXX: constification fallout
				return reinterpret_cast <void *>
					(const_cast <tchar *>(name->name));
			break;
		case _PLATFORM_OP_SET:
			if (n == cnt) {
				_pref.platid_hi = mask->dw.dw0;
				_pref.platid_lo = mask->dw.dw1;
				return static_cast <void *>(0);
			}
			break;
		case _PLATFORM_OP_DEFAULT:
			if ((mask->dw.dw0 == _pref.platid_hi) &&
			    (mask->dw.dw1 == _pref.platid_lo))
				return reinterpret_cast <void *>(cnt);
			break;
		}
		cnt++;
	}

	return 0;
}
