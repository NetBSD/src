#	$NetBSD: Makefile,v 1.1.2.2 2012/03/02 18:27:54 riz Exp $

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

.include <bsd.lib.mk>

gaga:
	@echo MKLINT=${MKLINT}
	@echo LSRCS=${LSRCS}
	@echo LOBJS=${LOBJS}
