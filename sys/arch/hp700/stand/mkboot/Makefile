#	$NetBSD: Makefile,v 1.5 2009/11/07 17:56:14 skrll Exp $

NOMAN=		# defined

PROG=		mkboot
SRCS=		mkboot.c

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/external/gpl3/binutils/dist

CPPFLAGS=	-I${BFDTOP}/include
CPPFLAGS+=	-DATTRIBUTE_PACKED=

.include <bsd.prog.mk>
