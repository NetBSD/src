#	$NetBSD: Makefile,v 1.2 2003/03/26 15:18:38 christos Exp $
#

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/file
.PATH:		${DIST}/src ${DIST}/doc

LIB=		magic

.if ${MACHINE_ARCH} == "i386"
# Set lint to exit on warnings
# XXX: Soon
#LINTFLAGS+=	-w
.endif
# ignore 'empty translation unit' warnings.
LINTFLAGS+=	-X 272

SRCS=		magic.c apprentice.c softmagic.c ascmagic.c compress.c	\
		is_tar.c readelf.c print.c fsmagic.c apptype.c funcs.c
MAN=		libmagic.3
INCS=		magic.h
INCSDIR=	/usr/include

MFILESDIR?=      /usr/share/misc
MAGIC?=          ${MFILESDIR}/magic

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR} -I${DIST}/src

.include <bsd.lib.mk>
