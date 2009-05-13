/*	$NetBSD: lvm2.h,v 1.1.1.1.2.2 2009/05/13 18:52:41 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _LIB_LVM2_H
#define _LIB_LVM2_H

#include <stdint.h>

/*
 * Library Initialisation
 * FIXME: For now just #define lvm2_create() and lvm2_destroy() to 
 * create_toolcontext() and destroy_toolcontext()
 */
struct arg;
struct cmd_context;
struct cmd_context *create_toolcontext(unsigned is_long_lived);
void destroy_toolcontext(struct cmd_context *cmd);

/*
 * lvm2_create
lvm_handle_t lvm2_create(void);
 *
 * Description: Create an LVM2 handle used in many other APIs.
 *
 * Returns:
 * NULL: Fail - unable to initialise handle.
 * non-NULL: Success - valid LVM2 handle returned
 */
#define lvm2_create(X) create_toolcontext(1)

/*
 * lvm2_destroy
void lvm2_destroy(lvm_handle_t h);
 *
 * Description: Destroy an LVM2 handle allocated with lvm2_create
 *
 * Parameters:
 * - h (IN): handle obtained from lvm2_create
 */
#define lvm2_destroy(X) destroy_toolcontext(X)

#endif
