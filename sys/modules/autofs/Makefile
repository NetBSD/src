#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/autofs

KMOD=	autofs
IOCONF= autofs.ioconf
SRCS=	autofs.c autofs_vfsops.c autofs_vnops.c

CPPFLAGS+=	-DDIAGNOSTIC
DBG+=		-DDEBUG -DLOCKDEBUG

WARNS=	3

.include <bsd.kmodule.mk>
