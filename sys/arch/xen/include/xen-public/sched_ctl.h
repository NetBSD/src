/* $NetBSD: sched_ctl.h,v 1.2 2005/03/09 22:39:20 bouyer Exp $ */

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

/**
 * Generic scheduler control interface.
 *
 */

#ifndef __XEN_PUBLIC_SCHED_CTL_H__
#define __XEN_PUBLIC_SCHED_CTL_H__

/* Scheduler types */
#define SCHED_BVT      0
#define SCHED_ATROPOS  2
#define SCHED_RROBIN   3

/* these describe the intended direction used for a scheduler control or domain
 * command */
#define SCHED_INFO_PUT 0
#define SCHED_INFO_GET 1

/*
 * Generic scheduler control command - used to adjust system-wide scheduler
 * parameters
 */
struct sched_ctl_cmd
{
    u32 sched_id;                     /*  0 */
    u32 direction;                    /*  4 */
    union {                           /*  8 */
        struct bvt_ctl
        {
            /* IN variables. */
            u32 ctx_allow;            /*  8: context switch allowance */
        } PACKED bvt;

        struct rrobin_ctl
        {
            /* IN variables */
            u64 slice;                /*  8: round robin time slice */
        } PACKED rrobin;
    } PACKED u;
} PACKED; /* 16 bytes */

struct sched_adjdom_cmd
{
    u32     sched_id;                 /*  0 */
    u32     direction;                /*  4 */
    domid_t domain;                   /*  8 */
    u16     __pad0;
    u32     __pad1;
    union {                           /* 16 */
        struct bvt_adjdom
        {
            u32 mcu_adv;            /* 16: mcu advance: inverse of weight */
            u32 warpback;           /* 20: warp? */
            s32 warpvalue;          /* 24: warp value */
            long long warpl;        /* 32: warp limit */
            long long warpu;        /* 40: unwarp time requirement */
        } PACKED bvt;

        struct atropos_adjdom
        {
            u64 nat_period; /* 16 */
            u64 nat_slice;  /* 24 */
            u64 latency;    /* 32 */
            u32 xtratime;   /* 36 */
        } PACKED atropos;
    } PACKED u;
} PACKED; /* 40 bytes */

#endif /* __XEN_PUBLIC_SCHED_CTL_H__ */
