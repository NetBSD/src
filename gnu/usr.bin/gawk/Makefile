#	$NetBSD: Makefile,v 1.6 1997/03/24 22:06:14 christos Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.c regex.c dfa.c \
		getopt.c getopt1.c
CFLAGS+=	-DGAWK
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} ${LIBGNUMALLOC}
CLEANFILES+=	awk.c y.tab.h

.include <bsd.prog.mk>
