#	$NetBSD: Makefile,v 1.8 2020/10/10 09:17:33 nia Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.fontconfig}/conf.d
FONTCONFIG_LINKS= \
	10-hinting-slight.conf \
	10-scale-bitmap-fonts.conf \
	20-unhint-small-vera.conf \
	30-metric-aliases.conf \
	40-nonlatin.conf \
	45-generic.conf \
	45-latin.conf \
	49-sansserif.conf \
	50-user.conf \
	51-local.conf \
	60-generic.conf \
	60-latin.conf \
	65-fonts-persian.conf \
	65-nonlatin.conf \
	69-unifont.conf \
	70-no-bitmap-webfonts.conf \
	80-delicious.conf \
	90-synthetic.conf

CONFAVAILDIR=	../conf.avail
CONFDIR=	/etc/fonts/conf.d

.for _conffile in ${FONTCONFIG_LINKS}
CONFIGSYMLINKS+=${CONFAVAILDIR}/${_conffile} ${CONFDIR}/${_conffile}
.endfor

.include <bsd.x11.mk>
.include <bsd.prog.mk>

# obsolete 30-urw-aliases.conf 
