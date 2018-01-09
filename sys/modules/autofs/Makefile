#	$NetBSD: Makefile,v 1.1 2018/01/09 03:31:14 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/autofs

KMOD=	autofs
SRCS=	autofs.c autofs_vfsops.c autofs_vnops.c

CPPFLAGS+=	-DDIAGNOSTIC
DBG+=		-DDEBUG -DLOCKDEBUG

.include <bsd.kmodule.mk>
