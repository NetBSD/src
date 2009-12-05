/*        $NetBSD: libdm-netbsd.h,v 1.1 2009/12/05 11:38:40 haad Exp $

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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

#ifndef __LIB_DM_H__
#define __LIB_DM_H__

#include <prop/proplib.h>

#  define MAJOR(x) major((x))
#  define MINOR(x) minor((x))
#  define MKDEV(x,y) makedev((x),(y))

/* Name of device-mapper driver in kernel */
#define DM_NAME "dm"

/* Types for nbsd_get_dm_major */
#define DM_CHAR_MAJOR 1
#define DM_BLOCK_MAJOR 2

/* libdm_netbsd.c */
int nbsd_get_dm_major(uint32_t *, int); /* Get dm device major numbers */

int nbsd_dmi_add_cmd(const char *, prop_dictionary_t);
int nbsd_dmi_add_version(const int [3], prop_dictionary_t);
int nbsd_dm_add_uint(const char *, uint64_t, prop_dictionary_t);
int nbsd_dm_add_str(const char *, char *, prop_dictionary_t );

struct dm_ioctl* nbsd_dm_dict_to_dmi(prop_dictionary_t, const int);

#endif /* __NETBSD_DM_H__ */
