/* $NetBSD: infblock.h,v 1.5 2003/03/25 22:48:44 mycroft Exp $ */

/* infblock.h -- header to use infblock.c
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_blocks_state;
typedef struct inflate_blocks_state FAR inflate_blocks_statef;

extern inflate_blocks_statef * inflate_blocks_new __P((
    z_streamp z,
    check_func c,               /* check function */
    uInt w));                   /* window size */

extern int inflate_blocks __P((
    inflate_blocks_statef *,
    z_streamp ,
    int));                      /* initial return code */

extern void inflate_blocks_reset __P((
    inflate_blocks_statef *,
    z_streamp ,
    uLongf *));                  /* check value on output */

extern int inflate_blocks_free __P((
    inflate_blocks_statef *,
    z_streamp));
