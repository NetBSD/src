/*	$NetBSD: filter-regex.h,v 1.1.1.1 2008/12/22 00:17:58 haad Exp $	*/

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

#ifndef _LVM_FILTER_REGEX_H
#define _LVM_FILTER_REGEX_H

#include "config.h"
#include "dev-cache.h"

/*
 * patterns must be an array of strings of the form:
 * [ra]<sep><regex><sep>, eg,
 * r/cdrom/          - reject cdroms
 * a|loop/[0-4]|     - accept loops 0 to 4
 * r|.*|             - reject everything else
 */

struct dev_filter *regex_filter_create(struct config_value *patterns);

#endif
