# Copyright (C) 2007 Red Hat, Inc. All rights reserved.
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

#TEST_OPTS=--verbose --debug
SHELL_PATH ?= $(SHELL)
TAR ?= $(TAR)
RM ?= rm -f

subdir := $(shell pwd|sed 's,.*/,,')

srcdir = .
top_srcdir = ..
top_builddir = ..
abs_srcdir = /home/haad/src/LVM2.2.02.56/test
abs_builddir = /home/haad/src/LVM2.2.02.56/test
abs_top_builddir = /home/haad/src/LVM2.2.02.56
abs_top_srcdir = /home/haad/src/LVM2.2.02.56

all: init.sh
	./bin/harness t-*.sh

bin/not: $(srcdir)/not.c .bin-dir-stamp
	$(CC) -o bin/not $<

bin/harness: $(srcdir)/harness.c .bin-dir-stamp
	$(CC) -o bin/harness $<

init.sh: $(srcdir)/Makefile.in $(srcdir)/test-utils.sh .bin-dir-stamp bin/not bin/harness $(T)
	rm -f $@-t $@
	echo 'top_srcdir=$(top_srcdir)' >> $@-t
	echo 'abs_top_builddir=$(abs_top_builddir)' >> $@-t
	echo 'abs_top_srcdir=$(abs_top_builddir)' >> $@-t
	echo 'PATH=$(abs_top_builddir)/test/bin:$$PATH' >> $@-t
	echo 'export LD_LIBRARY_PATH="$(abs_top_builddir)/libdm:$(abs_top_builddir)/liblvm:$(abs_top_builddir)/daemons/dmeventd"' >> $@-t
	echo 'abs_srcdir=$(abs_srcdir)' >> $@-t
	echo 'abs_builddir=$(abs_builddir)' >> $@-t
	echo 'export PATH' >> $@-t
	chmod a-w $@-t
	mv $@-t $@
	-cp $(srcdir)/test-utils.sh .
	-cp $(srcdir)/test-lib.sh .
	-cp $(T) .

# Shell quote;
SHELL_PATH_SQ = $(subst ','\'',$(SHELL_PATH))

T = $(wildcard $(srcdir)/t-*.sh)

ifeq ("no", "yes")
T += lvm2app.sh
all: api/vgtest
api/vgtest:
	$(MAKE) -C api vgtest
endif

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@

#$(T): init.sh
#	./bin/harness $@

.bin-dir-stamp: lvm-wrapper
	rm -rf bin
	mkdir bin
	for i in lvm $$(cat ../tools/.commands); do \
	  ln -s ../lvm-wrapper bin/$$i; \
	done
	ln -s "$(abs_top_builddir)/tools/dmsetup" bin/dmsetup
	touch $@

lvm-wrapper: Makefile
	rm -f $@-t $@
	echo '#!/bin/sh'                                          >  $@-t
	echo 'cmd=$$(echo ./$$0|sed "s,.*/,,")'                   >> $@-t
	echo 'test "$$cmd" = lvm &&'                              >> $@-t
	echo 'exec "$(abs_top_builddir)/tools/lvm" "$$@"'         >> $@-t
	echo 'exec "$(abs_top_builddir)/tools/lvm" "$$cmd" "$$@"' >> $@-t
	chmod a-w,a+x $@-t
	mv $@-t $@

clean:
	rm -rf init.sh lvm-wrapper bin .bin-dir-stamp

distclean: clean
	rm -f Makefile

.PHONY: $(T) clean distclean
.NOTPARALLEL:
