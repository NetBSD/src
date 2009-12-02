/*	$NetBSD: vgtest.c,v 1.1.1.1 2009/12/02 00:26:03 haad Exp $	*/

/*
 * Copyright (C) 2009 Red Hat, Inc. All rights reserved.
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
/*
 * Unit test case for vgcreate and related APIs.
 * # gcc -g vgcreate.c -I../../liblvm -I../../include -L../../liblvm \
 *   -L../../libdm -ldevmapper -llvm2app
 * # export LD_LIBRARY_PATH=`pwd`/../../libdm:`pwd`/../../liblvm
 */
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#include "lvm2app.h"

lvm_t handle;
vg_t vg;
const char *vg_name;
#define MAX_DEVICES 16
const char *device[MAX_DEVICES];
uint64_t size = 1024;

#define vg_create(vg_name) \
	printf("Creating VG %s\n", vg_name); \
	vg = lvm_vg_create(handle, vg_name); \
	if (!vg) { \
		fprintf(stderr, "Error creating volume group %s\n", vg_name); \
		goto bad; \
	}
#define vg_extend(vg, dev) \
	printf("Extending VG %s by %s\n", vg_name, dev); \
	status = lvm_vg_extend(vg, dev); \
	if (status) { \
		fprintf(stderr, "Error extending volume group %s " \
			"with device %s\n", vg_name, dev); \
		goto bad; \
	}
#define vg_commit(vg) \
	printf("Committing VG %s to disk\n", vg_name); \
	status = lvm_vg_write(vg); \
	if (status) { \
		fprintf(stderr, "Commit of volume group '%s' failed\n", \
			lvm_vg_get_name(vg)); \
		goto bad; \
	}
#define vg_open(vg_name, mode) \
	printf("Opening VG %s %s\n", vg_name, mode); \
	vg = lvm_vg_open(handle, vg_name, mode, 0); \
	if (!vg) { \
		fprintf(stderr, "Error opening volume group %s\n", vg_name); \
		goto bad; \
	}
#define vg_close(vg) \
	printf("Closing VG %s\n", vg_name); \
	if (lvm_vg_close(vg)) { \
		fprintf(stderr, "Error closing volume group %s\n", vg_name); \
		goto bad; \
	}
#define vg_reduce(vg, dev) \
	printf("Reducing VG %s by %s\n", vg_name, dev); \
	status = lvm_vg_reduce(vg, dev); \
	if (status) { \
		fprintf(stderr, "Error reducing volume group %s " \
			"by device %s\n", vg_name, dev); \
		goto bad; \
	}
#define vg_remove(vg) \
	printf("Removing VG %s from system\n", vg_name); \
	status = lvm_vg_remove(vg); \
	if (status) { \
		fprintf(stderr, "Revmoval of volume group '%s' failed\n", \
			vg_name); \
		goto bad; \
	}

static int init_vgtest(int argc, char *argv[])
{
	int i;

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <vgname> <pv1> <pv2> [... <pvN> ]",
			argv[0]);
		return -1;
	}
	vg_name = argv[1];
	for(i=2; i<MAX_DEVICES && i < argc; i++) {
		device[i-2] = argv[i];
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int status;

	if (init_vgtest(argc, argv) < 0)
		goto bad;

	/* FIXME: make the below messages verbose-only and print PASS/FAIL*/
	printf("Opening LVM\n");
	handle = lvm_init(NULL);
	if (!handle) {
		fprintf(stderr, "Unable to lvm_init\n");
		goto bad;
	}

	printf("Library version: %s\n", lvm_library_get_version());
	vg_create(vg_name);
	vg_extend(vg, device[0]);

	printf("Setting VG %s extent_size to %"PRIu64"\n", vg_name, size);
	status = lvm_vg_set_extent_size(vg, size);
	if (status) {
		fprintf(stderr, "Can not set physical extent "
			"size '%"PRIu64"' for '%s'\n",
			size, vg_name);
		goto bad;
	}

	vg_commit(vg);
	vg_close(vg);

	vg_open(vg_name, "r");
	vg_close(vg);

	vg_open(vg_name, "w");
	vg_extend(vg, device[1]);
	vg_reduce(vg, device[0]);
	vg_commit(vg);
	vg_close(vg);

	vg_open(vg_name, "w");
	vg_extend(vg, device[0]);
	vg_commit(vg);
	vg_close(vg);

	vg_open(vg_name, "w");
	vg_remove(vg);
	vg_commit(vg);
	vg_close(vg);

	lvm_quit(handle);
	printf("liblvm vgcreate unit test PASS\n");
	_exit(0);
bad:
	printf("liblvm vgcreate unit test FAIL\n");
	if (handle && lvm_errno(handle))
		fprintf(stderr, "LVM Error: %s\n", lvm_errmsg(handle));
	if (vg)
		lvm_vg_close(vg);
	if (handle)
		lvm_quit(handle);
	_exit(-1);
}
