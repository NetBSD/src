/*	$NetBSD: parse_rx.h,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DM_PARSE_REGEX_H
#define _DM_PARSE_REGEX_H

enum {
	CAT,
	STAR,
	PLUS,
	OR,
	QUEST,
	CHARSET
};

/*
 * We're never going to be running the regex on non-printable
 * chars, so we can use a couple of these chars to represent the
 * start and end of a string.
 */
#define HAT_CHAR 0x2
#define DOLLAR_CHAR 0x3

struct rx_node {
	int type;
	dm_bitset_t charset;
	struct rx_node *left, *right;

	/* used to build the dfa for the toker */
	int nullable, final;
	dm_bitset_t firstpos;
	dm_bitset_t lastpos;
	dm_bitset_t followpos;
};

struct rx_node *rx_parse_str(struct dm_pool *mem, const char *str);
struct rx_node *rx_parse_tok(struct dm_pool *mem,
			     const char *begin, const char *end);

#endif
/*	$NetBSD: parse_rx.h,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DM_PARSE_REGEX_H
#define _DM_PARSE_REGEX_H

enum {
	CAT,
	STAR,
	PLUS,
	OR,
	QUEST,
	CHARSET
};

/*
 * We're never going to be running the regex on non-printable
 * chars, so we can use a couple of these chars to represent the
 * start and end of a string.
 */
#define HAT_CHAR 0x2
#define DOLLAR_CHAR 0x3

struct rx_node {
	int type;
	dm_bitset_t charset;
	struct rx_node *left, *right;

	/* used to build the dfa for the toker */
	int nullable, final;
	dm_bitset_t firstpos;
	dm_bitset_t lastpos;
	dm_bitset_t followpos;
};

struct rx_node *rx_parse_str(struct dm_pool *mem, const char *str);
struct rx_node *rx_parse_tok(struct dm_pool *mem,
			     const char *begin, const char *end);

#endif
