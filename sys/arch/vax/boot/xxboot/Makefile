#	$NetBSD: Makefile,v 1.3 2000/04/16 01:44:36 matt Exp $

S=	${.CURDIR}/../../../../

PROG=	xxboot
LINKS=	${BINDIR}/xxboot ${BINDIR}/raboot 
LINKS+=	${BINDIR}/xxboot ${BINDIR}/rdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/sdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/hpboot

SRCS=	start.s bootxx.c romread.s urem.s udiv.s str.s

STRIPFLAG=
CPPFLAGS+=-D_STANDALONE -DLIBSA_NO_FD_CHECKING -DLIBSA_NO_RAW_ACCESS \
	-DLIBSA_NO_TWIDDLE -DLIBSA_SINGLE_DEVICE=rom \
	-DLIBSA_SINGLE_FILESYSTEM=ufs
BINDIR=	/usr/mdec
NOMAN=	1

SA_AS=	library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

${PROG}: ${OBJS} ${LIBSA}
	ld -N -Ttext 100000 -o ${PROG}.out ${OBJS} ${LIBSA}
	objcopy -O binary ${PROG}.out ${PROG}

.include <bsd.prog.mk>
