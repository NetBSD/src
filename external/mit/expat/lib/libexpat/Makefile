#	$NetBSD: Makefile,v 1.1.2.2.6.1 2016/05/18 10:45:14 snj Exp $

.include <bsd.init.mk>

LIB=		expat

.PATH:	${EXPATSRCDIR}/lib
SRCS=	xmlparse.c xmlrole.c xmltok.c

INCS=		expat.h expat_external.h
INCSDIR=	/usr/include

CPPFLAGS+=		-I${EXPATSRCDIR} -I${.CURDIR}
CPPFLAGS+=		-DHAVE_EXPAT_CONFIG_H
COPTS.xmlparse.c=	-Wno-error # macppc
COPTS.xmlrole.c=	-Wno-error # macppc
COPTS.xmltok.c=		-Wno-error # macppc

FILES+=			expat.pc
FILESOWN_expat.pc=	${BINOWN}
FILESGRP_expat.pc=	${BINGRP}
FILESMODE_expat.pc=	${NONBINMODE}
FILESDIR_expat.pc=	/usr/lib/pkgconfig

CLEANFILES+=expat.pc

.include <bsd.lib.mk>

gaga:
	@echo MKLINT=${MKLINT}
	@echo LSRCS=${LSRCS}
	@echo LOBJS=${LOBJS}

all: expat.pc
dependall: all

expat.pc: expat_config.h expat.pc.in
	@(V=$$( (echo '#include <expat_config.h>'; echo PACKAGE_VERSION) | \
	    ${HOST_CC} -E -I${.CURDIR} - | tail -1 | tr -d '"') && \
	    ${TOOL_SED} -e s/@VERSION@/$$V/ < ${.CURDIR}/expat.pc.in \
	    > ${.TARGET})

