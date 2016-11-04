#	$NetBSD: Makefile.inc,v 1.13.2.1 2016/11/04 14:49:05 pgoyette Exp $

NOSSP=		# defined
NOPIE=		# defined
NOMAN=		# defined

BINDIR=		/usr/mdec
NOPIE=		yes

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-ffreestanding ${${ACTIVE_CC} == "gcc":? -msoft-float :}

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif
