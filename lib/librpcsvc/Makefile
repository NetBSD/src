#       @(#)Makefile    5.10 (Berkeley) 6/24/90

.PATH: ${.CURDIR}/../../include/rpcsvc /usr/src/include/rpcsvc
LIB=    rpcsvc
RPCSRCS= rstat.x rnusers.x rwall.x
CFLAGS+= -I/usr/include/rpcsvc

RPCCOM = rpcgen

SRCS= ${RPCSRCS:R:S/$/_xdr.c/g}

NOMAN= noman

CLEANFILES+= ${RPCSRCS:R:S/$/_xdr.c/g}

.include <bsd.lib.mk>

.SUFFIXES: .x _xdr.c

.x_xdr.c:
	@echo generating $@...
	@PWD=`pwd` ; cd ${.CURDIR} ; if cmp -s ${.IMPSRC} ${*F}.x > /dev/null; then :; else cp -f ${.IMPSRC} .; fi
	@PWD=`pwd` ; cd ${.CURDIR} ; ${RPCCOM} -c ${*F}.x -o $$PWD/${.TARGET}

OBJS+= ${RPCSRCS:R:S/$/_xdr.o/g}
