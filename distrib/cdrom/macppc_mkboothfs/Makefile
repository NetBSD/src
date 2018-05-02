#	$NetBSD: Makefile,v 1.5.30.1 2018/05/02 07:19:59 pgoyette Exp $

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
