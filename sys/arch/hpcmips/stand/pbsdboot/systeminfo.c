/* $NetBSD: systeminfo.c,v 1.3 2000/01/16 03:07:33 takemura Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>
#include <machine/platid_mask.h>
#if 0
#define PROCESSOR_LEVEL_R4000 4
#define PROCESSOR_LEVEL_R3000 3
#define PROCESSOR_REVMAJOR_VR41XX 0x0c
#define PROCESSOR_REVMAJOR_TX39XX 0x22
#endif

struct system_info system_info;

static void dump_archinfo(SYSTEM_INFO*);

int
set_system_info(platid_t *platid)
{
	SYSTEM_INFO info;
	/*
	 * Set machine dependent information.
	 */
	GetSystemInfo(&info);
	if (platid_match(platid, &platid_mask_CPU_MIPS)) {
		if (platid_match(platid, &platid_mask_CPU_MIPS_VR_41XX)) {
			vr41xx_init(&info);
		} else
		if (platid_match(platid, &platid_mask_CPU_MIPS_TX)) {
			tx39xx_init(&info);
		} else {
			dump_archinfo(&info);
			return 0;
		}
	} else {
		dump_archinfo(&info);
		return 0;
	}

#if 0
	switch (info.wProcessorArchitecture) {
	default:
		dump_archinfo(&info);
		return 0;
		break;
	case PROCESSOR_ARCHITECTURE_MIPS:
		switch (info.wProcessorLevel) {
		default:
			dump_archinfo(&info);
			return 0;
			break;
		case PROCESSOR_LEVEL_R4000:
			switch (info.wProcessorRevision >> 8) {
			default:
				dump_archinfo(&info);
				return 0;
				break;
			case PROCESSOR_REVMAJOR_VR41XX:
				vr41xx_init(&info);
				break;
			}
			break;
		case PROCESSOR_LEVEL_R3000:
			switch (info.wProcessorRevision >> 8) {
			default:
				dump_archinfo(&info);
				return 0;
				break;
			case PROCESSOR_REVMAJOR_TX39XX:
				tx39xx_init(&info);
				break;
			}
			break;
		}
		break;
	case PROCESSOR_ARCHITECTURE_SHx:
		dump_archinfo(&info);
		return 0;
		break;
	case PROCESSOR_ARCHITECTURE_ARM:
		dump_archinfo(&info);
		return 0;
		break;
	}
#endif

	if (system_info.si_asmcodelen > (signed)system_info.si_pagesize) {
		msg_printf(MSG_ERROR, whoami, 
			   TEXT("asmcodelen=%d > pagesize=%d\n"),
			   system_info.si_asmcodelen,
			   system_info.si_pagesize);
		return 0;
	}

	return 1;
}

static void
dump_archinfo(SYSTEM_INFO *info)
{
	msg_printf(MSG_ERROR, whoami, TEXT("Unsupported CPU\n"));
#if 0
	msg_printf(MSG_ERROR, whoami, 
		   TEXT("Unknown machine ARCHITECTURE %#x, LEVEL %#x REVISION %#x.\n LCD(%dx%d)\n"),
		   info->wProcessorArchitecture, info->wProcessorLevel, 
		   info->wProcessorRevision,
		   GetSystemMetrics(SM_CXSCREEN),
		   GetSystemMetrics(SM_CYSCREEN));
#endif
}
