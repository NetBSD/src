#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp
#	$Id: Makefile,v 1.4 1995/07/10 01:39:58 cgd Exp $

PROG=	mrouted
SRCS=	igmp.c inet.c kern.c config.c cfparse.c main.c route.c vif.c \
	prune.c callout.c
MAN=	mrouted.8

CLEANFILES+= cfparse.c y.tab.h

.include <bsd.prog.mk>
