#	$Id: Makefile,v 1.1 1995/07/24 04:02:55 jtc Exp $

CFLAGS += -DUSER_CACHE -DWTMP -DUSE_YP

PROG=	rpc.pcnfsd

SRCS=	pcnfsd_v1.c pcnfsd_v2.c pcnfsd_misc.c \
	pcnfsd_cache.c pcnfsd_print.c

MAN=	pcnfsd.8
MLINKS= pcnfsd.8 rpc.pcnfsd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc -lcrypt

OBJS= pcnfsd_svc.o pcnfsd_xdr.o

# Special rules for the generated C code...

pcnfsd_svc.o : pcnfsd_svc.c pcnfsd.h
	$(CC) $(CFLAGS) -c $*.c

pcnfsd_xdr.o : pcnfsd_xdr.c pcnfsd.h
	$(CC) $(CFLAGS) -c $*.c

pcnfsd_svc.c: pcnfsd.x
	rpcgen -s udp -s tcp -I pcnfsd.x -o $@

pcnfsd_xdr.c: pcnfsd.x
	rpcgen -c pcnfsd.x -o $@

pcnfsd.h: pcnfsd.x
	rpcgen -h pcnfsd.x -o $@

CLEANFILES += pcnfsd_svc.c pcnfsd_xdr.c pcnfsd.h *.o
# CLEANFILES +=	 clnt.pcnfsd pcnfsd_clnt.c

.include <bsd.prog.mk>
