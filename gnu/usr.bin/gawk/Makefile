#	$NetBSD: Makefile,v 1.8 1998/02/04 23:04:11 thorpej Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.c regex.c dfa.c \
		getopt.c getopt1.c
CPPFLAGS+=	-DGAWK -DSTDC_HEADERS
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} ${LIBGNUMALLOC}
CLEANFILES+=	awk.c y.tab.h

.include <bsd.prog.mk>
