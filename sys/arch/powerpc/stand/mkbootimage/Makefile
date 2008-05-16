#	$NetBSD: Makefile,v 1.1.18.1 2008/05/16 02:23:02 yamt Exp $

BINDIR=	/usr/mdec
PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/gnu/dist/binutils

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR} -I${BFDTOP}/include
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
