#	$NetBSD: Makefile,v 1.5 2001/01/09 08:52:58 enami Exp $

DIST=		${.CURDIR}/../../dist

PROG=		gasp
SRCS=		gasp.c macro.c sb.c hash.c
MKMAN=		no

.PATH:		${DIST}/gas

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb")
ARCHSUBDIR= mips
.else
ARCHSUBDIR= ${MACHINE_ARCH}
.endif

CPPFLAGS+=	-I${.CURDIR}/../gas.new/arch/${ARCHSUBDIR} \
		-I${.CURDIR}/../gas.new -I${DIST}/gas/config -I${DIST}/gas \
		-I${DIST}/include -D_GNU_SOURCE \
		-DTARGET_ALIAS=\"${MACHINE_GNU_ARCH}-netbsd\" \
		-DTARGET_CPU=\"${MACHINE_GNU_ARCH}\" \
		-DTARGET_CANONICAL=\"${MACHINE_GNU_ARCH}--netbsd\"

BFDOBJ!=	cd ${.CURDIR}/../../lib/bfd && ${PRINTOBJDIR}

LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd.a

.include <bsd.prog.mk>
