#	$NetBSD: Makefile,v 1.20 2003/12/11 17:34:10 matt Exp $

S=	${.CURDIR}/../../../..

.include <bsd.own.mk>

PROG=	xxboot
LINKS=	${BINDIR}/xxboot ${BINDIR}/raboot 
LINKS+=	${BINDIR}/xxboot ${BINDIR}/rdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/sdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/hpboot

SRCS=	start.S bootxx.c romread.S str.S

STRIPFLAG=
CPPFLAGS+=-D_STANDALONE -DLIBSA_NO_FD_CHECKING -DLIBSA_NO_RAW_ACCESS \
	-DLIBSA_NO_TWIDDLE -DLIBSA_SINGLE_DEVICE=rom \
	-DLIBSA_NO_COMPAT_UFS \
	-DLIBSA_NO_FS_SYMLINK -DLIBSA_NO_FS_CLOSE \
	-DLIBSA_NO_FS_WRITE -DLIBSA_NO_FS_SEEK \
	-DNEED_UFS -DNEED_CD9660
# Use small daddr_t to avoid code bloat
CPPFLAGS+=-D__daddr_t=int32_t
BINDIR=	/usr/mdec

CFLAGS=	-Os
LIBC=

SA_AS=	library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

CLEANFILES+= ${PROG}.out

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -N -Ttext 100000 -o ${PROG}.out ${OBJS} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.out
	${OBJCOPY} -O binary ${PROG}.out ${PROG}

.include <bsd.prog.mk>
