#	$NetBSD: Makefile,v 1.6 1999/02/02 22:02:47 tv Exp $
#

.include <bsd.own.mk>
DIST=	${.CURDIR}/../../dist
TARGET_ARCH ?= ${MACHINE_ARCH}

PROG=	as
SRCS=	app.c as.c atof-generic.c bignum-copy.c cond.c depend.c ecoff.c \
	ehopt.c expr.c flonum-konst.c flonum-copy.c flonum-mult.c frags.c \
	hash.c input-file.c input-scrub.c literal.c messages.c output-file.c \
	read.c subsegs.c symbols.c write.c listing.c stabs.c \
	sb.c macro.c

.PATH:	${DIST}/gas/config ${DIST}/gas ${DIST}/gas/doc

CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/arch/${TARGET_ARCH} \
	-I${BFDOBJ} -I${DIST}/gas -I${DIST}/gas/config \
	-I${DIST}/bfd -I${DIST}/include -I${DIST} \
	-D_GNU_SOURCE

BFDOBJ!=cd ${.CURDIR}/../../lib/bfd; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

.include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"

SRCS+=	${TARG_CPU_C} ${OBJ_FORMAT_C} ${ATOF_TARG_C}

LDADD+= -L${BFDOBJ} -lbfd
DPADD+= ${BFDOBJ}/libbfd.a

MAN=	as.1

.include <bsd.prog.mk>
