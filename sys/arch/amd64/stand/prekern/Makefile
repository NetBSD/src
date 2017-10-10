#	$NetBSD: Makefile,v 1.1 2017/10/10 09:29:14 maxv Exp $

PROG=		prekern
SRCS=	locore.S trap.S prekern.c mm.c console.c elf.c

NOSSP=		# defined
NOPIE=		# defined
NOMAN=		1

S=	${.CURDIR}/../../../..

.PATH: ${.CURDIR}

BINDIR=		/usr/mdec
BINMODE=	444

.include <bsd.own.mk>

CPPFLAGS+=	-I. -I${S}

.include <bsd.klinks.mk>

CPPFLAGS+=	-DKERNEL -D__x86_64__
CFLAGS+=	-Wall -Werror -mno-red-zone -mno-mmx -mno-sse -mno-avx -ffreestanding
STRIPFLAG=
LINKFLAGS=	-X -z max-page-size=0x100000 -Ttext 0x100000 -T prekern.ldscript

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBC=		# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing

${PROG}: ${OBJS}
	${LD} ${LINKFLAGS} -o ${.TARGET} ${OBJS}

all:	${PROG}

.include <bsd.prog.mk>

