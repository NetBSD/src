/*	$NetBSD: nubus.c,v 1.10 1995/04/21 02:47:59 briggs Exp $	*/

/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>

#include "nubus.h"

static struct dir *getRsrcByNum __P((struct slot *slot, struct dir *p,
				     int num, struct dir *out, int max));
static int print_rsrcinfo __P((struct slot *slot, struct dir *p));
static char *GetStringInfo __P((struct slot *slot, u_char *data1,
				char *space, int len));
static int printTree __P((struct slot *slot, struct dir *root));
static long GetLongInfo __P((struct slot *slot, u_char *data));
static int FindMagic __P((u_long *data));
static int GetHeader __P((struct slot *slot, u_long pos));
static u_char *IncPtr __P((struct slot *slot, u_char *p, int size));
static char GetByteInfo __P((struct slot *slot, u_char *data));
static int GetRsrcs __P((struct slot *slot, u_char *p,
			 struct dir *dir, int maxdir));
static u_char *getDataAtRsrc __P((struct slot *slot, struct dir *p, int num));
static short GetShortInfo __P((struct slot *slot, u_char *data));

static int 
GetHeader(slot, pos)
	struct slot *slot;
	u_long pos;
{
	/* the pos passed in is the pos that the magic testvalue was found at */
	u_char *p;
	u_char *dirBase;

	switch (slot->size) {
	case 1:		/* char */
		pos -= 14;
		break;
	case 2:
		pos -= 28;
		break;
	case 4:
		pos -= 56;
		break;
	}

	p = (u_char *) pos;

	slot->head.offset = (0xff000000 | (u_long) GetLongInfo(slot, p));
	p = IncPtr(slot, p, 4);

	slot->head.length = GetLongInfo(slot, p);
	p = IncPtr(slot, p, 4);

	slot->head.crc = GetLongInfo(slot, p);
	p = IncPtr(slot, p, 4);

	slot->head.romrev = GetByteInfo(slot, p);
	p = IncPtr(slot, p, 1);

	slot->head.format = GetByteInfo(slot, p);
	p = IncPtr(slot, p, 1);

	slot->head.tst = GetLongInfo(slot, p);
	p = IncPtr(slot, p, 4);

	slot->head.reserved = GetByteInfo(slot, p);
	p = IncPtr(slot, p, 1);

/* byte lanes should be used instead of size, this hasn't bitten me yet */
	slot->head.bytelane = GetByteInfo(slot, p);
	p = IncPtr(slot, p, 1);

	dirBase = (u_char *) (pos + slot->head.offset * slot->size);
	GetRsrcs(slot, dirBase, slot->mainDir, 15);
	return 0;
}

static int
printTree(slot, root)
	struct slot *slot;
	struct dir *root;
{
	struct dir *b;
	u_char *c;
	struct dir *d;
	u_char *e;
	struct dir *f;
	u_char *g;
	struct dir *h;
	u_char *i;
	u_char *j;
	u_char *k;
	struct dir bSpace[15];
	struct dir cSpace[15];
	struct dir dSpace[15];
	struct dir fSpace[15];
	struct dir hSpace[15];
	char    space[40];
/* to get a good idea of what is happening here you should get the
"slots" program from apple dts, it is so cool.  Its the next
best thing to actual docs, which i didn't have...
*/

	b = getRsrcByNum(slot, root, 1, bSpace, 15);
	c = getDataAtRsrc(slot, b, 2);
	d = getRsrcByNum(slot, b, 0x24, dSpace, 15);

	e = getDataAtRsrc(slot, d, 1);

	f = getRsrcByNum(slot, root, 0x80, fSpace, 15);
	g = getDataAtRsrc(slot, f, 2);
	j = getDataAtRsrc(slot, f, 0x0a);
	k = getDataAtRsrc(slot, f, 0x0b);

	h = getRsrcByNum(slot, root, 0xa0, hSpace, 15);
	i = getDataAtRsrc(slot, h, 2);

	printf("A\n");
	print_rsrcinfo(slot, root);

	printf("B\n");
	print_rsrcinfo(slot, b);

	printf("C\n");
	printf("%s\n", GetStringInfo(slot, c, space, 40));

	printf("D\n");
	print_rsrcinfo(slot, d);

	printf("E\n");
	printf("%s\n", GetStringInfo(slot, e, space, 40));

	printf("F\n");
	print_rsrcinfo(slot, f);

	printf("g\n");
	printf("%s\n", GetStringInfo(slot, g, space, 40));
	printf("Video RAM Base %lx\n", GetLongInfo(slot, j));
	printf("Video RAM Length %lx\n", GetLongInfo(slot, k));

	printf("H\n");
	print_rsrcinfo(slot, h);

	printf("I\n");
	printf("%s\n", GetStringInfo(slot, i, space, 40));
}

static int
print_rsrcinfo(slot, p)
	struct slot *slot;
	struct dir *p;
{
	int     i = 0;
	int     failsafe = 20;

	if (p == NULL)
		return 1;
	while (failsafe--) {
		printf("RSRC %02x :%06lx\n", p[i].rsrc, p[i].offset);
		if (p[i].rsrc == 0xff)
			break;
		i++;
	}

}

static struct dir *
getRsrcByNum(slot, p, num, out, max)
	struct slot *slot;
	struct dir *p;
	int num;
	struct dir *out;
	int max;
{
	int     i = 0;
	int     failsafe = 20;
	long    nextoffset = 0;
	u_char *base;

	if (p == NULL)
		return NULL;


	base = getDataAtRsrc(slot, p, num);

	if (NULL == base)
		return NULL;

	GetRsrcs(slot, base, out, max);

	return out;
}

static char *
GetStringInfo(slot, data, space, len)
	struct slot *slot;
	u_char *data;
	char *space;
	int len;
{
	int i;
	char *p = space;

	if (NULL == data)
		return "";

	for (i = 0; (i < len) && *data; i++, p++) {
		*p = GetByteInfo(slot, data);
		data = IncPtr(slot, data, 1);
	}
	*p = '\0';
	return space;
}

static long 
GetLongInfo(slot, data)
	struct slot *slot;
	u_char *data;
{
	long ret = 0;

	switch (slot->size) {
	case 1:
		ret =     (u_long) data[0] << 24 | (u_long) data[1] << 16
			| (u_long) data[2] << 8  | data[3];
		break;
	case 2:
		ret =     (u_long) data[0] << 24 | (u_long) data[2] << 16
			| (u_long) data[4] << 8  | data[6];
		break;
	case 4:
		ret =     (u_long) data[0] << 24 | (u_long) data[4] << 16
			| (u_long) data[8] << 8 | data[12];
		break;
	}

	return ret;
}

static short 
GetShortInfo(slot, data)
	struct slot *slot;
	u_char *data;
{
	short   ret;

	switch (slot->size) {
	case 1:
		ret = (u_long) data[0] << 8 | data[1];
		break;
	case 2:
		ret = (u_long) data[0] << 8 | data[2];
		break;
	case 4:
		ret = (u_long) data[0] << 8 | data[4];
		break;
	}

	return ret;
}

static char 
GetByteInfo(slot, data)
	struct slot *slot;
	u_char *data;
{
	/* boring  .... */
	return data[0];
}


static int 
FindMagic(data)
	u_long data[];
{
	u_short *data2;
	u_char *data3 = (u_char *) data;

	data2 = (u_short *) data;
	data3 = (u_char *) data;

	/* char data */
	if (((data3[0]) == 0x5a) &&
	    ((data3[1]) == 0x93) &&
	    ((data3[2]) == 0x2b) &&
	    ((data3[3]) == 0xc7))
		return 1;

	/* short data */
	if (((data3[0]) == 0x5a) &&
	    ((data3[2]) == 0x93) &&
	    ((data3[4]) == 0x2b) &&
	    ((data3[6]) == 0xc7))
		return 2;

	/* long data */
	if (((data3[0]) == 0x5a) &&
	    ((data3[4]) == 0x93) &&
	    ((data3[8]) == 0x2b) &&
	    ((data3[12]) == 0xc7))
		return 4;

	return 0;
}

static u_char *
IncPtr(slot, p, size)
	struct slot *slot;
	u_char *p;
	int size;
{
	u_char *tmp;

	tmp = p + size * slot->size;

	return tmp;
}

static int 
GetRsrcs(slot, p, dir, maxdir)
	struct slot *slot;
	u_char *p;
	struct dir *dir;
	int maxdir;
{
	int i = 0;

	if (p == NULL)
		return 1;

	while (maxdir--) {
		long    entry;

		entry = GetLongInfo(slot, p);

		dir[i].rsrc = (entry & 0xff000000) >> 24;
		dir[i].offset = entry & 0x00ffffff;
		dir[i].base = (u_long) p;
		p = IncPtr(slot, p, 4);
		if (dir[i].rsrc == 0xff)
			break;
		i++;
	}

	return 0;
}

static u_char *
getDataAtRsrc(slot, p, num)
	struct slot *slot;
	struct dir *p;
	int num;
{
	int i = 0;
	int failsafe = num;
	long nextoffset = 0;
	u_char *base;

	if (p == NULL)
		return NULL;

	while (failsafe--) {
		if (p[i].rsrc == num) {
			base = (u_char *) ((u_long) slot->size * p[i].offset +
			    (u_long) p[i].base);
			return base;
		}
		if (p[i].rsrc == 0xff)
			return NULL;
		i++;
	}

	return NULL;
}


static int 
InitNubusSlot(slotaddr, newslot)
	u_long slotaddr;
	struct slot *newslot;
{
	int i = 0;
	struct slot slot;
	struct dir *b;
	struct dir bSpace[5];
	struct dir *d;
	struct dir dSpace[5];
	struct dir *f;
	struct dir fSpace[5];
	u_char *c;
	u_char *e;
	u_char *g;
	u_long slotend;

	slotend = slotaddr + NBMEMSIZE - 1;

/*
 * If magic is not on the card, then we will quite likely bus error,
 * because we will read a long word when there are only 3 bytes left
 * on the card, unless there is a card in the next slot that has
 * readable memory starting at 0, so more than likely we crash.  Oh well.
 *
 * The directory and the rest of the card can be in different formats.
 * This code won't handle that case.
 */
	for (i = 5; i < 100; i++) {
		if (slot.size = FindMagic((u_long *) (slotend - i))) {
			GetHeader(&slot, slotend - i);
			break;
		}
	}

	if (slot.size) {
		b = getRsrcByNum(&slot, slot.mainDir, 1, bSpace, 5);
		c = getDataAtRsrc(&slot, b, 2);
		d = getRsrcByNum(&slot, b, 0x24, dSpace, 5);
		e = getDataAtRsrc(&slot, d, 1);
		f = getRsrcByNum(&slot, slot.mainDir, 0x80, fSpace, 5);
		g = getDataAtRsrc(&slot, f, 1);
		GetStringInfo(&slot, c, slot.name, 40);

		GetStringInfo(&slot, e, slot.manufacturer, 40);

/* info here is two long words (cat,type,drvrsw,drvrhw) */
		slot.type = (GetLongInfo(&slot, g) & 0xffff0000) >> 16;

	} else
		return 1;

	*newslot = slot;
	return 0;
}

struct imagedata *
NUBUS_GetImageData(slot, Rimage)
	struct slot *slot;
	struct imagedata *Rimage;
{
	struct imagedata image;
	struct dir *dir;
	struct dir dirSpace[10];
	struct dir dirSpace2[10];
	u_char *rawImage;


	dir = getRsrcByNum(slot, slot->mainDir, 128, dirSpace, 10);
	dir = getRsrcByNum(slot, dir, 128, dirSpace2, 10);

	rawImage = getDataAtRsrc(slot, dir, 1);

	image.whatTheHellIsThis = GetLongInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 4);

	image.offset = GetLongInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 4);

	image.rowbytes = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.top = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.left = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.bottom = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.right = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.version = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.packType = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.packSize = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.hRes = GetLongInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 4);

	image.vRes = GetLongInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 4);

	image.pixelType = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	image.pixelSize = GetShortInfo(slot, rawImage);
	rawImage = IncPtr(slot, rawImage, 2);

	*Rimage = image;

	return Rimage;
}

struct nubus_hw nubus_table[NUBUS_MAXSLOTS];

extern int
nubus_addr_to_slot(addr)
	caddr_t addr;
{
	int nubus_num;

	for (nubus_num = 0; nubus_num < NUBUS_MAXSLOTS; nubus_num++)
		if (nubus_table[nubus_num].addr == addr)
			return nubus_num;
	return -1;
}

static void
find_nubus()
{
	/*
	 * This function sets up the array "nubus_table" which contains the
	 * basic information about each card in the Nubus slot.  When device
	 * drivers are initialized later, they can look through this array to
	 * see if their hardware is present and claim it.
	 */

	extern u_long NuBusBase;
	register struct nubus_hw *nu;
	int nubus_num;

	for (nubus_num = 0; nubus_num < NUBUS_MAXSLOTS; nubus_num++)
		nubus_table[nubus_num].found = 0;	/* Empty */

	/*
	 * LAK: For now we can only check 9..F because that's all we map in
	 * locore.s.  Eventually (i.e. near future) we should put THIS
	 * function in locore.s before enabling the MMU and only map the slots
	 * that have a card in them.  Also, the next loop should go from 1 to
	 * 0xF inclusive (0 is "reserved") to cover all possible hardware.
	 * Even if the MacII only has 9..F, it won't hurt us to probe 1..8
	 * also.
	 */
	for (nubus_num = 0; nubus_num < 6; nubus_num++) {
		nu = nubus_table + nubus_num + 9;
		nu->addr = (caddr_t) (NuBusBase + nubus_num * NBMEMSIZE);
		nu->rom = nu->addr + NBROMOFFSET;

		if (!badbaddr(nu->addr + NBMEMSIZE - 1)) {
			InitNubusSlot((u_long) nu->addr, &(nu->slot));

			nu->found = 1;
			nu->claimed = 0;	/* No driver has claimed this
						 * slot yet */

		}
	}
}

static int
nubus_print(aux, name)
	void   *aux;
	char   *name;
{
	struct nubus_hw *nu = (struct nubus_hw *) aux;
	int     i;

	if (name) {
		i = nu - nubus_table;
		printf("%s: s:%d t:%d \"",
		    name, i, nu->slot.type);
		printf("%s, ", nu->slot.name);
		printf("%s\"", nu->slot.manufacturer);
	}
	return (UNCONF);
}

static void
nubus_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;
{
	register struct nubus_hw *nu;
	int     i;

	printf("\n");

	find_nubus();

	for (i = 0; i < 6; i++) {
		nu = nubus_table + i + 9;

		if (!nu->found)
			continue;

		if (config_found(self, nu, nubus_print))
			nu->claimed = 1;
	}
}

extern int matchbyname();

struct cfdriver nubuscd = {
	NULL, "nubus", matchbyname, nubus_attach,
	DV_DULL, sizeof(struct device), 1
};
