#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$Id: Makefile,v 1.7 1994/06/19 02:17:46 cgd Exp $

# The following modules do not build/install:
# 10.named, 13.amd

# Missing:
# 02.config 11.timedop 12.timed

# Missing from 4.4BSD-Lite:
# 14.uucpimpl 15.uucpnet 16.security 17.password

BINDIR=	/usr/share/doc/smm
FILES=	00.contents Makefile Title
.ifnmake install
SUBDIR=	01.setup 03.fsck 04.quotas 05.fastfs 06.nfs 07.lpd \
	08.sendmailop 09.sendmail 18.net
.else
TOP=	../../..
SUBDIR=	01.setup ${TOP}/sbin/fsck/SMM.doc 04.quotas 05.fastfs 06.nfs \
	${TOP}/usr.sbin/lpr/SMM.doc ${TOP}/usr.sbin/sendmail/doc/op \
	${TOP}/usr.sbin/sendmail/doc/intro 18.net
.endif

Title.ps: ${FILES}
	groff Title > ${.TARGET}

contents.ps: ${FILES}
	groff -ms 00.contents > ${.TARGET}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>
