/*
 * Copyright (c) 1993 Paul Mackerras.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *	$Id: kbd.h,v 1.1 1994/02/22 23:50:33 paulus Exp $
 */
/*
 * Definitions relating to the XT230 keyboard
 * and communication of keyboard modes with the GSP.
 */

/* ASCII control chars */
#define ESC		0x1B
#define CSI		0x9B
#define SS3		0x8F

/* Key codes from keyboard */
#define KEY_SCRL	0x90
#define KEY_PRINT	0x93
#define KEY_S_PRINT	0x9a
#define KEY_C_PRINT	0x92
#define KEY_SETUP	0x91
#define KEY_SESSION	0x95
#define KEY_S_SESSION	0x9b
#define KEY_C_SESSION	0x94
#define KEY_BREAK	0x98
#define KEY_S_BREAK	0x97
#define KEY_C_BREAK	0x96

/* Command codes to keyboard */
#define KC_CLICK	0x11		/* makes a keyclick */
#define KC_BELL		0x13		/* makes a beep */

/* Keyboard modes, from GSP */
#define KBD_CLICK	1		/* click on key depressions */
#define KBD_7_BIT	2		/* translate to 7-bit controls */
#define KBD_APPL_CURS	4		/* application cursor keys */
#define KBD_APPL_KPAD	8		/* application keypad */
#define KBD_AUTO_RPT	0x10		/* enable auto-repeat */

/* Command codes to GSP */
#define GSP_CMD_REFRESH	1

