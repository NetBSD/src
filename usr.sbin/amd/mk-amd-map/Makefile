#	from: @(#)Makefile	8.1 (Berkeley) 6/28/93
#	$Id: Makefile,v 1.6 1994/12/22 11:32:28 cgd Exp $

.include "../config/Makefile.config"

PROG=	mk-amd-map
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-I${.CURDIR}/../rpcx
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-DOS_HDR=\"os-${OS}.h\"
MAN=	mk-amd-map.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
