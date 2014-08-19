#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $

NOMAN=		# defined

PROG=		mkboot
SRCS=		mkboot.c

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/external/gpl3/binutils/dist

CPPFLAGS=	-I${BFDTOP}/include
CPPFLAGS+=	-DATTRIBUTE_PACKED=

.include <bsd.prog.mk>
