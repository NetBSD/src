# $NetBSD: Makefile.inc,v 1.2 2009/04/22 15:23:01 lukem Exp $
# Id: Makefile.inc,v 1.1 2000/07/14 18:16:22 iwasaki Exp 
# $FreeBSD: src/usr.sbin/acpi/Makefile.inc,v 1.7 2002/09/17 14:56:23 jhb Exp $

CPPFLAGS+= -I${.CURDIR}/..

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
