#	$NetBSD: Makefile,v 1.3 1998/05/22 17:19:12 tv Exp $
#

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
	-I${.CURDIR}/../../lib/bfd -I${DIST}/gas -I${DIST}/gas/config \
	-I${DIST}/bfd -I${DIST}/include -I${DIST} \
	-D_GNU_SOURCE

.include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"

SRCS+=	${TARG_CPU_C} ${OBJ_FORMAT_C} ${ATOF_TARG_C}
TARGET_CPU= ${GNU_ARCH.${TARGET_ARCH}}
CPPFLAGS+= -DTARGET_ALIAS=\"${TARGET_CPU}-netbsd\"
CPPFLAGS+= -DTARGET_CPU=\"${TARGET_CPU}\"
CPPFLAGS+= -DTARGET_CANONICAL=\"${TARGET_CPU}-unknown-netbsd\"

LDADD+= -lbfd
DPADD+= ${LIBBFD}

MAN=	as.1

.include <bsd.prog.mk>
