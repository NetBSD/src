#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.2 1997/03/24 22:06:17 christos Exp $

PROG=           gcc
SRCS=	gcc.c obstack.c version.c 
MAN=		gcc.1
BINDIR=		/usr/bin
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/$(MACHINE_ARCH) \
	-DMD_EXEC_PREFIX=\"/usr/libexec/\" \
	-DMD_STARTFILE_PREFIX=\"/usr/lib/\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 
MLINKS=		gcc.1 cc.1

.PATH:		$(.CURDIR)/../common 

.include <bsd.prog.mk>
