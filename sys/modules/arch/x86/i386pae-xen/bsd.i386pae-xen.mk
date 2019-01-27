#	$NetBSD: bsd.i386pae-xen.mk,v 1.2 2019/01/27 02:08:44 pgoyette Exp $

.ifndef _BSD_I386PAE_XEN_MK_
_BSD_I386PAE_XEN_MK_=1

KMODULEARCHDIR:=	i386pae-xen

XEN=	1
PAE=	1

#CPPFLAGS+=	-DPAE -DXEN

.endif # _BSD_I386PAE_XEN_MK_
