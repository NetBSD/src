#       @(#)Makefile    5.10 (Berkeley) 6/24/90

.PATH: ${.CURDIR}/../../include/rpcsvc /usr/src/include/rpcsvc
LIB=    rpcsvc
RPCSRCS= rstat.x rnusers.x rwall.x

RPCCOM = rpcgen

SRCS= ${RPCSRCS:R:S/$/_xdr.c/g}

NOMAN= noman

CLEANFILES+= ${RPCSRCS:R:S/$/_xdr.c/g} ${RPCSRCS:R:S/$/.h/g}

.include <bsd.lib.mk>

.SUFFIXES: .x _xdr.c .h

.x_xdr.c:
	${RPCCOM} -c -o ${.TARGET} ${.IMPSRC}

.x.h:
	${RPCCOM} -h -o ${.TARGET} ${.IMPSRC}

OBJS+= ${RPCSRCS:R:S/$/_xdr.o/g}
