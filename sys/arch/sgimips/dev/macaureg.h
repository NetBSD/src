/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * MACE audio CODEC register definitions
 *
 * With an Analog Device AD1843 CODEC (?)
 */

#define MACAU_CONTROL_STATUS		0x00
#define MACAU_CODEC_STATUS_CNTL		0x08
#define MACAU_CODEC_STATUS_INPUT_MASK	0x10
#define MACAU_CODEC_STATUS_INPUT	0x18
#define MACAU_CH1_IN_RING_CONTROL	0x20
#define MACAU_CH1_IN_READ_POINTER	0x28
#define MACAU_CH1_IN_WRITE_POINTER	0x30
#define MACAU_CH1_IN_RING_DEPTH		0x38
#define MACAU_CH2_OUT_RING_CONTROL	0x40
#define MACAU_CH2_OUT_READ_POINTER	0x48
#define MACAU_CH2_OUT_WRITE_POINTER	0x50
#define MACAU_CH2_OUT_RING_DEPTH	0x58
#define MACAU_CH3_OUT_RING_CONTROL	0x60
#define MACAU_CH3_OUT_READ_POINTER	0x68
#define MACAU_CH3_OUT_WRITE_POINTER	0x70
#define MACAU_CH3_OUT_RING_DEPTH	0x78
