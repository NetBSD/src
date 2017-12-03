/*	$NetBSD: mdesc.c,v 1.6.16.2 2017/12/03 11:36:45 jdolecek Exp $	*/
/*	$OpenBSD: mdesc.c,v 1.7 2014/11/30 22:26:15 kettenis Exp $	*/
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/autoconf.h>
#include <machine/hypervisor.h>
#include <machine/mdesc.h>
#include <machine/sparc64.h>

vaddr_t mdesc;
paddr_t mdesc_pa;
size_t mdesc_len;

psize_t
mdesc_get_len(void)
{
	psize_t len = 0;

	len = 0;
	hv_mach_desc(0, &len);
	KASSERT(len != 0);

	return len;
}

void
mdesc_init(vaddr_t va, paddr_t pa, psize_t avail_len)
{
	psize_t len;
	int64_t err;

	len = avail_len;
	err = hv_mach_desc(pa, &len);
	if (err != H_EOK)
		panic("hv_mach_desc() failed - err = %" PRId64 "\n", err);
	KASSERT(len <= avail_len);
	mdesc = (vaddr_t)va;
	mdesc_pa = pa;
	mdesc_len = len;

	return;
}

uint64_t
mdesc_get_prop_val(int idx, const char *name)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *str;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;

	while (elem[idx].tag != 'E') {
		str = name_blk + elem[idx].name_offset;
		if (elem[idx].tag == 'v' && strcmp(str, name) == 0)
			return (elem[idx].d.val);
		idx++;
	}

	return (-1);
}

const char *
mdesc_get_prop_str(int idx, const char *name)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *data_blk;
	const char *str;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;
	data_blk = name_blk + hdr->name_blk_sz;

	while (elem[idx].tag != 'E') {
		str = name_blk + elem[idx].name_offset;
		if (elem[idx].tag == 's' && strcmp(str, name) == 0)
			return (data_blk + elem[idx].d.y.data_offset);
		idx++;
	}

	return (NULL);
}

const char *
mdesc_get_prop_data(int idx, const char *name, size_t *len)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *data_blk;
	const char *str;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;
	data_blk = name_blk + hdr->name_blk_sz;

	while (elem[idx].tag != 'E') {
		str = name_blk + elem[idx].name_offset;
		if (elem[idx].tag == 'd' && strcmp(str, name) == 0) {
			*len = elem[idx].d.y.data_len;
			return (data_blk + elem[idx].d.y.data_offset);
		}
		idx++;
	}

	return (NULL);
}

int
mdesc_find(const char *name, uint64_t cfg_handle)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *str;
	uint64_t val;
	int idx;

	hdr = (struct md_header *)mdesc;
	(void)hdr; /* XXX avoid compiler warning */
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));

	for (idx = 0; elem[idx].tag == 'N'; idx = elem[idx].d.val) {
		str = mdesc_get_prop_str(idx, "name");
		val = mdesc_get_prop_val(idx, "cfg-handle");
		if (str && strcmp(str, name) == 0 && val == cfg_handle)
			return (idx);
	}

	return (-1);
}

int
mdesc_find_child(int idx, const char *name, uint64_t cfg_handle)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *str;
	uint64_t val;
	int arc;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;

	for (; elem[idx].tag != 'E'; idx++) {
		str = name_blk + elem[idx].name_offset;
		if (elem[idx].tag != 'a' || strcmp(str, "fwd") != 0)
			continue;

		arc = elem[idx].d.val;
		str = mdesc_get_prop_str(arc, "name");
		val = mdesc_get_prop_val(arc, "cfg-handle");
		if (str && strcmp(str, name) == 0 && val == cfg_handle)
			return (arc);
	}

	return (-1);
}

int
mdesc_find_node_by_idx(int idx, const char *name)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *str;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;

	for ( ; elem[idx].tag == 'N'; idx = elem[idx].d.val) {
		str = name_blk + elem[idx].name_offset;
		if (str && strcmp(str, name) == 0)
			return (idx);
	}

	return (-1);
}

int
mdesc_find_node(const char *name)
{
	return mdesc_find_node_by_idx(0, name);
}

int
mdesc_next_node(int idx)
{
	struct md_element *elem;

	elem = (struct md_element *)(mdesc + sizeof(struct md_header));

	return elem[idx].d.val;
}

const char *
mdesc_name_by_idx(int idx)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *str;

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;

	str = name_blk + elem[idx].name_offset;

	return str;
	
}



