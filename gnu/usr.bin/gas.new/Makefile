#	$NetBSD: Makefile,v 1.11 2000/05/11 00:56:48 deberg Exp $
#

.include <bsd.own.mk>

# Override to make a cross assembler.
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
TARGET_ARCH ?=	mips
.else
TARGET_ARCH ?=	${MACHINE_ARCH}
.endif

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

TEXINFO=	as.texi
INFOFLAGS=	-I${DIST}/gas/doc

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${MAKE} print-objdir

.PATH:		${DIST}/gas/config ${DIST}/gas ${DIST}/gas/doc

.include "${.CURDIR}/arch/${TARGET_ARCH}/Makefile.inc"
.include <bsd.info.mk>
.include <bsd.prog.mk>
