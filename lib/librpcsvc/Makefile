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
	@echo generating $@...
	@PWD=`pwd` ; cd ${.CURDIR} ; ${RPCCOM} -c ${.IMPSRC} -o $$PWD/${.TARGET}

.x.h:
	@echo generating $@...
	@PWD=`pwd` ; cd ${.CURDIR} ; ${RPCCOM} -h ${.IMPSRC} -o $$PWD/${.TARGET}

OBJS+= ${RPCSRCS:R:S/$/_xdr.o/g}
