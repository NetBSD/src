#	$NetBSD: Makefile,v 1.2.2.1 2014/08/10 07:09:33 tls Exp $

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.inc"

.PATH: ${IDIST}/html

.if ${MKDOC} != "no"
NTP_DOCFILES= \
		accopt.html \
		assoc.html \
		audio.html \
		authopt.html \
		bugs.html \
		build.html \
		clockopt.html \
		comdex.html \
		config.html \
		confopt.html \
		copyright.html \
		debug.html \
		decode.html \
		drivers/driver1.html \
		drivers/driver3.html \
		drivers/driver4.html \
		drivers/driver5.html \
		drivers/driver6.html \
		drivers/driver7.html \
		drivers/driver8.html \
		drivers/driver9.html \
		drivers/driver10.html \
		drivers/driver11.html \
		drivers/driver12.html \
		drivers/driver16.html \
		drivers/driver18.html \
		drivers/driver19.html \
		drivers/driver20.html \
		drivers/driver22.html \
		drivers/driver26.html \
		drivers/driver27.html \
		drivers/driver28.html \
		drivers/driver29.html \
		drivers/driver30.html \
		drivers/driver31.html \
		drivers/driver32.html \
		drivers/driver33.html \
		drivers/driver34.html \
		drivers/driver35.html \
		drivers/driver36.html \
		drivers/driver37.html \
		drivers/driver38.html \
		drivers/driver39.html \
		drivers/driver40.html \
		drivers/driver42.html \
		drivers/driver43.html \
		drivers/driver44.html \
		drivers/mx4200data.html \
		drivers/oncore-shmem.html \
		drivers/scripts/footer.txt \
		drivers/scripts/style.css \
		drivers/tf582_4.html \
		extern.html \
		hints.html \
		howto.html \
		index.html \
		kern.html \
		kernpps.html \
		keygen.html \
		miscopt.html \
		monopt.html \
		msyslog.html \
		ntp_conf.html \
		ntpd.html \
		ntpdate.html \
		ntpdc.html \
		ntpdsim.html \
		ntpdsim_new.html \
		ntpq.html \
		ntptime.html \
		ntptrace.html \
		parsedata.html \
		parsenew.html \
		pps.html \
		prefer.html \
		quick.html \
		rate.html \
		rdebug.html \
		refclock.html \
		release.html \
		scripts/accopt.txt \
		scripts/audio.txt \
		scripts/authopt.txt \
		scripts/clockopt.txt \
		scripts/command.txt \
		scripts/config.txt \
		scripts/confopt.txt \
		scripts/external.txt \
		scripts/footer.txt \
		scripts/install.txt \
		scripts/manual.txt \
		scripts/misc.txt \
		scripts/miscopt.txt \
		scripts/monopt.txt \
		scripts/refclock.txt \
		scripts/style.css \
		sitemap.html \
		sntp.html \
		tickadj.html \
		xleave.html

.for F in ${NTP_DOCFILES:O:u}
_FDIR:=		ntp/${F:H}				# subdir
_FDIR:=		${_FDIR:S,/.$,,}			# remove trivial directory
_FNAME:=	${F:T}					# name override
_F:=		${DESTDIR}${NTP_HTMLDIR}/${_FDIR}/${_FNAME}		# installed path
_FILE:=		${F}

.if ${MKUPDATE} == "no"
${_F}!		${_FILE} __fileinstall			# install rule
.if !defined(BUILD) && !make(all) && !make(${_FILE})
${_F}!		.MADE					# no build at install
.endif
.else
${_F}:		${_FILE} __fileinstall			# install rule
.if !defined(BUILD) && !make(all) && !make(${_FILE})
${_F}:		.MADE					# no build at install
.endif
.endif

filesinstall::	${_F}
.PRECIOUS: 	${_F}					# keep if install fails
.endfor

.endif

.include <bsd.prog.mk>
