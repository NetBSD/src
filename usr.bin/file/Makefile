#	$NetBSD: Makefile,v 1.42 2003/04/05 11:33:16 pooka Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/file
.PATH:		${DIST}/src ${DIST}/doc

WFORMAT=	1
MFILESDIR=	/usr/share/misc
MFILES=		magic magic.mgc ${DIST}/magic/magic.mime magic.mime.mgc
MAGIC=		${MFILESDIR}/magic
MKMAGIC?=	${.OBJDIR}/file

.if ${MKSHARE} != "no"
FILESDIR=	${MFILESDIR}
FILES=		${MFILES}
.endif

PROG=		file
SRCS=		file.c
CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR}/../../lib/libmagic -I${DIST}
LDADD+=		-lz -lmagic
MAN=		file.1 magic.5

CLEANFILES+=	magic magic.mgc magic.mime.mgc
.if ${MKSHARE} != "no"
realall:	file magic magic.mgc magic.mime.mgc
.endif

MAGDIRF:sh=	echo ${DIST}/magic/magdir/[0-9a-z]*
MAGFILES=	${DIST}/magic/Header \
		${DIST}/magic/Localstuff \
		${DIST}/magic/magdir/netbsd \
		${MAGDIRF}

.if ${MKSHARE} != "no"
magic: ${MAGFILES}
	cat ${MAGFILES} > ${.TARGET}

magic.mgc: magic
	 ${MKMAGIC} -C -m magic

magic.mime.mgc:
	${MKMAGIC} -C -m ${DIST}/magic/magic.mime
.else
magic:
.endif

.include <bsd.prog.mk>
