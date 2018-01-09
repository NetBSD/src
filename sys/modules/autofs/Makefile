#	$NetBSD: Makefile,v 1.2 2018/01/09 16:18:35 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/autofs

KMOD=	autofs
IOCONF= autofs.ioconf
SRCS=	autofs.c autofs_vfsops.c autofs_vnops.c

CPPFLAGS+=	-DDIAGNOSTIC
DBG+=		-DDEBUG -DLOCKDEBUG

.include <bsd.kmodule.mk>
