#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.3 1997/06/23 13:25:45 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "mips") 
PROG=           gcc
SRCS=	gcc.c obstack.c version.c 
BINDIR=		/usr/bin
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/$(MACHINE_ARCH) \
	-DMD_EXEC_PREFIX=\"/usr/libexec/\" \
	-DMD_STARTFILE_PREFIX=\"/usr/lib/\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

.PATH:		$(.CURDIR)/../common 
.endif

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include <bsd.prog.mk>
