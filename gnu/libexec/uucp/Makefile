#	$NetBSD: Makefile,v 1.6 1999/03/29 11:40:36 kleink Exp $
#
# This is the Makefile for Taylor UUCP

SUBDIR=	libunix libuucp libuuconf \
	cu uuchk uucico uuconv uucp uulog uuname uupick uusched \
	uustat uuto uux uuxqt doc

all-cu all-uuchk all-uucico all-uuconv all-uucp all-uulog all-uuname \
all-uupick all-uusched all-uustat all-uuto all-uux all-uuxqt: \
	all-libunix all-libuucp all-libuuconf

.include <bsd.subdir.mk>
