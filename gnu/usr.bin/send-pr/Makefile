#
# Makefile for building a standalone send-pr.
#

VERSION=3.01.6
RELEASE=NetBSD-current
GNATS_HOST=sun-lamp

BINDIR=		/usr/bin
LIBDIR=		/usr/lib

NOPROG=	
MAN1=		send-pr.0

CLEANFILES=	send-pr install-sid

all:		send-pr install-sid send-pr.1

send-pr:	send-pr.sh
	sed -e 's/@VERSION@/$(VERSION)/g' \
	    -e 's/@DEFAULT_RELEASE@/$(RELEASE)/g' \
		${.CURDIR}/send-pr.sh > send-pr

send-pr.1:	send-pr.man
	sed -e 's/@VERSION@/$((VERSION)/g' \
		${.CURDIR}/send-pr.man > send-pr.1

install-sid:	install-sid.sh
	sed -e 's/@VERSION@/$(VERSION)/g' \
		${.CURDIR}/install-sid.sh > install-sid

beforeinstall:
	install -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} send-pr \
		${DESTDIR}${BINDIR}
	install -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} install-sid \
		${DESTDIR}${BINDIR}

	install -d -o root -g wheel -m 755 ${DESTDIR}${LIBDIR}/gnats
	install -o ${BINOWN} -g ${BINGRP} -m 444 categories \
		${DESTDIR}${LIBDIR}/gnats/${GNATS_HOST}

.include <bsd.prog.mk>
