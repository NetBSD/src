/*	$NetBSD: ofw_patch.h,v 1.4 2020/10/29 06:47:38 jdc Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
#ifndef OFW_PATCH_H
#define OFW_PATCH_H

#ifdef DEBUG
#define ACDB_BOOTDEV    0x1
#define ACDB_PROBE      0x2
#define ACDB_BOOTARGS   0x4
extern int autoconf_debug;
#define DPRINTF(l, s)   do { if (autoconf_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

void add_gpio_props_v210(device_t, void *);
void add_gpio_props_e250(device_t, void *);
void add_drivebay_props(device_t, int, void *);
void add_spdmem_props_sparcle(device_t);
void add_env_sensors_v210(device_t);
void add_i2c_props_e450(device_t, uint64_t);
void add_i2c_props_e250(device_t, uint64_t);
void set_hw_props(device_t);
void set_static_edid(prop_dictionary_t);

#endif /* OFW_PATCH_H */
