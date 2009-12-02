#
# Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
# Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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
top_srcdir = .
top_builddir = .


SUBDIRS = doc include man scripts

ifeq ("no", "yes")
  SUBDIRS += udev
endif

ifeq ("no", "yes")
  SUBDIRS += po
endif

SUBDIRS += lib tools daemons libdm

ifeq ("no", "yes")
  SUBDIRS += liblvm
endif

ifeq ($(MAKECMDGOALS),distclean)
  SUBDIRS += daemons/clvmd \
	     daemons/cmirrord \
	     daemons/dmeventd/plugins \
	     daemons/dmeventd \
	     lib/format1 \
	     lib/format_pool \
	     lib/locking \
	     lib/mirror \
	     lib/snapshot \
	     liblvm \
	     udev \
	     test/api \
	     test \
	     po
  DISTCLEAN_TARGETS += lib/misc/configure.h lib/misc/lvm-version.h
  DISTCLEAN_DIRS += lcov_reports*
endif

include make.tmpl

libdm: include
lib: libdm
liblvm: lib
daemons: lib tools
tools: lib device-mapper
po: tools daemons

libdm.device-mapper: include.device-mapper
daemons.device-mapper: libdm.device-mapper
tools.device-mapper: libdm.device-mapper
device-mapper: tools.device-mapper daemons.device-mapper man.device-mapper

ifeq ("no", "yes")
lib.pofile: include.pofile
tools.pofile: lib.pofile
daemons.pofile: lib.pofile
po.pofile: tools.pofile daemons.pofile
pofile: po.pofile
endif

ifneq ("", "")
tools.cflow: lib.cflow
cflow: tools.cflow
endif

ifneq ("", "")
cscope.out: tools
	 -b -R
all: cscope.out
endif

check: all
	$(MAKE) -C test all

ifneq ("", "")
.PHONY: lcov-reset lcov lcov-dated

ifeq ($(MAKECMDGOALS),lcov-dated)
LCOV_REPORTS_DIR=$(top_srcdir)/lcov_reports-$(shell date +%Y%m%d%k%M%S)
else
LCOV_REPORTS_DIR=$(top_srcdir)/lcov_reports
endif

lcov-reset:
	$(LCOV) -d $(top_srcdir)/dmeventd --zerocounters
	$(LCOV) -d $(top_srcdir)/libdm --zerocounters
	$(LCOV) -d $(top_srcdir)/lib --zerocounters
	$(LCOV) -d $(top_srcdir)/tools --zerocounters

lcov: all
	$(RM) -rf $(LCOV_REPORTS_DIR)
	$(MKDIR_P) $(LCOV_REPORTS_DIR)
	$(LCOV) -b $(top_srcdir)/libdm -d $(top_srcdir)/libdm -c -o $(LCOV_REPORTS_DIR)/libdm.info
	$(LCOV) -b $(top_srcdir)/lib -d $(top_srcdir)/lib -c -o $(LCOV_REPORTS_DIR)/lib.info
	$(LCOV) -b $(top_srcdir)/tools -d $(top_srcdir)/tools -c -o $(LCOV_REPORTS_DIR)/tools.info
	DMEVENTD_INFO="$(LCOV_REPORTS_DIR)/dmeventd.info" ;\
	DMEVENTD_INFO_A="-a $$DMEVENTDINFO" ;\
	$(LCOV) -b $(top_srcdir)/dmeventd -d $(top_srcdir)/dmeventd -c -o $$DMEVENTD_INFO || DMEVENTD_INFO_A="" ;\
	$(LCOV) $$DMEVENTD_INFO_A -a $(LCOV_REPORTS_DIR)/lib.info \
		-a $(LCOV_REPORTS_DIR)/libdm.info \
		-a $(LCOV_REPORTS_DIR)/tools.info \
		-o $(LCOV_REPORTS_DIR)/lvm.info
ifneq ("", "")
	$(GENHTML) -o $(LCOV_REPORTS_DIR) -p $(top_srcdir) $(LCOV_REPORTS_DIR)/lvm.info
endif

lcov-dated: lcov

endif
