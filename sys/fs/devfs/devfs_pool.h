/* 	$NetBSD: devfs_pool.h,v 1.1.2.1 2008/02/18 22:07:02 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
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

#ifndef _FS_DEVFS_DEVFS_POOL_H_
#define _FS_DEVFS_DEVFS_POOL_H_


/* --------------------------------------------------------------------- */

/*
 * devfs_pool is an extension of regular system pools to also hold data
 * specific to devfs.  More specifically, we want a pointer to the
 * devfs_mount structure using the pool so that we can update its memory
 * usage statistics.
 */
struct devfs_pool {
	struct pool		tp_pool;

	/* Reference to the mount point that holds the pool.  This is used
	 * by the devfs_pool_allocator to access and modify the memory
	 * accounting variables for the mount point. */
	struct devfs_mount *	tp_mount;

	/* The pool's name.  Used as the wait channel. */
	char			tp_name[64];
};

/* --------------------------------------------------------------------- */

/*
 * devfs uses variable-length strings to store file names and to store
 * link targets.  Reserving a fixed-size buffer for each of them is
 * inefficient because it will consume a lot more memory than is really
 * necessary.  However, managing variable-sized buffers is difficult as
 * regards memory allocation and very inefficient in computation time.
 * This is why devfs provides an hybrid scheme to store strings: string
 * pools.
 *
 * A string pool is a collection of memory pools, each one with elements
 * of a fixed size.  In devfs's case, a string pool contains independent
 * memory pools for 16-byte, 32-byte, 64-byte, 128-byte, 256-byte,
 * 512-byte and 1024-byte long objects.  Whenever an object is requested
 * from the pool, the new object's size is rounded to the closest upper
 * match and an item from the corresponding pool is returned.
 */
struct devfs_str_pool {
	struct devfs_pool	tsp_pool_16;
	struct devfs_pool	tsp_pool_32;
	struct devfs_pool	tsp_pool_64;
	struct devfs_pool	tsp_pool_128;
	struct devfs_pool	tsp_pool_256;
	struct devfs_pool	tsp_pool_512;
	struct devfs_pool	tsp_pool_1024;
};

/* --------------------------------------------------------------------- */
#ifdef _KERNEL

/*
 * Convenience functions and macros to manipulate a devfs_pool.
 */

void	devfs_pool_init(struct devfs_pool *tpp, size_t size,
	    const char *what, struct devfs_mount *tmp);
void	devfs_pool_destroy(struct devfs_pool *tpp);

#define	DEVFS_POOL_GET(tpp, flags) pool_get((struct pool *)(tpp), flags)
#define	DEVFS_POOL_PUT(tpp, v) pool_put((struct pool *)(tpp), v)

/* --------------------------------------------------------------------- */

/*
 * Functions to manipulate a devfs_str_pool.
 */

void	devfs_str_pool_init(struct devfs_str_pool *, struct devfs_mount *);
void	devfs_str_pool_destroy(struct devfs_str_pool *);
char *	devfs_str_pool_get(struct devfs_str_pool *, size_t, int);
void	devfs_str_pool_put(struct devfs_str_pool *, char *, size_t);

#endif

#endif /* _FS_DEVFS_DEVFS_POOL_H_ */
