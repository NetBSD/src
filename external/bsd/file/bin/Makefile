#	$NetBSD: Makefile,v 1.1.14.2 2014/08/19 23:46:46 tls Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

.if ${MKSHARE} != "no"
FILESDIR=	${MFILESDIR}
FILES=		${MFILES}
.endif

PROG=		file
LDADD+=		-lmagic -lz
DPADD+=		${LIBMAGIC} ${LIBZ}
MAN=		file.1 magic.5

CLEANFILES+=	magic.mgc
.if ${MKSHARE} != "no"
realall:	file magic.mgc
.endif

LIBMAGIC!= cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${LIBMAGIC}

.if ${MKSHARE} != "no"
magic.mgc: ${TOOL_MKMAGIC}
	${_MKTARGET_CREATE}
	${TOOL_MKMAGIC} -C -m ${DIST}/magic/magdir
	@mv magdir.mgc ${.TARGET}

.else
magic.mgc:
.endif

.include <bsd.prog.mk>
