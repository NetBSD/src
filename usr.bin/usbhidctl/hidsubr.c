/*	$NetBSD: hidsubr.c,v 1.5 1999/04/21 16:23:14 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@netbsd.org).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>
#include <ctype.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include "hidsubr.h"

#define MAXUSAGE 100
struct hid_data {
	u_char *start;
	u_char *end;
	u_char *p;
	struct hid_item cur;
	u_int32_t usages[MAXUSAGE];
	int nusage;
	int minset;
	int multi;
	int multimax;
	int kindset;
};

static int min(int x, int y) { return x < y ? x : y; }

static void
hid_clear_local(struct hid_item *c)
{
	c->usage = 0;
	c->usage_minimum = 0;
	c->usage_maximum = 0;
	c->designator_index = 0;
	c->designator_minimum = 0;
	c->designator_maximum = 0;
	c->string_index = 0;
	c->string_minimum = 0;
	c->string_maximum = 0;
	c->set_delimiter = 0;
}

struct hid_data *
hid_start_parse(u_char *d, int len, int kindset)
{
	struct hid_data *s;

	s = malloc(sizeof *s);
	memset(s, 0, sizeof *s);
	s->start = s->p = d;
	s->end = d + len;
	s->kindset = kindset;
	return (s);
}

void
hid_end_parse(struct hid_data *s)
{
	while (s->cur.next) {
		struct hid_item *hi = s->cur.next->next;
		free(s->cur.next);
		s->cur.next = hi;
	}
	free(s);
}

int
hid_get_item(struct hid_data *s, struct hid_item *h)
{
	struct hid_item *c = &s->cur;
	int bTag = 0, bType = 0, bSize;
	u_char *data;
	int32_t dval;
	u_char *p;
	struct hid_item *hi;
	int i;

 top:
	if (s->multimax) {
		if (s->multi < s->multimax) {
			c->usage = s->usages[min(s->multi, s->nusage-1)];
			s->multi++;
			*h = *c;
			c->pos += c->report_size;
			h->next = 0;
			return (1);
		} else {
			c->report_count = s->multimax;
			s->multimax = 0;
			s->nusage = 0;
			hid_clear_local(c);
		}
	}
	for (;;) {
		p = s->p;
		if (p >= s->end)
			return (0);

		bSize = *p++;
		if (bSize == 0xfe) {
			/* long item */
			bSize = *p++;
			bSize |= *p++ << 8;
			bTag = *p++;
			data = p;
			p += bSize;
		} else {
			/* short item */
			bTag = bSize >> 4;
			bType = (bSize >> 2) & 3;
			bSize &= 3;
			if (bSize == 3) bSize = 4;
			data = p;
			p += bSize;
		}
		s->p = p;
		switch(bSize) {
		case 0:
			dval = 0;
			break;
		case 1:
			dval = (int8_t)*data++;
			break;
		case 2:
			dval = *data++;
			dval |= *data++ << 8;
			dval = (int16_t)dval;
			break;
		case 4:
			dval = *data++;
			dval |= *data++ << 8;
			dval |= *data++ << 16;
			dval |= *data++ << 24;
			break;
		default:
			printf("BAD LENGTH %d\n", bSize);
			continue;
		}
		
		switch (bType) {
		case 0:			/* Main */
			switch (bTag) {
			case 8:		/* Input */
				if (!(s->kindset & (1 << hid_input)))
					continue;
				c->kind = hid_input;
				c->flags = dval;
			ret:
				if (c->flags & HIO_VARIABLE) {
					s->multimax = c->report_count;
					s->multi = 0;
					c->report_count = 1;
					if (s->minset) {
						for (i = c->usage_minimum; 
						     i <= c->usage_maximum; 
						     i++) {
							s->usages[s->nusage] = i;
							if (s->nusage < MAXUSAGE-1)
								s->nusage++;
						}
						s->minset = 0;
					}
					goto top;
				} else {
					if (s->minset) 
						c->usage = c->usage_minimum;
					*h = *c;
					h->next = 0;
					c->pos += c->report_size * c->report_count;
					hid_clear_local(c);
					s->minset = 0;
					return (1);
				}
			case 9:		/* Output */
				if (!(s->kindset & (1 << hid_output)))
					continue;
				c->kind = hid_output;
				c->flags = dval;
				goto ret;
			case 10:	/* Collection */
				c->kind = hid_collection;
				c->collection = dval;
				c->collevel++;
				*h = *c;
				hid_clear_local(c);
				s->nusage = 0;
				return (1);
			case 11:	/* Feature */
				if (!(s->kindset & (1 << hid_feature)))
					continue;
				c->kind = hid_feature;
				c->flags = dval;
				goto ret;
			case 12:	/* End collection */
				c->kind = hid_endcollection;
				c->collevel--;
				*h = *c;
				hid_clear_local(c);
				s->nusage = 0;
				return (1);
			default:
				printf("Main bTag=%d\n", bTag);
				break;
			}
			break;
		case 1:		/* Global */
			switch (bTag) {
			case 0:
				c->_usage_page = dval << 16;
				break;
			case 1:
				c->logical_minimum = dval;
				break;
			case 2:
				c->logical_maximum = dval;
				break;
			case 3:
				c->physical_maximum = dval;
				break;
			case 4:
				c->physical_maximum = dval;
				break;
			case 5:
				c->unit_exponent = dval;
				break;
			case 6:
				c->unit = dval;
				break;
			case 7:
				c->report_size = dval;
				break;
			case 8:
				c->report_ID = dval;
				break;
			case 9:
				c->report_count = dval;
				break;
			case 10: /* Push */
				hi = malloc(sizeof *hi);
				*hi = s->cur;
				c->next = hi;
				break;
			case 11: /* Pop */
				hi = c->next;
				s->cur = *hi;
				free(hi);
				break;
			default:
				printf("Global bTag=%d\n", bTag);
				break;
			}
			break;
		case 2:		/* Local */
			switch (bTag) {
			case 0:
				if (bSize == 1) 
					dval = c->_usage_page | (dval&0xff);
				else if (bSize == 2) 
					dval = c->_usage_page | (dval&0xffff);
				c->usage = dval;
				if (s->nusage < MAXUSAGE)
					s->usages[s->nusage++] = dval;
				/* else XXX */
				break;
			case 1:
				s->minset = 1;
				if (bSize == 1) 
					dval = c->_usage_page | (dval&0xff);
				else if (bSize == 2) 
					dval = c->_usage_page | (dval&0xffff);
				c->usage_minimum = dval;
				break;
			case 2:
				if (bSize == 1) 
					dval = c->_usage_page | (dval&0xff);
				else if (bSize == 2) 
					dval = c->_usage_page | (dval&0xffff);
				c->usage_maximum = dval;
				break;
			case 3:
				c->designator_index = dval;
				break;
			case 4:
				c->designator_minimum = dval;
				break;
			case 5:
				c->designator_maximum = dval;
				break;
			case 7:
				c->string_index = dval;
				break;
			case 8:
				c->string_minimum = dval;
				break;
			case 9:
				c->string_maximum = dval;
				break;
			case 10:
				c->set_delimiter = dval;
				break;
			default:
				printf("Local bTag=%d\n", bTag);
				break;
			}
			break;
		default:
			printf("default bType=%d\n", bType);
			break;
		}
	}
}

int 
hid_report_size(u_char *buf, int len, enum hid_kind k, int *idp)
{
	struct hid_data *d;
	struct hid_item h;
	int size, id;

	id = 0;
	if (idp)
		*idp = 0;
	memset(&h, 0, sizeof h);
	for (d = hid_start_parse(buf, len, 1<<k); hid_get_item(d, &h); ) {
		if (h.report_ID != 0) {
			if (idp)
				*idp = h.report_ID;
			id = 8;
		}
	}
	hid_end_parse(d);
	size = h.pos + id;
	return ((size + 7) / 8);
}

struct usage_in_page {
	char *name;
	int usage;
};

struct usage_page {
	char *name;
	int usage;
	struct usage_in_page *page_contents;
	int pagesize, pagesizemax;
} *pages;
int npages, npagesmax;

void
dump_hid_table(void)
{
	int i, j;

	for (i = 0; i < npages; i++) {
		printf("%d\t%s\n", pages[i].usage, pages[i].name);
		for (j = 0; j < pages[i].pagesize; j++) {
			printf("\t%d\t%s\n", pages[i].page_contents[j].usage,
			       pages[i].page_contents[j].name);
		}
	}
}

void
init_hid(char *hidname)
{
	FILE *f;
	char line[100], name[100], *p, *n;
	int no;
	int lineno;
	struct usage_page *curpage = 0;

	f = fopen(hidname, "r");
	if (f == NULL)
		err(1, "%s", hidname);
	for (lineno = 1; ; lineno++) {
		if (fgets(line, sizeof line, f) == NULL)
			break;
		if (line[0] == '#')
			continue;
		for (p = line; *p && isspace(*p); p++)
			;
		if (!*p)
			continue;
		if (sscanf(line, " * %[^\n]", name) == 1)
			no = -1;
		else if (sscanf(line, " 0x%x %[^\n]", &no, name) != 2 &&
			 sscanf(line, " %d %[^\n]", &no, name) != 2)
			errx(1, "file %s, line %d, syntax error\n",
			     hidname, lineno);
		for (p = name; *p; p++)
			if (isspace(*p) || *p == '.')
				*p = '_';
		n = strdup(name);
		if (!n)
			err(1, "strdup");
		if (isspace(line[0])) {
			if (!curpage)
				errx(1, "file %s, line %d, syntax error\n",
				     hidname, lineno);
			if (curpage->pagesize >= curpage->pagesizemax) {
				curpage->pagesizemax += 10;
				curpage->page_contents = 
					realloc(curpage->page_contents,
						curpage->pagesizemax * 
						sizeof (struct usage_in_page));
				if (!curpage->page_contents)
					err(1, "realloc");
			}
			curpage->page_contents[curpage->pagesize].name = n;
			curpage->page_contents[curpage->pagesize].usage = no;
			curpage->pagesize++;
		} else {
			if (npages >= npagesmax) {
				if (pages == 0) {
					npagesmax = 5;
					pages = malloc(npagesmax * 
						  sizeof (struct usage_page));
				} else {
					npagesmax += 5;
					pages = realloc(pages, 
						   npagesmax * 
						   sizeof (struct usage_page));
				}
				if (!pages)
					err(1, "alloc");
			}
			curpage = &pages[npages++];
			curpage->name = n;
			curpage->usage = no;
			curpage->pagesize = 0;
			curpage->pagesizemax = 10;
			curpage->page_contents = 
				malloc(curpage->pagesizemax * 
				       sizeof (struct usage_in_page));
			if (!curpage->page_contents)
				err(1, "malloc");
		}
	}
	fclose(f);
	/*dump_hid_table();*/
}

char *
usage_page(int i)
{
	static char b[10];
	int k;

	if (!pages)
		errx(1, "no hid table\n");

	for (k = 0; k < npages; k++)
		if (pages[k].usage == i)
			return pages[k].name;
	sprintf(b, "0x%02x", i);
	return b;
}

char *
usage_in_page(unsigned int u)
{
	int page = HID_PAGE(u);
	int i = HID_USAGE(u);
	static char b[100];
	int j, k, us;

	for (k = 0; k < npages; k++)
		if (pages[k].usage == page)
			break;
	if (k >= npages)
		goto bad;
	for (j = 0; j < pages[k].pagesize; j++) {
		us = pages[k].page_contents[j].usage;
		if (us == -1) {
			sprintf(b, pages[k].page_contents[j].name, i);
			return b;
		}
		if (us == i)
			return pages[k].page_contents[j].name;
	}
 bad:
	sprintf(b, "0x%02x", i);
	return b;
}

