#	$NetBSD: Makefile,v 1.5 1999/01/11 11:49:20 christos Exp $
#

.include <bsd.own.mk>
DIST=	${.CURDIR}/../../dist
TARGET_ARCH ?= ${MACHINE_ARCH}

PROG=	as
SRCS=	app.c as.c atof-generic.c bignum-copy.c cond.c expr.c \
	flonum-konst.c flonum-copy.c flonum-mult.c frags.c hash.c \
	input-file.c input-scrub.c literal.c messages.c output-file.c \
	read.c subsegs.c symbols.c write.c listing.c ecoff.c stabs.c \
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
TARGET_CPU= ${GNU_ARCH.${TARGET_ARCH}}
CPPFLAGS+= -DTARGET_ALIAS=\"${TARGET_CPU}-netbsd\"
CPPFLAGS+= -DTARGET_CPU=\"${TARGET_CPU}\"
CPPFLAGS+= -DTARGET_CANONICAL=\"${TARGET_CPU}--netbsd\"

LDADD+= -L${BFDOBJ} -lbfd
DPADD+= ${BFDOBJ}/libbfd.a

MAN=	as.1

.include <bsd.prog.mk>
