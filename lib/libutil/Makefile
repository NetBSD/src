#	from: @(#)Makefile	5.10 (Berkeley) 5/6/91
#	$Id: Makefile,v 1.3 1993/08/01 05:39:50 mycroft Exp $

LIB=	util
CFLAGS+=-DLIBC_SCCS
SRCS=	daemon.c getloadavg.c kvm.c login.c login_tty.c logout.c logwtmp.c \
	pty.c pwcache.c
MAN3=	getloadavg.0 pwcache.0

MLINKS+=pwcache.3 user_from_uid.3 pwcache.3 group_from_gid.3

.include <bsd.lib.mk>
