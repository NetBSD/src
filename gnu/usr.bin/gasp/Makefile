#	$NetBSD: Makefile,v 1.2 1998/12/29 15:13:03 tv Exp $

DIST=		${.CURDIR}/../../dist

PROG=		gasp
SRCS=		gasp.c macro.c sb.c hash.c
NOMAN=

.PATH:		${DIST}/gas

CPPFLAGS+=	-I${.CURDIR}/../gas.new/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../gas.new -I${DIST}/gas/config -I${DIST}/gas \
		-I${DIST}/include -D_GNU_SOURCE \
		-DTARGET_ALIAS=\"${MACHINE_GNU_ARCH}-netbsd\" \
		-DTARGET_CPU=\"${MACHINE_GNU_ARCH}\" \
		-DTARGET_CANONICAL=\"${MACHINE_GNU_ARCH}--netbsd\"

BFDOBJ!=	cd ${.CURDIR}/../../lib/bfd; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd.a

.include <bsd.prog.mk>
