#	$NetBSD: Makefile,v 1.1 2003/09/14 06:42:24 lukem Exp $

NOMAN=		yes

.include <bsd.own.mk>

PROG=		ssh-askpass
SRCS=		drawing.c dynlist.c resources.c x11-ssh-askpass.c

APPDEFS=	SshAskpass.ad

LNFILES=	SshAskpass-default.ad SshAskpass.ad

FILESDIR=	${X11ROOTDIR}/share/examples/ssh-askpass
FILES=		SshAskpass-1337.ad SshAskpass-NeXTish.ad \
		SshAskpass-default.ad SshAskpass-green.ad SshAskpass-motif.ad

CPPFLAGS+=	-I.
DPSRCS+=	SshAskpass_ad.h
CLEANFILES+=	SshAskpass_ad.h
SshAskpass_ad.h: SshAskpass.ad
	rm -f ${.TARGET}
	sed -n '/^[^!]/s/.*/"&",/p' <${.ALLSRC} > ${.TARGET}

LDADD+=		-lXt -lSM -lICE -lXext -lX11
DPADD+=		${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:		${X11SRCDIR.local}/programs/x11-ssh-askpass

.include <bsd.x11.mk>
.include <bsd.prog.mk>
