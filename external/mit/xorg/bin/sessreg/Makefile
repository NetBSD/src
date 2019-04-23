#	$NetBSD: Makefile,v 1.6.8.1 2019/04/23 10:24:54 martin Exp $

.include <bsd.own.mk>

PROG=	sessreg

.include "../xdm/Makefile.xdm"

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include

X11EXTRAMANDEFS+=	-e 's,__ttys_file__,/etc/ttys,g' \
			-e 's,__utmp_file__,${XDMPIDDIR}/utmp,g' \
			-e 's,__wtmp_file__,${XDMLOGDIR}/wtmp,g'

X11EXTRAMANTRANSFORMS+= \
	__utmp_manpage__	utmpx \
	__wtmp_manpage__	wtmpx \
	__lastlog_file__	lastlogx

.include <bsd.x11.mk>
.include <bsd.prog.mk>

.PATH:		${X11SRCDIR.${PROG}}
.PATH:		${X11SRCDIR.${PROG}}/man
