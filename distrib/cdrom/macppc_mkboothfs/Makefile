#	$NetBSD: Makefile,v 1.3 2009/03/29 04:52:12 tsutsui Exp $

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
	echo '$$NetBSD: Makefile,v 1.3 2009/03/29 04:52:12 tsutsui Exp $$' > ${.TARGET}.tmp
	${UUENCODE} boothfs boothfs >> ${.TARGET}.tmp
	cp ${.TARGET}.tmp ${NETBSDSRCDIR}/distrib/cdrom/${.TARGET}

CLEANFILES+= boothfs.uue.tmp

.include <bsd.hostprog.mk>
