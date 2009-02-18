###############################################################################
###############################################################################
##
##  Copyright (C) 2009 Red Hat, Inc.  All rights reserved.
##
##  This copyrighted material is made available to anyone wishing to use,
##  modify, copy, or redistribute it subject to the terms and conditions
##  of the GNU General Public License v.2.
##
###############################################################################
###############################################################################

SOURCES = clogd.c cluster.c functions.c link_mon.c local.c logging.c

TARGET = $(shell if [ ! -e /usr/include/linux/dm-clog-tfr.h ]; then \
		echo 'no_clogd_kernel_headers'; \
	elif [ ! -e /usr/include/linux/ext2_fs.h ]; then \
		echo 'no_e2fsprogs_devel'; \
	elif [ ! -e /usr/include/openais/saCkpt.h ]; then \
		echo 'no_openais_devel'; \
	else \
		echo 'clogd'; \
	fi)

ifneq ($(DEBUG), )
CFLAGS += -DDEBUG
endif

ifneq ($(MEMB), )
CFLAGS += -DMEMB
endif

ifneq ($(CKPT), )
CFLAGS += -DCKPT
endif

ifneq ($(RESEND), )
CFLAGS += -DRESEND
endif

CFLAGS += -g

LDFLAGS += $(shell if [ -e /usr/lib64/openais ]; then \
		echo '-L/usr/lib64/openais -L/usr/lib64'; \
	else \
		echo '-L/usr/lib/openais -L/usr/lib'; \
	fi)
LDFLAGS += -lcpg -lSaCkpt -lext2fs

all: ${TARGET}

clogd: ${SOURCES}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

no_clogd_kernel_headers:
	echo "Unable to find clogd kernel headers"
	exit 1

no_e2fsprogs_devel:
	echo "Unable to find ext2fs kernel headers."
	echo "Install 'e2fsprogs-devel'?"
	exit 1

no_openais_devel:
	echo "Unable to find openAIS headers."
	echo "http://sources.redhat.com/cluster/wiki/"
	exit 1

install: clogd
	install -d /usr/sbin
	install clogd /usr/sbin

uninstall:
	rm /usr/sbin/clogd

clean:
	rm -f *.o clogd *~
