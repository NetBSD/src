#
# This is the Makefile for Taylor UUCP
#

SUBDIR=		libunix libuucp libuuconf \
		cu uuchk uucico uuconv uucp uulog uuname uupick uusched \
		uustat uuto uux uuxqt

owner=		uucp

beforeinstall:
	install -d -o ${owner} ${DESTDIR}/var/spool/uucp
	install -d -o ${owner} ${DESTDIR}/var/spool/uucppublic

.include <bsd.subdir.mk>
