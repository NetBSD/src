/*	$NetBSD: pogen.h,v 1.1.1.2 2009/12/02 00:26:20 haad Exp $	*/

/*
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

/*
 * Macros to change log messages into a format that xgettext can handle.
 *
 * Note that different PRI* definitions lead to different strings for
 * different architectures.
 */

#define print_log(level, dm_errno, file, line, format, args...) print_log(format, args)
#define dm_log(level, file, line, format, args...) dm_log(format, args)
#define dm_log_with_errno(level, dm_errno, file, line, format, args...) \
    dm_log(level, file, line, format, args)

