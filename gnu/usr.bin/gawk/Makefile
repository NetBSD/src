#	$NetBSD: Makefile,v 1.6.2.1 1997/11/08 06:53:06 lukem Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.c regex.c dfa.c \
		getopt.c getopt1.c
CPPFLAGS+=	-DGAWK
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} ${LIBGNUMALLOC}
CLEANFILES+=	awk.c y.tab.h

.include <bsd.prog.mk>
