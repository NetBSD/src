#	from: @(#)Makefile	8.1 (Berkeley) 6/28/93
#	$Id: Makefile,v 1.5 1994/06/13 20:50:39 mycroft Exp $

.include "../config/Makefile.config"

PROG=	mk-amd-map
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-I${.CURDIR}/../rpcx
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-DOS_HDR=\"os-${OS}.h\"
MAN8=	mk-amd-map.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
