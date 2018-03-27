#	$NetBSD: bsd.amd64-xen.mk,v 1.1.28.2 2018/03/27 03:57:37 pgoyette Exp $

.ifndef _BSD_AMD64_XEN_MK_
_BSD_AMD64_XEN_MK_=1

KMODULEARCHDIR:=	amd64-xen

XEN=	1

#CPPFLAGS+=	-DXEN

.endif # _BSD_AMD64_XEN_MK_
