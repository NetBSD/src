#	$NetBSD: Makefile,v 1.4.18.1 2013/08/28 23:59:20 rmind Exp $

BINDIR=	/usr/mdec
PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/external/gpl3/binutils/dist

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR} -I${BFDTOP}/include
CPPFLAGS+=	-DATTRIBUTE_PACKED=
.if !defined(HOSTPROGNAME)
DPADD+=	${LIBZ}
.endif
LDADD+=	-lz

.include <bsd.prog.mk>
