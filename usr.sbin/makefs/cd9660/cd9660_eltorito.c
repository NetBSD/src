/*	$NetBSD: cd9660_eltorito.c,v 1.7 2005/10/30 06:45:46 dyoung Exp $	*/

/*
 * Copyright (c) 2005 Daniel Watt, Walter Deignan, Ryan Gabrys, Alan
 * Perez-Rathke and Ram Vedam.  All rights reserved.
 *
 * This code was written by Daniel Watt, Walter Deignan, Ryan Gabrys,
 * Alan Perez-Rathke and Ram Vedam.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY DANIEL WATT, WALTER DEIGNAN, RYAN
 * GABRYS, ALAN PEREZ-RATHKE AND RAM VEDAM ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DANIEL WATT, WALTER DEIGNAN, RYAN
 * GABRYS, ALAN PEREZ-RATHKE AND RAM VEDAM BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE,DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#include "cd9660.h"
#include "cd9660_eltorito.h"

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: cd9660_eltorito.c,v 1.7 2005/10/30 06:45:46 dyoung Exp $");
#endif  /* !__lint */

#ifdef DEBUG
#define	ELTORITO_DPRINTF(__x)	printf __x
#else
#define	ELTORITO_DPRINTF(__x)
#endif

static struct boot_catalog_entry *cd9660_init_boot_catalog_entry(void);
static struct boot_catalog_entry *cd9660_boot_setup_validation_entry(char);
static struct boot_catalog_entry *cd9660_boot_setup_default_entry(
    struct cd9660_boot_image *);
static struct boot_catalog_entry *cd9660_boot_setup_section_head(char);
static struct boot_catalog_entry *cd9660_boot_setup_validation_entry(char);
#if 0
static u_char cd9660_boot_get_system_type(struct cd9660_boot_image *);
#endif

int
cd9660_add_boot_disk(const char *boot_info)
{
	struct stat stbuf;
	char *temp;
	char *sysname;
	char *filename;
	struct cd9660_boot_image *new_image,*tmp_image;

	assert(boot_info != NULL);

	if (*boot_info == '\0') {
		warnx("Error: Boot disk information must be in the "
		      "format 'system;filename'");
		return 0;
	}

	/* First decode the boot information */
	if ((temp = strdup(boot_info)) == NULL) {
		warn("%s: strdup", __func__);
		return 0;
	}

	sysname = temp;
	filename = strchr(sysname, ';');
	if (filename == NULL) {
		warnx("supply boot disk information in the format "
		    "'system;filename'");
		return 0;
	}

	*filename++ = '\0';

	printf("Found bootdisk with system %s, and filename %s\n",
	    sysname, filename);
	if ((new_image = malloc(sizeof(*new_image))) == NULL) {
		warn("%s: malloc", __func__);
		return 0;
	}
	(void)memset(new_image, 0, sizeof(*new_image));
	new_image->loadSegment = 0;	/* default for now */

	/* Decode System */
	if (strcmp(sysname, "i386") == 0)
		new_image->system = ET_SYS_X86;
	else if (strcmp(sysname, "powerpc") == 0)
		new_image->system = ET_SYS_PPC;
	else if (strcmp(sysname, "macppc") == 0 ||
	         strcmp(sysname, "mac68k") == 0)
		new_image->system = ET_SYS_MAC;
	else {
		warnx("boot disk system must be "
		      "i386, powerpc, macppc, or mac68k");
		return 0;
	}


	if ((new_image->filename = strdup(filename)) == NULL) {
		warn("%s: strdup", __func__);
		return 0;
	}

	free(temp);

	/* Get information about the file */
	if (lstat(new_image->filename, &stbuf) == -1)
		err(EXIT_FAILURE, "%s: lstat(\"%s\")", __func__,
		    new_image->filename);

	switch (stbuf.st_size) {
	case 1440 * 1024:
		new_image->targetMode = ET_MEDIA_144FDD;
		printf("Assigned boot image to 1.44 emulation mode\n");
		break;
	case 1200 * 1024:
		new_image->targetMode = ET_MEDIA_12FDD;
		printf("Assigned boot image to 1.2 emulation mode\n");
		break;
	case 2880 * 1024:
		new_image->targetMode = ET_MEDIA_288FDD;
		printf("Assigned boot image to 2.88 emulation mode\n");
		break;
	default:
		new_image->targetMode = ET_MEDIA_NOEM;
		printf("Assigned boot image to no emulation mode\n");
		break;
	}

	new_image->size = stbuf.st_size;
	new_image->num_sectors =
	    howmany(new_image->size, diskStructure.sectorSize) *
	    howmany(diskStructure.sectorSize, 512);
	printf("New image has size %i, uses %i 512-byte sectors\n",
	    new_image->size, new_image->num_sectors);
	new_image->sector = -1;
	/* Bootable by default */
	new_image->bootable = ET_BOOTABLE;
	/* Add boot disk */

	if (TAILQ_FIRST(&diskStructure.boot_images) == NULL) {
		TAILQ_INSERT_HEAD(&diskStructure.boot_images, new_image,
		    image_list);
	} else {
		tmp_image = TAILQ_FIRST(&diskStructure.boot_images);
		while (TAILQ_NEXT(tmp_image, image_list) != NULL
			&& TAILQ_NEXT(tmp_image, image_list)->system ==
			    new_image->system) {
			tmp_image = TAILQ_NEXT(tmp_image, image_list);
		}
		TAILQ_INSERT_AFTER(&diskStructure.boot_images, tmp_image,
		    new_image, image_list);
	}

	/* TODO : Need to do anything about the boot image in the tree? */
	diskStructure.is_bootable = 1;

	return 1;
}

int
cd9660_eltorito_add_boot_option(const char *option_string, const char *value)
{
	char *eptr;
	struct cd9660_boot_image *image;

	assert(option_string != NULL);

	/* Find the last image added */
	image = TAILQ_FIRST(&diskStructure.boot_images);
	if (image == NULL)
		errx(EXIT_FAILURE, "Attempted to add boot option, "
		    "but no boot images have been specified");

	while (TAILQ_NEXT(image, image_list) != NULL)
		image = TAILQ_NEXT(image, image_list);

	/* TODO : These options are NOT copied yet */
	if (strcmp(option_string, "no-emul-boot") == 0) {
		image->targetMode = ET_MEDIA_NOEM;
	} else if (strcmp(option_string, "no-boot") == 0) {
		image->bootable = ET_NOT_BOOTABLE;
	} else if (strcmp(option_string, "hard-disk-boot") == 0) {
		image->targetMode = ET_MEDIA_HDD;
	} else if (strcmp(option_string, "boot-load-segment") == 0) {
		image->loadSegment = strtoul(value, &eptr, 16);
		if (eptr == value || *eptr != '\0' || errno != ERANGE) {
			warn("%s: strtoul", __func__);
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

static struct boot_catalog_entry *
cd9660_init_boot_catalog_entry(void)
{
	struct boot_catalog_entry *temp;

	if ((temp = malloc(sizeof(*temp))) == NULL)
		return NULL;

	return memset(temp, 0, sizeof(*temp));
}

static struct boot_catalog_entry *
cd9660_boot_setup_validation_entry(char sys)
{
	struct boot_catalog_entry *entry;
	boot_catalog_validation_entry *ve;
	int16_t checksum;
	unsigned char *csptr;
	int i;
	entry = cd9660_init_boot_catalog_entry();

	if (entry == NULL) {
		warnx("Error: memory allocation failed in "
		      "cd9660_boot_setup_validation_entry");
		return 0;
	}
	ve = &entry->entry_data.VE;

	ve->header_id[0] = 1;
	ve->platform_id[0] = sys;
	ve->key[0] = 0x55;
	ve->key[1] = 0xAA;

	/* Calculate checksum */
	checksum = 0;
	cd9660_721(0, ve->checksum);
	csptr = (unsigned char*)ve;
	for (i = 0; i < sizeof(*ve); i += 2) {
		checksum += (int16_t)csptr[i];
		checksum += 256 * (int16_t)csptr[i + 1];
	}
	checksum = -checksum;
	cd9660_721(checksum, ve->checksum);

        ELTORITO_DPRINTF(("%s: header_id %d, platform_id %d, key[0] %d, key[1] %d, "
	    "checksum %04x\n", __func__, ve->header_id[0], ve->platform_id[0],
	    ve->key[0], ve->key[1], checksum));
	return entry;
}

static struct boot_catalog_entry *
cd9660_boot_setup_default_entry(struct cd9660_boot_image *disk)
{
	struct boot_catalog_entry *default_entry;
	boot_catalog_initial_entry *ie;

	default_entry = cd9660_init_boot_catalog_entry();
	if (default_entry == NULL)
		return NULL;

	ie = &default_entry->entry_data.IE;

	ie->boot_indicator[0] = disk->bootable;
	ie->media_type[0] = disk->targetMode;
	cd9660_721(disk->loadSegment, ie->load_segment);
	ie->system_type[0] = disk->system;
	cd9660_721(disk->num_sectors, ie->sector_count);
	cd9660_731(disk->sector, ie->load_rba);

	ELTORITO_DPRINTF(("%s: boot indicator %d, media type %d, "
	    "load segment %04x, system type %d, sector count %d, "
	    "load rba %d\n", __func__, ie->boot_indicator[0],
	    ie->media_type[0], disk->loadSegment, ie->system_type[0],
	    disk->num_sectors, disk->sector));
	return default_entry;
}

static struct boot_catalog_entry *
cd9660_boot_setup_section_head(char platform)
{
	struct boot_catalog_entry *entry;
	boot_catalog_section_header *sh;

	entry = cd9660_init_boot_catalog_entry();
	if (entry == NULL)
		return NULL;

	sh = &entry->entry_data.SH;
	/* More by default. The last one will manually be set to 0x91 */
	sh->header_indicator[0] = ET_SECTION_HEADER_MORE;
	sh->platform_id[0] = platform;
	sh->num_section_entries[0] = 0;
	return entry;
}

static struct boot_catalog_entry *
cd9660_boot_setup_section_entry(struct cd9660_boot_image *disk)
{
	struct boot_catalog_entry *entry;
	boot_catalog_section_entry *se;
	if ((entry = cd9660_init_boot_catalog_entry()) == NULL)
		return NULL;

	se = &entry->entry_data.SE;

	se->boot_indicator[0] = ET_BOOTABLE;
	se->media_type[0] = disk->targetMode;
	cd9660_721(disk->loadSegment, se->load_segment);
	cd9660_721(disk->num_sectors, se->sector_count);
	cd9660_731(disk->sector, se->load_rba);
	return entry;
}

#if 0
static u_char
cd9660_boot_get_system_type(struct cd9660_boot_image *disk)
{
	/*
		For hard drive booting, we need to examine the MBR to figure
		out what the partition type is
	*/
	return 0;
}
#endif

/*
 * Set up the BVD, Boot catalog, and the boot entries, but do no writing
 */
int
cd9660_setup_boot(int first_sector)
{
	int need_head;
	int sector;
	int used_sectors;
	int num_entries = 0;
	int catalog_sectors;
	struct boot_catalog_entry *x86_head, *mac_head, *ppc_head,
		*last_x86, *last_ppc, *last_mac, *last_head,
		*valid_entry, *default_entry, *temp, *head_temp, *next;
	struct cd9660_boot_image *tmp_disk;

	head_temp = NULL;
	need_head = 0;
	x86_head = mac_head = ppc_head =
		last_x86 = last_ppc = last_mac = last_head = NULL;

	/* If there are no boot disks, don't bother building boot information */
	if ((tmp_disk = TAILQ_FIRST(&diskStructure.boot_images)) == NULL)
		return 0;

	/* Point to catalog: For now assume it consumes one sector */
	ELTORITO_DPRINTF(("Boot catalog will go in sector %i\n", first_sector));
	diskStructure.boot_catalog_sector = first_sector;
	cd9660_bothendian_dword(first_sector,
		diskStructure.boot_descriptor->boot_catalog_pointer);

	/* Step 1: Generate boot catalog */
	/* Step 1a: Validation entry */
	valid_entry = cd9660_boot_setup_validation_entry(ET_SYS_X86);
	if (valid_entry == NULL)
		return -1;

	/*
	 * Count how many boot images there are,
	 * and how many sectors they consume.
	 */
	num_entries = 1;
	used_sectors = 0;

	TAILQ_FOREACH(tmp_disk, &diskStructure.boot_images, image_list) {
		used_sectors += tmp_disk->num_sectors;

		/* One default entry per image */
		num_entries++;
	}
	catalog_sectors = howmany(num_entries * 0x20, diskStructure.sectorSize);
	used_sectors += catalog_sectors;

	printf("%s: there will be %i entries consuming %i sectors. "
	       "Catalog is %i sectors\n", __func__, num_entries, used_sectors,
		catalog_sectors);

	/* Populate sector numbers */
	sector = first_sector + catalog_sectors;
	TAILQ_FOREACH(tmp_disk, &diskStructure.boot_images, image_list) {
		tmp_disk->sector = sector;
		sector += tmp_disk->num_sectors;
	}

	LIST_INSERT_HEAD(&diskStructure.boot_entries, valid_entry, ll_struct);

	/* Step 1b: Initial/default entry */
	/* TODO : PARAM */
	tmp_disk = TAILQ_FIRST(&diskStructure.boot_images);
	default_entry = cd9660_boot_setup_default_entry(tmp_disk);
	if (default_entry == NULL) {
		warnx("Error: memory allocation failed in cd9660_setup_boot");
		return -1;
	}

	LIST_INSERT_AFTER(valid_entry, default_entry, ll_struct);

	/* Todo: multiple default entries? */

	tmp_disk = TAILQ_NEXT(tmp_disk, image_list);

	temp = default_entry;

	/* If multiple boot images are given : */
	while (tmp_disk != NULL) {
		/* Step 2: Section header */
		switch (tmp_disk->system) {
		case ET_SYS_X86:
			need_head = (x86_head == NULL);
			if (!need_head)
				head_temp = x86_head;
			break;
		case ET_SYS_PPC:
			need_head = (ppc_head == NULL);
			if (!need_head)
				head_temp = ppc_head;
			break;
		case ET_SYS_MAC:
			need_head = (mac_head == NULL);
			if (!need_head)
				head_temp = mac_head;
			break;
		}

		if (need_head) {
			head_temp =
			    cd9660_boot_setup_section_head(tmp_disk->system);
			if (head_temp == NULL) {
				warnx("Error: memory allocation failed in "
				      "cd9660_setup_boot");
				return -1;
			}
			LIST_INSERT_AFTER(default_entry,head_temp, ll_struct);
		}
		head_temp->entry_data.SH.num_section_entries[0]++;

		/* Step 2a: Section entry and extensions */
		temp = cd9660_boot_setup_section_entry(tmp_disk);
		if (temp == NULL) {
			warnx("Error: memory allocation failed in "
			      "cd9660_setup_boot");
			return -1;
		}

		while ((next = LIST_NEXT(head_temp, ll_struct)) != NULL &&
		       next->entry_type == ET_ENTRY_SE)
			head_temp = LIST_NEXT(head_temp, ll_struct);

		LIST_INSERT_AFTER(head_temp,temp, ll_struct);
		tmp_disk = TAILQ_NEXT(tmp_disk, image_list);
	}

	/* TODO: Remaining boot disks when implemented */

	return first_sector + used_sectors;
}

int
cd9660_setup_boot_volume_descritpor(volume_descriptor *bvd)
{
	boot_volume_descriptor *bvdData =
	    (boot_volume_descriptor*)bvd->volumeDescriptorData;

	bvdData->boot_record_indicator[0] = ISO_VOLUME_DESCRIPTOR_BOOT;
	memcpy(bvdData->identifier, ISO_VOLUME_DESCRIPTOR_STANDARD_ID, 5);
	bvdData->version[0] = 1;
	memcpy(bvdData->boot_system_identifier, ET_ID, 23);
	memcpy(bvdData->identifier, ISO_VOLUME_DESCRIPTOR_STANDARD_ID, 5);
	diskStructure.boot_descriptor =
	    (boot_volume_descriptor*) bvd->volumeDescriptorData;
	return 1;
}

int
cd9660_write_boot(FILE *fd)
{
	struct boot_catalog_entry *e;
	struct cd9660_boot_image *t;

	/* write boot catalog */
	fseek(fd, diskStructure.boot_catalog_sector * diskStructure.sectorSize,
	    SEEK_SET);

	printf("Writing boot catalog to sector %i\n",
	    diskStructure.boot_catalog_sector);
	LIST_FOREACH(e, &diskStructure.boot_entries, ll_struct) {
		printf("Writing catalog entry of type %i\n", e->entry_type);
		/*
		 * It doesnt matter which one gets written
		 * since they are the same size
		 */
		fwrite(&(e->entry_data.VE), 1, 32, fd);
	}
	printf("Finished writing boot catalog\n");

	/* copy boot images */
	TAILQ_FOREACH(t, &diskStructure.boot_images, image_list) {
		printf("Writing boot image from %s to sectors %i\n",
		    t->filename,t->sector);
		cd9660_copy_file(fd, t->sector, t->filename);
	}

	return 0;
}
