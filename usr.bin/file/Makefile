#	$NetBSD: Makefile,v 1.54.12.1 2008/09/18 04:29:11 wrstuden Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

USE_FORT?= yes	# data-driven bugs?

DIST=		${NETBSDSRCDIR}/dist/file
.PATH:		${DIST}/src ${DIST}/doc

MFILESDIR=	/usr/share/misc
MFILES=		magic magic.mgc 
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

CLEANFILES+=	magic magic.mgc
.if ${MKSHARE} != "no"
realall:	file magic magic.mgc
.endif

MAGDIRF:sh=	LANG=C ${HOST_SH} -c 'echo ${DIST}/magic/magdir/[0-9a-z]*'
MAGFILES=	${DIST}/magic/Header \
		${DIST}/magic/Localstuff \
		${DIST}/magic/magdir/netbsd \
		${MAGDIRF}

.if ${MKSHARE} != "no"
magic: ${MAGFILES}
	${_MKTARGET_CREATE}
	${TOOL_CAT} ${MAGFILES} > ${.TARGET}

magic.mgc: magic
	${_MKTARGET_CREATE}
	${TOOL_MKMAGIC} -C -m magic

.else
magic:
.endif

.include <bsd.prog.mk>
