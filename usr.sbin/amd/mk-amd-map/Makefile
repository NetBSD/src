#	@(#)Makefile	8.1 (Berkeley) 6/28/93

.include "../config/Makefile.config"

PROG=	mk-amd-map
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-I${.CURDIR}/../rpcx
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-DOS_HDR=\"os-${OS}.h\"
MAN8=	mk-amd-map.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
