/*	$NetBSD: dom_mem_ops.h,v 1.1.4.2 2004/08/03 10:43:19 skrll Exp $	*/

/*
 *
 * Copyright (c) 2003, 2004 Keir Fraser (on behalf of the Xen team)
 * Copyright (c) 2003, B Dragovic & K A Fraser.
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
 * dom_mem_ops.h
 *
 * Guest OS operations dealing with physical memory reservations.
 *
 * Copyright (c) 2003, B Dragovic & K A Fraser.
 */

#define MEMOP_RESERVATION_INCREASE 0
#define MEMOP_RESERVATION_DECREASE 1

typedef struct reservation_increase {
    unsigned long   size;
    unsigned long   * pages;
} reservation_increase_t;

typedef struct reservation_decrease {
    unsigned long   size;
    unsigned long   * pages;
} reservation_decrease_t;

typedef struct dom_mem_op
{
    unsigned int op;
    union
    {
        reservation_increase_t increase;
        reservation_decrease_t decrease;
    } u;
} dom_mem_op_t;
