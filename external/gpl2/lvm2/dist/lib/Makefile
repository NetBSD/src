#
# Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
# Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

srcdir = .
top_srcdir = ..
top_builddir = ..


ifeq ("none", "shared")
  SUBDIRS = format1
endif

ifeq ("internal", "shared")
  SUBDIRS += format_pool
endif

ifeq ("internal", "shared")
  SUBDIRS += snapshot
endif

ifeq ("internal", "shared")
  SUBDIRS += mirror
endif

SOURCES =\
	activate/activate.c \
	cache/lvmcache.c \
	commands/toolcontext.c \
	config/config.c \
	datastruct/btree.c \
	datastruct/str_list.c \
	device/dev-cache.c \
	device/dev-io.c \
	device/dev-md.c \
	device/dev-swap.c \
	device/device.c \
	display/display.c \
	error/errseg.c \
	unknown/unknown.c \
	filters/filter-composite.c \
	filters/filter-persistent.c \
	filters/filter-regex.c \
	filters/filter-sysfs.c \
	filters/filter-md.c \
	filters/filter.c \
	format_text/archive.c \
	format_text/archiver.c \
	format_text/export.c \
	format_text/flags.c \
	format_text/format-text.c \
	format_text/import.c \
	format_text/import_vsn1.c \
	format_text/tags.c \
	format_text/text_label.c \
	freeseg/freeseg.c \
	label/label.c \
	locking/file_locking.c \
	locking/locking.c \
	locking/no_locking.c \
	log/log.c \
	metadata/lv_manip.c \
	metadata/merge.c \
	metadata/metadata.c \
	metadata/mirror.c \
	metadata/pv_manip.c \
	metadata/pv_map.c \
	metadata/segtype.c \
	metadata/snapshot_manip.c \
	misc/crc.c \
	misc/lvm-exec.c \
	misc/lvm-file.c \
	misc/lvm-globals.c \
	misc/lvm-string.c \
	misc/lvm-wrappers.c \
	misc/util.c \
	mm/memlock.c \
	report/report.c \
	striped/striped.c \
	uuid/uuid.c \
	zero/zero.c

ifeq ("", "yes")
  SOURCES +=\
	misc/timestamp.c
endif

ifeq ("none", "internal")
  SOURCES +=\
	format1/disk-rep.c \
	format1/format1.c \
	format1/import-export.c \
	format1/import-extents.c \
	format1/layout.c \
	format1/lvm1-label.c \
	format1/vg_number.c
endif

ifeq ("internal", "internal")
  SOURCES +=\
	format_pool/disk_rep.c \
	format_pool/format_pool.c \
	format_pool/import_export.c \
	format_pool/pool_label.c
endif

ifeq ("none", "internal")
  SOURCES += locking/cluster_locking.c
endif

ifeq ("none", "shared")
  SUBDIRS += locking
endif

ifeq ("internal", "internal")
  SOURCES += snapshot/snapshot.c
endif

ifeq ("internal", "internal")
  SOURCES += mirror/mirrored.c
endif

ifeq ("yes", "yes")
  SOURCES +=\
	activate/dev_manager.c \
	activate/fs.c
endif

ifeq ("no", "yes")
  SOURCES +=\
	locking/external_locking.c \
	misc/sharedlib.c
endif

ifeq ("", "yes")
  CLDFLAGS += -L../daemons/dmeventd
  LIBS += -ldevmapper-event
endif

LIB_NAME = liblvm-internal
LIB_STATIC = $(LIB_NAME).a

CLEAN_TARGETS += $(LIB_NAME).cflow

include ../make.tmpl

$(SUBDIRS): $(LIB_STATIC)

$(LIB_NAME).cflow: $(SOURCES)
	set -e; (echo -n "SOURCES += "; \
		 echo $(SOURCES) | \
		 sed "s/^/ /;s/ / $(top_srcdir)\/lib\//g;s/$$//"; \
		 ) > $@

cflow: $(LIB_NAME).cflow
