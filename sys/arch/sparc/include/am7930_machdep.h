/*	$NetBSD: am7930_machdep.h,v 1.1 1999/03/14 22:29:00 jonathan Exp $	*/

/*
 * Machine-dependent register accessors for am7930.
 * On Sparc, these are direct bus_space operations.
 */

#define AM7930_REGOFF(reg) (offsetof(struct am7930, reg))
 
#define AM7930_WRITE_REG(bt, bh, reg, val) \
    bus_space_write_1((bt), (bh), AM7930_REGOFF(reg), (val))

#define AM7930_READ_REG(bt, bh, reg) \
    bus_space_read_1((bt), (bh), AM7930_REGOFF(reg), (val))
