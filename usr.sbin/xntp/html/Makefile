#	$NetBSD: Makefile,v 1.2 1999/10/31 22:46:17 veego Exp $

HTML += HTMLPrimer.html accopt.html authcert.html authopt.html authspeed.html
HTML+= biblio.html build.html clockopt.html config.html confopt.html
HTML+= copyright.html debug.html driver1.html driver10.html driver11.html
HTML+= driver12.html driver18.html driver19.html driver2.html driver20.html
HTML+= driver22.html driver23.html driver24.html driver26.html driver27.html
HTML+= driver28.html driver3.html driver4.html driver5.html driver6.html
HTML+= driver7.html driver8.html driver9.html gadget.html hints.html
HTML+= howto.html index.html irig.html kern.html kernpps.html ldisc.html
HTML+= md5cert.html measure.html miscopt.html monopt.html notes.html
HTML+= ntpdate.html ntpq.html ntptime.html ntptrace.html parsedata.html
HTML+= parsenew.html patches.html porting.html pps.html prefer.html
HTML+= rdebug.html refclock.html release.html tickadj.html vxworks.html
HTML+= xntpd.html xntpdc.html

DIR=html/xntp
FILES=${HTML}

docinstall::
	${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${DESTDIR}/${DOCDIR}/${DIR}

.include <bsd.doc.mk>
