#	$NetBSD: Makefile,v 1.9 1998/04/09 00:32:33 tv Exp $

PROG=		awk
SRCS=		main.c eval.c builtin.c msg.c iop.c io.c field.c array.c \
		node.c version.c re.c awk.y regex.c dfa.c \
		getopt.c getopt1.c
CPPFLAGS+=	-DGAWK -DSTDC_HEADERS
LDADD=		-lm -lgnumalloc
DPADD=		${LIBM} ${LIBGNUMALLOC}

.include <bsd.prog.mk>
