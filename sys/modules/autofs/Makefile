#	$NetBSD: Makefile,v 1.2.4.1 2019/06/10 22:09:11 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/autofs

KMOD=	autofs
IOCONF= autofs.ioconf
SRCS=	autofs.c autofs_vfsops.c autofs_vnops.c

CPPFLAGS+=	-DDIAGNOSTIC
DBG+=		-DDEBUG -DLOCKDEBUG

WARNS=	3

.include <bsd.kmodule.mk>
