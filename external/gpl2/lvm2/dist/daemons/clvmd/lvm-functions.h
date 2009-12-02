/*	$NetBSD: lvm-functions.h,v 1.1.1.2 2009/12/02 00:27:06 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Functions in lvm-functions.c */

#ifndef _LVM_FUNCTIONS_H
#define _LVM_FUNCTIONS_H

extern int pre_lock_lv(unsigned char lock_cmd, unsigned char lock_flags,
		       char *resource);
extern int do_lock_lv(unsigned char lock_cmd, unsigned char lock_flags,
		      char *resource);
extern const char *do_lock_query(char *resource);
extern int post_lock_lv(unsigned char lock_cmd, unsigned char lock_flags,
			char *resource);
extern int do_check_lvm1(const char *vgname);
extern int do_refresh_cache(void);
extern int init_lvm(int using_gulm);
extern void destroy_lvm(void);
extern void init_lvhash(void);
extern void destroy_lvhash(void);
extern void lvm_do_backup(const char *vgname);
extern int hold_unlock(char *resource);
extern int hold_lock(char *resource, int mode, int flags);
extern char *get_last_lvm_error(void);
extern void drop_metadata(const char *vgname);

#endif
