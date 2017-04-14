TOP?=	../..
include ${TOP}/iconfig.mk

PROG=		run-test
SRCS=		run-test.c
SRCS+=		test_hmac_md5.c ${TOP}/src/crypt/hmac_md5.c

CFLAGS?=	-O2
CSTD?=		c99
CFLAGS+=	-std=${CSTD}

CPPFLAGS+=	-I${TOP} -I${TOP}/src

T_COMPAT_SRCS=	${COMPAT_SRCS:compat/%=${TOP}/compat/%}
T_MD5_SRC=	${MD5_SRC:compat/%=${TOP}/compat/%}
OBJS+=		${SRCS:.c=.o} ${T_COMPAT_SRCS:.c=.o} ${T_MD5_SRC:.c=.o}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

all: ${PROG}

clean:
	rm -f ${OBJS} ${PROG} ${PROG}.core ${CLEANFILES}

distclean: clean
	rm -f .depend

.depend: ${SRCS} ${T_COMPAT_SRCS} ${T_MD5_SRC}
	${CC} ${CPPFLAGS} -MM ${SRCS} ${T_COMPAT_SRCS} ${T_MD5_SRC} > .depend

depend: .depend

${PROG}: ${DEPEND} ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

test: ${PROG}
	./${PROG}
