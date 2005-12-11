#	$NetBSD: Makefile,v 1.4 2005/12/11 12:17:25 christos Exp $

NOMAN=		# defined

PROG=		mkboot
SRCS=		mkboot.c

# We don't use libbfd, but we do use the BFD ELF header files because
# they are somewhat more portable than NetBSD's own elf.h.
BFDTOP=	${NETBSDSRCDIR}/gnu/dist/binutils

CPPFLAGS=	-I${BFDTOP}/include

.include <bsd.prog.mk>
