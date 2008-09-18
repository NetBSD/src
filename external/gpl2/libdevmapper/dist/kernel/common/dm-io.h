/*
 * Copyright (C) 2003 Sistina Software
 *
 * This file is released under the GPL.
 */

#ifndef _DM_IO_H
#define _DM_IO_H

#include "dm.h"

#include <linux/list.h>

/* Move these to bitops.h eventually */
/* Improved generic_fls algorithm (in 2.4 there is no generic_fls so far) */
/* (c) 2002, D.Phillips and Sistina Software */
/* Licensed under Version 2 of the GPL */

static unsigned generic_fls8(unsigned n)
{
	return n & 0xf0 ?
	    n & 0xc0 ? (n >> 7) + 7 : (n >> 5) + 5:
	    n & 0x0c ? (n >> 3) + 3 : n - ((n + 1) >> 2);
}

static inline unsigned generic_fls16(unsigned n)
{
	return	n & 0xff00? generic_fls8(n >> 8) + 8 : generic_fls8(n);
}

static inline unsigned generic_fls32(unsigned n)
{
	return	n & 0xffff0000 ? generic_fls16(n >> 16) + 16 : generic_fls16(n);
}

/* FIXME make this configurable */
#define DM_MAX_IO_REGIONS 8

struct io_region {
	kdev_t dev;
	sector_t sector;
	sector_t count;
};


/*
 * 'error' is a bitset, with each bit indicating whether an error
 * occurred doing io to the corresponding region.
 */
typedef void (*io_notify_fn)(unsigned int error, void *context);


/*
 * Before anyone uses the IO interface they should call
 * dm_io_get(), specifying roughly how many pages they are
 * expecting to perform io on concurrently.
 *
 * This function may block.
 */
int dm_io_get(unsigned int num_pages);
void dm_io_put(unsigned int num_pages);


/*
 * Synchronous IO.
 *
 * Please ensure that the rw flag in the next two functions is
 * either READ or WRITE, ie. we don't take READA.  Any
 * regions with a zero count field will be ignored.
 */
int dm_io_sync(unsigned int num_regions, struct io_region *where, int rw,
	       struct page *pages, unsigned int offset,
	       unsigned int *error_bits);


/*
 * Aynchronous IO.
 *
 * The 'where' array may be safely allocated on the stack since
 * the function takes a copy.
 */
int dm_io_async(unsigned int num_regions, struct io_region *where, int rw,
		struct page *pages, unsigned int offset,
		io_notify_fn fn, void *context);

#endif
