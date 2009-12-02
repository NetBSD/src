/*	$NetBSD: text_export.h,v 1.1.1.2 2009/12/02 00:25:44 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_TEXT_EXPORT_H
#define _LVM_TEXT_EXPORT_H

#define outf(args...) do {if (!out_text(args)) return_0;} while (0)
#define outnl(f) do {if (!out_newline(f)) return_0;} while (0)

struct formatter;
struct lv_segment;
struct config_node;

int out_size(struct formatter *f, uint64_t size, const char *fmt, ...)
    __attribute__ ((format(printf, 3, 4)));

int out_hint(struct formatter *f, const char *fmt, ...)
    __attribute__ ((format(printf, 2, 3)));

int out_text(struct formatter *f, const char *fmt, ...)
    __attribute__ ((format(printf, 2, 3)));

int out_config_node(struct formatter *f, const struct config_node *cn);

int out_areas(struct formatter *f, const struct lv_segment *seg,
	      const char *type);

void out_inc_indent(struct formatter *f);
void out_dec_indent(struct formatter *f);
int out_newline(struct formatter *f);

#endif
