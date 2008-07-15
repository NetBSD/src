/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_LABEL_H
#define _LVM_LABEL_H

#include "uuid.h"
#include "device.h"

#define LABEL_ID "LABELONE"
#define LABEL_SIZE SECTOR_SIZE	/* Think very carefully before changing this */
#define LABEL_SCAN_SECTORS 4L
#define LABEL_SCAN_SIZE (LABEL_SCAN_SECTORS << SECTOR_SHIFT)

struct labeller;

/* On disk - 32 bytes */
struct label_header {
	int8_t id[8];		/* LABELONE */
	uint64_t sector_xl;	/* Sector number of this label */
	uint32_t crc_xl;	/* From next field to end of sector */
	uint32_t offset_xl;	/* Offset from start of struct to contents */
	int8_t type[8];		/* LVM2 001 */
} __attribute__ ((packed));

/* In core */
struct label {
	char type[8];
	uint64_t sector;
	struct labeller *labeller;
	void *info;
};

struct labeller;

struct label_ops {
	/*
	 * Is the device labelled with this format ?
	 */
	int (*can_handle) (struct labeller * l, void *buf, uint64_t sector);

	/*
	 * Write a label to a volume.
	 */
	int (*write) (struct label * label, void *buf);

	/*
	 * Read a label from a volume.
	 */
	int (*read) (struct labeller * l, struct device * dev,
		     void *buf, struct label ** label);

	/*
	 * Additional consistency checks for the paranoid.
	 */
	int (*verify) (struct labeller * l, void *buf, uint64_t sector);

	/*
	 * Populate label_type etc.
	 */
	int (*initialise_label) (struct labeller * l, struct label * label);

	/*
	 * Destroy a previously read label.
	 */
	void (*destroy_label) (struct labeller * l, struct label * label);

	/*
	 * Destructor.
	 */
	void (*destroy) (struct labeller * l);
};

struct labeller {
	struct label_ops *ops;
	const void *private;
};

int label_init(void);
void label_exit(void);

int label_register_handler(const char *name, struct labeller *handler);

struct labeller *label_get_handler(const char *name);

int label_remove(struct device *dev);
int label_read(struct device *dev, struct label **result,
		uint64_t scan_sector);
int label_write(struct device *dev, struct label *label);
int label_verify(struct device *dev);
struct label *label_create(struct labeller *labeller);
void label_destroy(struct label *label);

#endif
