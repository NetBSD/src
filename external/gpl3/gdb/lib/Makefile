#	$NetBSD: Makefile,v 1.1 2011/09/26 03:02:44 christos Exp $

.include "../Makefile.inc"

#SUBDIR+= libsim .WAIT

SUBDIR=		libbfd libiberty libopcodes libgdb libdecnumber
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

.include <bsd.subdir.mk>
