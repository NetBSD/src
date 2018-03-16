#	$NetBSD: Makefile,v 1.1.2.2 2018/03/16 01:33:24 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_60

CPPFLAGS+=	-D COMPAT_60 -DCOMPAT_70 -DCOMPAT_80

# If being built for amd64, i386, or XEN3_DOM0 (but _not_ DOMU), we
# need to add
#
# SRCS+=	CPU_UCODE

SRCS+=	kern_sa_60.c tty_60.c kern_time_60.c

.include <bsd.kmodule.mk>
