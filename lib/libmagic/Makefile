#	$NetBSD: Makefile,v 1.1 2003/03/25 22:43:09 pooka Exp $
#

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/file
.PATH:		${DIST}/src ${DIST}/doc

LIB=		magic

SRCS=		magic.c apprentice.c softmagic.c ascmagic.c compress.c	\
		is_tar.c readelf.c print.c fsmagic.c apptype.c funcs.c
MAN=		libmagic.3
INCS=		magic.h
INCSDIR=	/usr/include

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR} -I${DIST}/src

.include <bsd.lib.mk>
