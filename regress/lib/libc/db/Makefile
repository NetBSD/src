#	@(#)Makefile	8.11 (Berkeley) 6/16/94

PROG=	dbtest
OBJS=	dbtest.o strerror.o

# Add -DSTATISTICS to CFLAGS to get btree statistical use info.
# Note, the db library has to be compiled for statistics as well.
INC=	-I${PORTDIR}/include -I${PORTDIR}
CFLAGS=	-D__DBINTERFACE_PRIVATE -DDEBUG -O ${INC}

dbtest: ${OBJS} ${PORTDIR}/libdb.a
	${CC} -o $@ ${OBJS} ${PORTDIR}/libdb.a

strerror.o: ${PORTDIR}/clib/strerror.c
	${CC} -c ${PORTDIR}/clib/strerror.c

clean:
	rm -f gmon.out ${OBJS} ${PROG} t1 t2 t3
