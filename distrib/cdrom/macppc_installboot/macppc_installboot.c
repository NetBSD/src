#include <sys/types.h>
#include <sys/endian.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <macppc/include/disklabel.h>

int main(int argc, char **argv) {
	u_int32_t ofwbootblk;
	int bootxxoff;
	struct part_map_entry pme;
	FILE *imagef;

	if (argc != 4)
		errx(1, "Usage: %s imagefile ofwboot_512block bootxx_offset", argv[0]);

	if (!(imagef = fopen(argv[1], "rb+")))
		err(1, "error opening %s", argv[1]);

	ofwbootblk = htobe32(atoi(argv[2]));
	bootxxoff = atoi(argv[3]);

	/* Find the bootxx file from the second (NetBSD_BootBlock) partition. */

	fseek(imagef, 0x400, SEEK_SET);
	fread(&pme, sizeof pme, 1, imagef);

	if (strcmp(pme.pmPartName, "NetBSD_BootBlock"))
		errx(1, "not a valid image file: %s", argv[1]);

	fseek(imagef, htobe32(pme.pmPyPartStart) * 512 + bootxxoff, SEEK_SET);
	fwrite(&ofwbootblk, sizeof ofwbootblk, 1, imagef);

	fclose(imagef);

	return 0;
}
