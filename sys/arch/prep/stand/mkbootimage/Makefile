#	$NetBSD: Makefile,v 1.4.2.3 2004/09/21 13:21:07 skrll Exp $

PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/gnu/dist/binutils

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR} -I${BFDTOP}/include

.include <bsd.prog.mk>
