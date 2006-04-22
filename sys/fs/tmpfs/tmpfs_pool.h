/*	$NetBSD: tmpfs_pool.h,v 1.4.6.1 2006/04/22 11:39:58 simonb Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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

#ifndef _FS_TMPFS_TMPFS_POOL_H_
#define _FS_TMPFS_TMPFS_POOL_H_


/* --------------------------------------------------------------------- */

/*
 * tmpfs_pool is an extension of regular system pools to also hold data
 * specific to tmpfs.  More specifically, we want a pointer to the
 * tmpfs_mount structure using the pool so that we can update its memory
 * usage statistics.
 */
struct tmpfs_pool {
	struct pool		tp_pool;

	/* Reference to the mount point that holds the pool.  This is used
	 * by the tmpfs_pool_allocator to access and modify the memory
	 * accounting variables for the mount point. */
	struct tmpfs_mount *	tp_mount;

	/* The pool's name.  Used as the wait channel. */
	char			tp_name[64];
};

/* --------------------------------------------------------------------- */

/*
 * tmpfs uses variable-length strings to store file names and to store
 * link targets.  Reserving a fixed-size buffer for each of them is
 * inefficient because it will consume a lot more memory than is really
 * necessary.  However, managing variable-sized buffers is difficult as
 * regards memory allocation and very inefficient in computation time.
 * This is why tmpfs provides an hybrid scheme to store strings: string
 * pools.
 *
 * A string pool is a collection of memory pools, each one with elements
 * of a fixed size.  In tmpfs's case, a string pool contains independent
 * memory pools for 16-byte, 32-byte, 64-byte, 128-byte, 256-byte,
 * 512-byte and 1024-byte long objects.  Whenever an object is requested
 * from the pool, the new object's size is rounded to the closest upper
 * match and an item from the corresponding pool is returned.
 */
struct tmpfs_str_pool {
	struct tmpfs_pool	tsp_pool_16;
	struct tmpfs_pool	tsp_pool_32;
	struct tmpfs_pool	tsp_pool_64;
	struct tmpfs_pool	tsp_pool_128;
	struct tmpfs_pool	tsp_pool_256;
	struct tmpfs_pool	tsp_pool_512;
	struct tmpfs_pool	tsp_pool_1024;
};

/* --------------------------------------------------------------------- */
#ifdef _KERNEL

/*
 * Convenience functions and macros to manipulate a tmpfs_pool.
 */

void	tmpfs_pool_init(struct tmpfs_pool *tpp, size_t size,
	    const char *what, struct tmpfs_mount *tmp);
void	tmpfs_pool_destroy(struct tmpfs_pool *tpp);

#define	TMPFS_POOL_GET(tpp, flags) pool_get((struct pool *)(tpp), flags)
#define	TMPFS_POOL_PUT(tpp, v) pool_put((struct pool *)(tpp), v)

/* --------------------------------------------------------------------- */

/*
 * Functions to manipulate a tmpfs_str_pool.
 */

void	tmpfs_str_pool_init(struct tmpfs_str_pool *, struct tmpfs_mount *);
void	tmpfs_str_pool_destroy(struct tmpfs_str_pool *);
char *	tmpfs_str_pool_get(struct tmpfs_str_pool *, size_t, int);
void	tmpfs_str_pool_put(struct tmpfs_str_pool *, char *, size_t);

#endif

#endif /* _FS_TMPFS_TMPFS_POOL_H_ */
