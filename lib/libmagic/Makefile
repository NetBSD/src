#	$NetBSD: Makefile,v 1.3 2003/05/08 07:14:14 uebayasi Exp $
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
MLINKS+=	libmagic.3 magic_open.3 \
		libmagic.3 magic_close.3 \
		libmagic.3 magic_error.3 \
		libmagic.3 magic_file.3 \
		libmagic.3 magic_buffer.3 \
		libmagic.3 magic_setflags.3 \
		libmagic.3 magic_check.3 \
		libmagic.3 magic_compile.3 \
		libmagic.3 magic_load.3
INCS=		magic.h
INCSDIR=	/usr/include

MFILESDIR?=      /usr/share/misc
MAGIC?=          ${MFILESDIR}/magic

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR} -I${DIST}/src

.include <bsd.lib.mk>
