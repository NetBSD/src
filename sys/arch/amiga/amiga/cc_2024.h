/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: cc_2024.h,v 1.2 1994/01/29 06:58:36 chopps Exp $
 */

#if ! defined (_CC_2024_H)
#define _CC_2024_H

#include "cc.h"

#define DIGITAL_RED 0x0800
#define DIGITAL_GREEN 0x0080
#define DIGITAL_BLUE 0x0008
#define DIGITAL_INTENSE 0x0001

#define A2024_L0_BLACK  (0)
#define A2024_L0_DGREY  (DIGITAL_BLUE)
#define A2024_L0_LGREY  (DIGITAL_RED)
#define A2024_L0_WHITE  (DIGITAL_RED|DIGITAL_BLUE)

#define A2024_L1_BLACK  (0)
#define A2024_L1_DGREY  (DIGITAL_INTENSE)
#define A2024_L1_LGREY  (DIGITAL_GREEN)
#define A2024_L1_WHITE  (DIGITAL_GREEN|DIGITAL_INTENSE)

extern u_word a2024_color_value_line0[4];
extern u_word a2024_color_value_line1[4];

#define A2024_L0_INDEX(color_reg) (((0x4&color_reg)>>1)|(0x1&color_reg))
#define A2024_L1_INDEX(color_reg) (((0x8&color_reg)>>2)|((0x2&color_reg)>>1))
#define A2024_CM_TO_CR(cm,rn) \
	(a2024_color_value_line0[0x3 & (cm)->entry[A2024_L0_INDEX(rn)]] |\
	 a2024_color_value_line1[0x3 & (cm)->entry[A2024_L1_INDEX(rn)]])


#endif /* _CC_2024_H */
