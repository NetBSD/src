/*	$NetBSD: kdev_t.h,v 1.1.1.1 2008/12/22 00:18:36 haad Exp $	*/

/*
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

#ifndef _LIBDM_KDEV_H
#define _LIBDM_KDEV_H

#define MAJOR(dev)      ((dev & 0xfff00) >> 8)
#define MINOR(dev)      ((dev & 0xff) | ((dev >> 12) & 0xfff00))
#define MKDEV(ma,mi)    ((mi & 0xff) | (ma << 8) | ((mi & ~0xff) << 12))

#endif
