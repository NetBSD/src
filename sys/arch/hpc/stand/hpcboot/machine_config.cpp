/* -*-C++-*-	$NetBSD: machine_config.cpp,v 1.4.2.2 2002/02/11 20:07:59 jdolecek Exp $	*/

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

//
// Platform dependent configuration.
//

#include <hpcmenu.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <framebuffer.h>

//
// Frame buffer information.
//
struct FrameBufferInfo::framebuffer_info
FrameBufferInfo::_table[] =
{
	//         CPU                        MACHINE                             BPP      WIDTH   HEIGHT LINEBYTES  FRAME BUFFER ADDR
#ifdef MIPS
	// VR41 (kseg1 address)
	{ PLATID_CPU_MIPS_VR_4102, PLATID_MACH_EVEREX_FREESTYLE_AXX        ,        2,      240,      320,       80, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_11                 ,        2,      480,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_12                 ,        2,      480,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_13                 ,        2,      480,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCR_MPRO700             ,        2,      640,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_CASIO_CASSIOPEIAE_E55       ,        2,      240,      320,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_COMPAQ_AERO_1530            ,        2,      240,      320,        0, 0xa0000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_COMPAQ_PRESARIO_213         ,        8,      240,      320,        0, 0xa0000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_300                 ,        2,      640,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_500                 ,        8,      640,      240,     1024, 0xb3000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_500A                ,        8,      640,      240,     1024, 0xb3000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_FORDOCOMO           ,        2,      640,      240,      256, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4111, PLATID_MACH_SHARP_TRIPAD_PV6000         ,        8,      640,      480,      640, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_CASSIOPEIAE_E100      ,       16,      240,      320,      512, 0xaa200000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_CASSIOPEIAE_E500      ,       16,      240,      320,      512, 0xaa200000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_POCKETPOSTPET_POCKETPOSTPET,  16,      320,      240,     1024, 0xaa200000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT300      ,        8,      640,      480,      640, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT300      ,       16,      640,      480,     1280, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT310      ,        8,      640,      480,      640, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_IBM_WORKPAD_26011AU         ,       16,      640,      480,     1280, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_320                 ,        2,      640,      240,      160, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_330                 ,        2,      640,      240,      160, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_430                 ,       16,      640,      240,     1280, 0xaa180100 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_510                 ,        8,      640,      240,     1024, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_510                 ,       16,      640,      240,     1600, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_520                 ,       16,      640,      240,     1600, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_520A                ,       16,      640,      240,     1600, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530                 ,        8,      640,      240,      640, 0xaa1d4c00 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530                 ,       16,      640,      240,     1280, 0xaa180100 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530A                ,       16,      640,      240,     1280, 0xaa180100 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_700                 ,       16,      800,      600,     1600, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_700A                ,       16,      800,      600,     1600, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_730                 ,       16,      800,      600,     1600, 0xaa0ea600 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_730A                ,       16,      800,      600,     1600, 0xaa0ea600 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_SIGMARION           ,       16,      640,      240,     1280, 0xaa000000 },
	{ PLATID_CPU_MIPS_VR_4131, PLATID_MACH_NEC_MCR_SIGMARION2          ,       16,      640,      240,     1280, 0xb0800000 },
	{ PLATID_CPU_MIPS_VR_4121, PLATID_MACH_SHARP_TRIPAD_PV6000         ,       16,      640,      480,     1280, 0xaa200000 },
        { PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_PENCENTRA_130       ,        8,      640,      480,      640, 0xb0201e00 },
	{ PLATID_CPU_MIPS_VR_4122, PLATID_MACH_VICTOR_INTERLINK_MPC303     ,       16,      800,      600,        0, 0x00000000 },
	// TX39 (can't determine frame buffer address)
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_2010               ,        8,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_2015               ,        8,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_810                ,        2,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_PHILIPS_NINO_312            ,        2,      240,      320,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_SHARP_MOBILON_HC1200        ,        4,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3912, PLATID_MACH_SHARP_MOBILON_HC4100        ,        4,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3922, PLATID_MACH_SHARP_TELIOS_HCAJ1          ,       16,      800,      600,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3922, PLATID_MACH_SHARP_TELIOS_HCVJ1C_JP      ,       16,      800,      480,        0, 0x00000000 },
	{ PLATID_CPU_MIPS_TX_3922, PLATID_MACH_VICTOR_INTERLINK_MPC101     ,       16,      640,      480,        0, 0x00000000 },
#endif // MIPS
#ifdef SHx
	// SH7709 (P2 address)
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HP_LX_620                   ,        8,      640,      240,      640, 0xb2000000 },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HP_LX_620JP                 ,        8,      640,      240,      640, 0xb2000000 },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HITACHI_PERSONA_HPW50PAD    ,        8,      640,      240,      640, 0xb2000000 },
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_HITACHI_PERSONA_HPW230JC    ,        8,      640,      240,      640, 0xb2000000 },
	// SH7709A (P2 address)
	{ PLATID_CPU_SH_3_7709A  , PLATID_MACH_HP_JORNADA                  ,       16,      640,      240,     1280, 0xb2000000 },
	// SH7750 (P2 address)
	{ PLATID_CPU_SH_4_7750   , PLATID_MACH_HITACHI_PERSONA_HPW650PA    ,       16,      640,      480, 0, 0 },
#endif // SHx
#ifdef ARM
	// SA-1100 (can't determine frame buffer address)
	{ PLATID_CPU_ARM_STRONGARM_SA1100 , PLATID_MACH_HP_JORNADA_820     ,        8,      640,      480,        0, 0x00000000 },
	{ PLATID_CPU_ARM_STRONGARM_SA1100 , PLATID_MACH_HP_JORNADA_820JP   ,        8,      640,      480,        0, 0x00000000 },
	// SA-1110
	{ PLATID_CPU_ARM_STRONGARM_SA1110 , PLATID_MACH_HP_JORNADA_720     ,       16,      640,      240,        0, 0x00000000 },
	{ PLATID_CPU_ARM_STRONGARM_SA1110 , PLATID_MACH_HP_JORNADA_720JP   ,       16,      640,      240,     1280, 0x48200000 },
	{ PLATID_CPU_ARM_STRONGARM_SA1110 , PLATID_MACH_COMPAQ_IPAQ_H3600  ,       16,      240,      320,        0, 0x00000000 },
#endif // ARM
	{ 0, 0, 0, 0, 0, 0, 0 } // TERMINATOR
};

//
// Unsupported machine list.
//
struct HpcMenuInterface::support_status
HpcMenuInterface::_unsupported[] =
{
#ifdef ARM
#endif // ARM
#ifdef MIPS
#endif // MIPS
#ifdef SHx
	{ PLATID_CPU_SH_3_7709   , PLATID_MACH_CASIO_CASSIOPEIAA_A55V      , L"unknown Companion Chip FM-7403" },
#endif // SHx
	{ 0, 0, 0 } // TERMINATOR
};
