#	$NetBSD: Makefile,v 1.4 2009/03/30 11:22:46 tsutsui Exp $

HOSTPROG=	macppc_mkboothfs
SRCS=		mkboothfs.c
NOMAN=		# defined

CLEANFILES+=	boothfs

.include <bsd.own.mk>

.ifndef NOCOMPATLIB
COMPATOBJ!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
.-include       "${COMPATOBJ}/defs.mk"
HOST_CPPFLAGS+=	-I${NETBSDSRCDIR}/tools/compat
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
