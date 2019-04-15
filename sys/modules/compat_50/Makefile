#	$NetBSD: Makefile,v 1.4 2019/04/15 02:07:11 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_50

CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_50_mod.c
SRCS+=	kern_50.c kern_time_50.c kern_select_50.c
SRCS+=	vfs_syscalls_50.c uipc_syscalls_50.c uvm_50.c
SRCS+=	rndpseudo_50.c clockctl_50.c if_spppsubr50.c
SRCS+=	kern_uipc_socket_50.c

CPPFLAGS+=	-DINET -DINET6
# XXX someday we'll need to add
#CPPFLAGS+=	-DSCTP -DNET_MPSAFE

SRCS+=	rtsock_50.c

.PATH:	${S}/fs/puffs

SRCS+=	puffs_compat.c

.PATH:  ${S}/dev/wscons

SRCS+=  wsevent_50.c

.PATH:	${S}/dev

SRCS+=	vnd_50.c

WARNS=	3

.include <bsd.kmodule.mk>
