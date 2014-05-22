# $NetBSD: Makefile.inc,v 1.2.4.2 2014/05/22 15:48:11 yamt Exp $
.include "../Makefile.inc"

CWARNFLAGS.clang+=	-Wno-error=unused-function
