#	$NetBSD: Makefile,v 1.5 2000/05/20 13:21:29 ragge Exp $

S=	${.CURDIR}/../../../../

PROG=	xxboot
LINKS=	${BINDIR}/xxboot ${BINDIR}/raboot 
LINKS+=	${BINDIR}/xxboot ${BINDIR}/rdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/sdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/hpboot
WARNS?=	1

SRCS=	start.s bootxx.c romread.s urem.s udiv.s str.s

STRIPFLAG=
CPPFLAGS+=-D_STANDALONE -DLIBSA_NO_FD_CHECKING -DLIBSA_NO_RAW_ACCESS \
	-DLIBSA_NO_TWIDDLE -DLIBSA_SINGLE_DEVICE=rom \
	-DLIBSA_NO_COMPAT_UFS \
	-DLIBSA_NO_FS_SYMLINK -DLIBSA_NO_FS_CLOSE \
	-DLIBSA_NO_FS_WRITE -DLIBSA_NO_FS_SEEK
BINDIR=	/usr/mdec
NOMAN=	1

CFLAGS=	-Os

SA_AS=	library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	ld -N -Ttext 100000 -o ${PROG}.out ${OBJS} ${LIBSA} ${LIBKERN}
	objcopy -O binary ${PROG}.out ${PROG}

.include <bsd.prog.mk>
