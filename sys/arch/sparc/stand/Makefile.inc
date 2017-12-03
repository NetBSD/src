#	$NetBSD: Makefile.inc,v 1.10.122.1 2017/12/03 11:36:44 jdolecek Exp $

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
