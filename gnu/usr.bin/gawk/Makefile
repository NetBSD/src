#	$Id: Makefile,v 1.3 1993/10/07 07:24:13 cgd Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.c regex.c dfa.c \
		getopt.c getopt1.c
CFLAGS+=	-DGAWK
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} /usr/lib/libgnumalloc.a
CLEANFILES+=	awk.c y.tab.h

MAN1=		awk.0

.include <bsd.prog.mk>
