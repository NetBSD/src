#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.8 1994/06/13 20:48:16 mycroft Exp $

.include "../config/Makefile.config"

PROG = amq
SRCS = amq.c amq_clnt.c amq_xdr.c misc_rpc.c
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-I${.CURDIR}/../rpcx
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-DARCH_REP=\"${MACHINE}\"
CFLAGS+=-DOS_REP=\"${OS}\"
CFLAGS+=-DOS_HDR=\"os-${OS}.h\"
.PATH: ${.CURDIR}/../rpcx ${.CURDIR}/../amd

.include "../../Makefile.inc"
.include <bsd.prog.mk>
