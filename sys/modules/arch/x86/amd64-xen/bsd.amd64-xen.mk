#	$NetBSD: bsd.amd64-xen.mk,v 1.1.30.1 2019/06/10 22:09:10 christos Exp $

.ifndef _BSD_AMD64_XEN_MK_
_BSD_AMD64_XEN_MK_=1

KMODULEARCHDIR:=	amd64-xen

XEN=	1

#CPPFLAGS+=	-DXEN

.endif # _BSD_AMD64_XEN_MK_
