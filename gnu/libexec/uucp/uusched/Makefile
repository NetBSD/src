# Makefile for uusched
# $Id: Makefile,v 1.4 1994/12/22 13:13:46 cgd Exp $

BINDIR=		$(bindir)

CLEANFILES=	uusched

all:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    uusched ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
