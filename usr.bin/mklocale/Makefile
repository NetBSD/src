#	@(#)Makefile	8.1 (Berkeley) 6/7/93
# $FreeBSD: src/usr.bin/mklocale/Makefile,v 1.19.2.1 1999/08/29 15:31:03 peter Exp $

PROG=	mklocale
SRCS=	yacc.y lex.l #y.tab.h
CFLAGS+=-I${.OBJDIR}	# for y.tab.h
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../include -I${.CURDIR}/../../lib/libc/locale
MAN1=	mklocale.1
CLEANFILES+= y.tab.c y.tab.h
YACC= yacc -d

LIBXPG4OBJDIR=cd ${.CURDIR}/../../lib/libxpg4; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
.if exists(${DESTDIR}/usr/lib/libxpg4.a)
XPG4LIB=-lxpg4
.elif exists(${LIBXPG4OBJDIR}/libxpg4.a)
XPG4LIB=-L${LIBXPG4OBJDIR} -Wl,-rpath,${LIBXPG4OBJDIR} -lxpg4 # XXX 
.endif

LDADD+=${XPG4LIB}

.include <bsd.prog.mk>
