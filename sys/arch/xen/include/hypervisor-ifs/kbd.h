/*	$NetBSD: kbd.h,v 1.1.4.2 2004/08/03 10:43:19 skrll Exp $	*/

/*
 *
 * Copyright (c) 2003, 2004 Keir Fraser (on behalf of the Xen team)
 * Copyright (c) 2003 James Scott, Intel Research Cambridge
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

/******************************************************************************
 * kbd.h
 *
 * PS/2 interface definitions
 * Copyright (c) 2003 James Scott, Intel Research Cambridge
 */

#ifndef __HYPERVISOR_KBD_H__
#define __HYPERVISOR_KBD_H__

			 
#define KBD_OP_WRITEOUTPUT   0
#define KBD_OP_WRITECOMMAND  1
#define KBD_OP_READ          2

#define KBD_CODE_SCANCODE(_r) ((unsigned char)((_r) & 0xff))
#define KBD_CODE_STATUS(_r) ((unsigned char)(((_r) >> 8) & 0xff))
#define KBD_CODE(_c, _s) ((int)(((_c) & 0xff)  | (((_s) & 0xff) << 8)))

#endif
