#	$NetBSD: Makefile,v 1.1.10.1 1999/11/15 00:39:47 fvdl Exp $

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
	ld -N -Ttext 100000 -o a.out ${OBJS} ${LIBSA}
	strip a.out
	size a.out
	dd if=a.out of=${PROG} bs=32 skip=1
	rm -f a.out

.include <bsd.prog.mk>
