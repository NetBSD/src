#	$NetBSD: Makefile,v 1.4.12.1 2013/02/25 00:24:27 tls Exp $

HOSTPROG=	macppc_mkboothfs
SRCS=		mkboothfs.c
NOMAN=		# defined

CLEANFILES+=	boothfs

.include <bsd.own.mk>

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
