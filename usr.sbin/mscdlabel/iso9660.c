/* $NetBSD: iso9660.c,v 1.2 2005/09/14 09:41:24 drochner Exp $ */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include <isofs/cd9660/iso.h>

#include "mscdlabel.h"

#define BLKSIZ ISO_DEFAULT_BLOCK_SIZE

static void
printinfo(struct iso_primary_descriptor *vd)
{
	char label[32 + 1], date[] = "yyyy/mm/dd hh:mm", *d;

	strlcpy(label, vd->volume_id, sizeof(label));
	/* strip trailing blanks */
	d = label + strlen(label);
	while (d > label && *(d - 1) == ' ')
		d--;
	*d = '\0';

	d = vd->creation_date;
	memcpy(date, d, 4); /* year */
	memcpy(date + 5, d + 4, 2); /* month */
	memcpy(date + 8, d + 6, 2); /* day */
	memcpy(date + 11, d + 8, 2); /* hour */
	memcpy(date + 14, d + 10, 2); /* min */
	printf("ISO filesystem, label \"%s\", creation time: %s\n",
	       label, date);
}

int
check_primary_vd(int fd, int start, int len)
{
	int i, res, isiso;
	struct iso_primary_descriptor *vd;

	isiso = 0;
	vd = malloc(BLKSIZ);

	for (i = 16; (i < 100) && (i < len); i++) {
		res = pread(fd, vd, BLKSIZ, (start + i) * BLKSIZ);
		if (res < 0) {
			warn("read CD sector %d", start + i);
			break;
		}

		if (memcmp(vd->id, ISO_STANDARD_ID, sizeof(vd->id)))
			continue;
		if (isonum_711(vd->type) == ISO_VD_PRIMARY) {
			printinfo(vd);
			isiso = 1;
			break;
		} else if (isonum_711(vd->type) == ISO_VD_END)
			break;
	}

	free(vd);
	return (isiso);
}
