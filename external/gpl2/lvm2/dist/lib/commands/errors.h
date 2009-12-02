/*	$NetBSD: errors.h,v 1.1.1.2 2009/12/02 00:26:26 haad Exp $	*/

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

#ifndef _LVM_ERRORS_H
#define _LVM_ERRORS_H

#define ECMD_PROCESSED		1
#define ENO_SUCH_CMD		2
#define EINVALID_CMD_LINE	3
#define ECMD_FAILED		5

/* FIXME Also returned by cmdlib. */

#endif
