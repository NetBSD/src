#	$Id: Makefile,v 1.3 1994/02/09 07:51:44 cgd Exp $

SUBDIR+= kern
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif

regress: _SUBDIRUSE

install:

.include <bsd.subdir.mk>
