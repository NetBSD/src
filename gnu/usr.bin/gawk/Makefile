#	$NetBSD: Makefile,v 1.7 1997/10/25 10:10:49 lukem Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.c regex.c dfa.c \
		getopt.c getopt1.c
CPPFLAGS+=	-DGAWK
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} ${LIBGNUMALLOC}
CLEANFILES+=	awk.c y.tab.h

.include <bsd.prog.mk>
