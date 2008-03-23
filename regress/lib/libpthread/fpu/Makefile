PROG=fpu
NOMAN=# defined
COPTS=-O2
LDADD+=-lpthread -lm

.include <bsd.prog.mk>

regress: ${PROG}
	env PTHREAD_CONCURRENCY=1 ./${PROG}
