#include <sys/types.h>
#include <sys/endian.h>

#include <err.h>
#include <stdio.h>
#include <string.h>

#include <macppc/include/disklabel.h>

/*
 * Creates a file for use by mkisofs's -boot-hfs-file.
 */

int main(int argc, char **argv) {
	struct drvr_map dm;
	struct part_map_entry pme;
	char buf[512];
	int i;

	/* Populate 18 byte driver map header. */

	memset(&dm, 0, sizeof dm);
	dm.sbSig = htobe16(DRIVER_MAP_MAGIC);
	dm.sbBlockSize = htobe16(2048);
	dm.sbBlkCount = htobe32(333000);
	dm.sbDevType = htobe16(1);
	dm.sbDevID = htobe16(1);
	dm.sbData = 0;
	dm.sbDrvrCount = 0;
	fwrite(&dm, sizeof dm, 1, stdout);

	/* Write 2048-byte and 512-byte maps. */

	memset(&pme, 0, sizeof pme);
	pme.pmSig = htobe16(PART_ENTRY_MAGIC);
	pme.pmPartBlkCnt = pme.pmDataCnt = htobe32(1);
	strcpy(pme.pmPartName, "NetBSD_BootBlock");
	strcpy(pme.pmPartType, "Apple_Driver");
	pme.pmPartStatus = htobe32(0x03b);
	pme.pmBootSize = htobe32(0x400);
	pme.pmBootLoad = pme.pmBootEntry = htobe32(0x4000);
	strcpy(pme.pmProcessor, "PowerPC");
	fwrite(&pme, sizeof pme, 1, stdout);
	pme.pmPartBlkCnt = pme.pmDataCnt = htobe32(4);
	fwrite(&pme, sizeof pme, 1, stdout);

	/* Placeholder */

	memset(buf, 0, sizeof buf);
	fwrite(buf, sizeof buf, 1, stdout);

	/* Now inject NetBSD bootblock, padded to 1k */

	fread(buf, sizeof buf, 1, stdin);
	fwrite(buf, sizeof buf, 1, stdout);
	fread(buf, sizeof buf, 1, stdin);
	fwrite(buf, sizeof buf, 1, stdout);

	/* Pad partition to 2k */

	memset(buf, 0, sizeof buf);
	fwrite(buf, sizeof buf, 1, stdout);
	fwrite(buf, sizeof buf, 1, stdout);

	/* HFS "bootblock"; enough to pacify mkisofs. */

	(*(unsigned short *)buf) = htobe16(0x4c4b);
	fwrite(buf, sizeof buf, 1, stdout);

	memset(buf, 0, sizeof buf);
	fwrite(buf, sizeof buf, 1, stdout);

	return 0;
}
