#	$NetBSD: Makefile,v 1.10.8.4 2002/04/17 00:04:37 nathanw Exp $

S=	${.CURDIR}/../../../..

PROG=	xxboot
LINKS=	${BINDIR}/xxboot ${BINDIR}/raboot 
LINKS+=	${BINDIR}/xxboot ${BINDIR}/rdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/sdboot
LINKS+=	${BINDIR}/xxboot ${BINDIR}/hpboot
WARNS?=	1

SRCS=	start.S bootxx.c romread.S str.S

STRIPFLAG=
CPPFLAGS+=-D_STANDALONE -DLIBSA_NO_FD_CHECKING -DLIBSA_NO_RAW_ACCESS \
	-DLIBSA_NO_TWIDDLE -DLIBSA_SINGLE_DEVICE=rom \
	-DLIBSA_NO_COMPAT_UFS \
	-DLIBSA_NO_FS_SYMLINK -DLIBSA_NO_FS_CLOSE \
	-DLIBSA_NO_FS_WRITE -DLIBSA_NO_FS_SEEK \
	-DNEED_UFS -DNEED_CD9660
BINDIR=	/usr/mdec
NOMAN=	# defined

CFLAGS=	-Os

SA_AS=	library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

.if ${MACHINE} == "vax"
.PHONY: machine-links
beforedepend: machine-links
machine-links:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h ${MACHINE_ARCH} ] || ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
.NOPATH: machine ${MACHINE_ARCH}
CLEANFILES+= machine ${MACHINE_ARCH}
.endif

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -N -Ttext 100000 -o ${PROG}.out ${OBJS} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.out
	${OBJCOPY} -O binary ${PROG}.out ${PROG}

.include <bsd.prog.mk>
