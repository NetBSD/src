/*	$NetBSD: fwnodevar.h,v 1.2 2001/05/03 04:38:33 jmc Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _DEV_IEEE1394_FWNODEVAR_H
#define _DEV_IEEE1394_FWNODEVAR_H

struct fwnode_softc;

struct configrom_data {
	u_int8_t key_type;
	u_int8_t key_value;
	u_int8_t key;
	u_int32_t val;
	struct configrom_leafdata *leafdata;
	TAILQ_ENTRY(configrom_data) data;
};

struct configrom_leafdata {
	u_int8_t desc_type;
	u_int32_t spec_id;
	u_int8_t char_width;
	u_int16_t char_set;
	u_int16_t char_lang;
	u_int32_t datalen;
	char *text;
};

struct configrom_dir {
	TAILQ_HEAD(, configrom_data) data_root;
	TAILQ_HEAD(, configrom_dir) subdir_root;
	TAILQ_ENTRY(configrom_dir) dir;
	struct configrom_dir *parent;
	u_int8_t dir_type;
	u_int32_t offset;
	u_int32_t refs;
};

struct fwnode_device_cap {
	int (*dev_print_data)(struct configrom_data *);
	int (*dev_print_dir)(u_int8_t);
	void (*dev_init)(struct fwnode_softc *, struct fwnode_device_cap *);
	void *dev_data;
	int dev_type;
	int dev_spec;
	int dev_info; /* Lun, etc. */
	int dev_valid;
	struct device *dev_subdev;
	TAILQ_ENTRY(fwnode_device_cap) dev_list;
};

struct fwnode_softc {
	struct ieee1394_softc sc_sc1394;
	
	int sc_flags;
	
	int (*sc1394_input)(struct ieee1394_abuf *);
	int (*sc1394_output)(struct ieee1394_abuf *);
	int (*sc1394_inreg)(struct ieee1394_abuf *, int);
	
	TAILQ_HEAD(, fwnode_device_cap) sc_dev_cap_head;
	TAILQ_HEAD(, configrom_dir) sc_configrom_root;
};

#endif /* _DEV_IEEE1394_FWNODEVAR_H */
