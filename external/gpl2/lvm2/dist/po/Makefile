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


LANGS=de

TARGETS=$(LANGS:%=lvm2_%.mo) $(LANGS:%=dm_%.mo)

DM_POSOURCES = $(top_srcdir)/dmsetup/*.pot $(top_srcdir)/libdm/*.pot \
	       $(top_srcdir)/libdm/*/*.pot

LVM_POSOURCES = $(top_srcdir)/tools/*.pot $(top_srcdir)/lib/*/*.pot

include ../make.tmpl

lvm2.po: Makefile $(LVM_POSOURCES)
	@echo Compiling string table
	@xgettext -C -F --keyword=print_log --keyword=log_debug \
		--keyword=log_info --keyword=_ --keyword=N_ \
		--keyword=log_notice --keyword=log_warn --keyword=log_err \
		--keyword=log_fatal --keyword=log_debug --keyword=log_error \
		--keyword=log_print --keyword=log_verbose \
		--keyword=log_very_verbose -d - \
		$(LVM_POSOURCES) > $@

device-mapper.po: Makefile $(DM_POSOURCES)
	@echo Compiling string table
	@xgettext -C -F --keyword=dm_log --keyword=log_debug \
		--keyword=log_info --keyword=_ --keyword=N_ \
		--keyword=log_notice --keyword=log_warn --keyword=log_err \
		--keyword=log_fatal --keyword=log_debug --keyword=log_error \
		--keyword=log_print --keyword=log_verbose \
		--keyword=log_very_verbose -d - \
		$(DM_POSOURCES) > $@

pofile: lvm2.po device-mapper.po

# FIXME
install: $(TARGETS)
	@echo Installing translation files in $(localedir)
	@( \
		for lang in $(LANGS); do \
			$(INSTALL) -D $(OWNER) $(GROUP) -m 444 $$lang.mo \
			    $(localedir)/$$lang/LC_MESSAGES/lvm2.mo;\
	  done; \
	)
	@( \
		for lang in $(LANGS); do \
			$(INSTALL) -D $(OWNER) $(GROUP) -m 444 $$lang.mo \
			    $(localedir)/$$lang/LC_MESSAGES/device-mapper.mo;\
	  done; \
	)
