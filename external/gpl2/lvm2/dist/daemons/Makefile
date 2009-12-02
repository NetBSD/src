#
# Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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


.PHONY: dmeventd clvmd cmirrord

ifneq ("none", "none")
  SUBDIRS = clvmd
endif

ifeq ("no", "yes")
  SUBDIRS += cmirrord
endif

ifeq ("", "yes")
  SUBDIRS += dmeventd
endif

include ../make.tmpl

ifeq ("", "yes")
device-mapper: dmeventd.device-mapper
endif
