/*	$NetBSD: mdesc.h,v 1.3.2.3 2015/09/22 12:05:52 skrll Exp $	*/
/*	$OpenBSD: mdesc.h,v 1.3 2014/11/30 22:26:14 kettenis Exp $	*/
/*
 * Copyright (c) 2009 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

struct md_header {
	uint32_t	transport_version;
	uint32_t	node_blk_sz;
	uint32_t	name_blk_sz;
	uint32_t	data_blk_sz;
};

struct md_element {
	uint8_t		tag;
	uint8_t		name_len;
	uint16_t	_reserved_field;
	uint32_t	name_offset;
	union {
		struct {
			uint32_t	data_len;
			uint32_t	data_offset;
		} y;
		uint64_t	val;
	} d;
};

#ifdef _KERNEL
psize_t	mdesc_get_len(void);
void	mdesc_init(vaddr_t, paddr_t, psize_t);
uint64_t mdesc_get_prop_val(int, const char *);
const char *mdesc_get_prop_str(int, const char *);
const char *mdesc_get_prop_data(int, const char *, size_t *);
int	mdesc_find(const char *, uint64_t);
int	mdesc_find_child(int, const char *, uint64_t);
int	mdesc_find_node(const char *);
int	mdesc_find_node_by_idx(int, const char *);
int	mdesc_next_node(int);
const char *mdesc_name_by_idx(int);
#endif
