#	%W% (Berkeley) %G%

PROG = amq
SRCS = amq.c amq_clnt.c misc_rpc.c
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-DARCH_REP=\"${MACHINE}\"
CFLAGS+=-DOS_REP=\"${OS}\"
CFLAGS+=-DOS_HDR=\"os-${OS}.h\"
# arguably XXX -- for broken (?) rpcgen #include lines
CFLAGS+=-I/usr/include/rpcsvc
DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc
.PATH: ${.CURDIR}/../amd

amq_clnt.c: /usr/include/rpcsvc/amq.x
	rpcgen -l -o ${.TARGET} ${.ALLSRC}
CLEANFILES+=amq_clnt.c

.include "../../Makefile.inc"
.include <bsd.prog.mk>
