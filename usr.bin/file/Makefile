#	$NetBSD: Makefile,v 1.47 2003/09/17 00:41:17 itojun Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/file
.PATH:		${DIST}/src ${DIST}/doc

WFORMAT=	1
MFILESDIR=	/usr/share/misc
MFILES=		magic magic.mgc ${DIST}/magic/magic.mime magic.mime.mgc
MAGIC=		${MFILESDIR}/magic
TOOL_MKMAGIC?=	${.OBJDIR}/file

.if ${MKSHARE} != "no"
FILESDIR=	${MFILESDIR}
FILES=		${MFILES}
.endif

PROG=		file
SRCS=		file.c
CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR}/../../lib/libmagic -I${DIST}
LDADD+=		-lmagic -lz
DPADD+=		${LIBMAGIC} ${LIBZ}
MAN=		file.1 magic.5

CLEANFILES+=	magic magic.mgc magic.mime magic.mime.mgc
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
	${TOOL_CAT} ${MAGFILES} > ${.TARGET}

magic.mgc: magic
	 ${TOOL_MKMAGIC} -C -m magic

magic.mime.mgc:
	cp ${DIST}/magic/magic.mime ${.OBJDIR}
	${TOOL_MKMAGIC} -C -m magic.mime
.else
magic:
.endif

.include <bsd.prog.mk>
