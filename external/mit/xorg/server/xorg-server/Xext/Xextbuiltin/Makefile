#	$NetBSD: Makefile,v 1.4.8.1 2009/09/17 04:24:37 snj Exp $

LIBISPRIVATE=	yes
LIB=		Xextbuiltin

.include "../../Makefile.serverlib"
.include "../Makefile.Xextbuiltin"

SRCS=	${SRCS.Xextbuiltin}

COPTS.panoramiX.c+=		-Wno-error
COPTS.panoramiXprocs.c+=	-Wno-error
COPTS.security.c+=		-Wno-error
COPTS.shape.c+=			-Wno-error
COPTS.shm.c+=			-Wno-error
COPTS.sync.c+=			-Wno-error
COPTS.xf86bigfont.c+=		-Wno-error

.include "../Makefile.Xext"
