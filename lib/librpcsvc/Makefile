#       @(#)Makefile    5.10 (Berkeley) 6/24/90

.PATH: ${.CURDIR}/../../include/rpcsvc /usr/src/include/rpcsvc

LIB=    rpcsvc

RPCSRCS= rstat.x rnusers.x rwall.x

RPCCOM = rpcgen

CFLAGS+= -I/usr/include/rpcsvc

SRCS= ${RPCSRCS:R:S/$/_xdr.c/g}

CLEANFILES+= ${SRCS} ${RPCSRCS}

NOMAN= noman

.include <bsd.lib.mk>

.SUFFIXES: .x _xdr.c

.x_xdr.c:
	@echo generating $@...
	@PWD=`pwd` ; cd ${.CURDIR} ; if cmp -s ${.IMPSRC} ${*F}.x > /dev/null; then :; else cp -f ${.IMPSRC} $$PWD/${*F}.x ; fi
	@${RPCCOM} -c ${*F}.x -o ${.TARGET}

OBJS+= ${RPCSRCS:R:S/$/_xdr.o/g}
