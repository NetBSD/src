#	$NetBSD: Makefile,v 1.1 2000/05/02 12:01:11 simonb Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
NTPSRCDIR=html
FILESDIR=${NTPDOCDIR}
FILES=	Oncore-SHMEM.htm accopt.htm assoc.htm audio.htm authopt.htm \
	biblio.htm build.htm clockopt.htm config.htm confopt.htm \
	copyright.htm debug.htm driver1.htm driver10.htm driver11.htm \
	driver12.htm driver16.htm driver18.htm driver19.htm driver2.htm \
	driver20.htm driver22.htm driver23.htm driver24.htm driver26.htm \
	driver27.htm driver28.htm driver29.htm driver3.htm driver30.htm \
	driver32.htm driver33.htm driver34.htm driver35.htm driver36.htm \
	driver37.htm driver4.htm driver5.htm driver6.htm driver7.htm \
	driver8.htm driver9.htm exec.htm extern.htm gadget.htm hints.htm \
	howto.htm htmlprimer.htm index.htm kern.htm kernpps.htm ldisc.htm \
	measure.htm miscopt.htm monopt.htm mx4200data.htm notes.htm ntpd.htm \
	ntpdate.htm ntpdc.htm ntpq.htm ntptime.htm ntptrace.htm parsedata.htm \
	parsenew.htm patches.htm porting.htm pps.htm prefer.htm qth.htm \
	quick.htm rdebug.htm refclock.htm release.htm tickadj.htm vxworks.htm \
	y2k.htm
.endif
SUBDIR=	hints
MKOBJ=  no

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
