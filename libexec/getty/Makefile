#	$NetBSD: Makefile,v 1.15 2006/11/16 00:49:20 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
LINKS=	${BINDIR}/getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL} ${LIBTERMCAP}
LDADD+=	-lutil -ltermcap
MAN=	getty.8 gettytab.5 ttys.5
MLINKS+=getty.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
.for f in init subr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
