#	$NetBSD: Makefile.inc,v 1.10.142.3 2016/10/05 20:55:35 skrll Exp $

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
