# $NetBSD: Makefile.inc,v 1.1 2007/01/14 04:36:13 christos Exp $
# Id: Makefile.inc,v 1.1 2000/07/14 18:16:22 iwasaki Exp 
# $FreeBSD: src/usr.sbin/acpi/Makefile.inc,v 1.7 2002/09/17 14:56:23 jhb Exp $

WARNS=4
CPPFLAGS+= -I${.CURDIR}/..

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
