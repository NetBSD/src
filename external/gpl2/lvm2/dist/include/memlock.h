/*	$NetBSD: memlock.h,v 1.1.1.2 2009/12/02 00:25:42 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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

#ifndef LVM_MEMLOCK_H
#define LVM_MEMLOCK_H

struct cmd_context;

void memlock_inc(void);
void memlock_dec(void);
void memlock_inc_daemon(void);
void memlock_dec_daemon(void);
int memlock(void);
void memlock_init(struct cmd_context *cmd);

#endif
