#	$NetBSD: Makefile,v 1.7 1999/02/09 17:50:35 tv Exp $
#

.include <bsd.own.mk>

# Override to make a cross assembler.
TARGET_ARCH ?=	${MACHINE_ARCH}

PROG=		as
MAN=		as.1
SRCS=		app.c as.c atof-generic.c bignum-copy.c cond.c depend.c \
		ecoff.c ehopt.c expr.c flonum-konst.c flonum-copy.c \
		flonum-mult.c frags.c hash.c input-file.c input-scrub.c \
		literal.c messages.c output-file.c read.c subsegs.c symbols.c \
		write.c listing.c stabs.c sb.c macro.c
SRCS+=		${TARG_CPU_C} ${OBJ_FORMAT_C} ${ATOF_TARG_C}

CPPFLAGS+=	-I${.CURDIR}/arch/${TARGET_ARCH} \
		-I${DIST}/gas -I${DIST}/gas/config -I${BFDOBJ} \
		-I${DIST}/bfd -I${DIST}/include -I${DIST} \
		-D_GNU_SOURCE
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${MAKE} print-objdir

.PATH:		${DIST}/gas/config ${DIST}/gas ${DIST}/gas/doc

.include "${.CURDIR}/arch/${TARGET_ARCH}/Makefile.inc"
.include <bsd.prog.mk>
