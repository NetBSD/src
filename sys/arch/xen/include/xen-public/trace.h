/* $NetBSD: trace.h,v 1.2 2005/03/09 22:39:20 bouyer Exp $ */

/*
 * Mark Williamson, (C) 2004 Intel Research Cambridge
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
 * trace.h
 */

#ifndef __XEN_PUBLIC_TRACE_H__
#define __XEN_PUBLIC_TRACE_H__

/* This structure represents a single trace buffer record. */
struct t_rec {
    u64 cycles;               /* 64 bit cycle counter timestamp */
    u32 event;                /* 32 bit event ID                */
    u32 d1, d2, d3, d4, d5;   /* event data items               */
};

/*
 * This structure contains the metadata for a single trace buffer.  The head
 * field, indexes into an array of struct t_rec's.
 */
struct t_buf {
    unsigned long data;      /* pointer to data area.  machine address
                              * for convenience in user space code           */

    unsigned long size;      /* size of the data area, in t_recs             */
    unsigned long head;      /* array index of the most recent record        */

    /* Xen-private elements follow... */
    struct t_rec *head_ptr; /* pointer to the head record                    */
    struct t_rec *vdata;    /* virtual address pointer to data               */
};

#endif /* __XEN_PUBLIC_TRACE_H__ */
