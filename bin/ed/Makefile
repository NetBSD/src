PROG   =ed
CFLAGS+=-DDES -DGNU_REGEX
SRCS=	ed.c re.c buf.c cbc.c
LDADD= -lgnuregex -lcrypt

.include <bsd.prog.mk>
