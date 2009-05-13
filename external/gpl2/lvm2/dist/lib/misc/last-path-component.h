/*	$NetBSD: last-path-component.h,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2007 Red Hat, Inc. All rights reserved.
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

/*
 * Return the address of the last file name component of NAME.
 * If NAME ends in a slash, return the empty string.
 */

#include <string.h>

static inline const char *last_path_component(char const *name)
{
	char const *slash = strrchr(name, '/');

	return (slash) ? slash + 1 : name;
}
