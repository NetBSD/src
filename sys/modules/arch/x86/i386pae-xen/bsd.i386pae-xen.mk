#	$NetBSD: bsd.i386pae-xen.mk,v 1.1.30.1 2019/06/10 22:09:10 christos Exp $

.ifndef _BSD_I386PAE_XEN_MK_
_BSD_I386PAE_XEN_MK_=1

KMODULEARCHDIR:=	i386pae-xen

XEN=	1
PAE=	1

#CPPFLAGS+=	-DPAE -DXEN

.endif # _BSD_I386PAE_XEN_MK_
