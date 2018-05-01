#	$NetBSD: Makefile,v 1.6 2018/05/01 19:59:43 christos Exp $

HOSTPROG=	macppc_mkboothfs
SRCS=		mkboothfs.c

CLEANFILES+=	boothfs

.include <bsd.hostinit.mk>

.ifndef NOCOMPATLIB
.-include	"${TOOLDIR}/share/compat/defs.mk"
.endif


UUENCODE?=	uuencode	# XXX no ${TOOL_UUENCODE}

boothfs: ${HOSTPROG}
	./${HOSTPROG} boothfs

boothfs.uue: boothfs
	echo \$$NetBSD\$$ > ${.TARGET}.tmp
	${UUENCODE} boothfs boothfs >> ${.TARGET}.tmp
	cp ${.TARGET}.tmp ${NETBSDSRCDIR}/distrib/cdrom/${.TARGET}

CLEANFILES+= boothfs.uue.tmp

.include <bsd.hostprog.mk>
