#	$NetBSD: bsd.info.mk,v 1.18 2000/06/10 14:12:04 mycroft Exp $

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:		all
.endif

MAKEINFO?=	makeinfo
INFOFLAGS?=	
INSTALL_INFO?=	install-info

.PHONY:		infoinstall cleaninfo
cleandir distclean: cleaninfo

.SUFFIXES: .txi .texi .texinfo .info

.txi.info .texi.info .texinfo.info:
	@${MAKEINFO} ${INFOFLAGS} --no-split -o $@ $<

.if defined(TEXINFO) && !empty(TEXINFO)
INFOFILES=	${TEXINFO:C/\.te?xi(nfo)?$/.info/}
.NOPATH:	${INFOFILES}

.if ${MKINFO} != "no"
realinstall: infoinstall
realall: ${INFOFILES}
.endif

cleaninfo:
	rm -f ${INFOFILES}

infoinstall:: ${INFOFILES:@F@${DESTDIR}${INFODIR_${F}:U${INFODIR}}/${INFONAME_${F}:U${INFONAME:U${F:T}}}@}
.PRECIOUS: ${INFOFILES:@F@${DESTDIR}${INFODIR_${F}:U${INFODIR}}/${INFONAME_${F}:U${INFONAME:U${F:T}}}@}
.if !defined(UPDATE)
.PHONY: ${INFOFILES:@F@${DESTDIR}${INFODIR_${F}:U${INFODIR}}/${INFONAME_${F}:U${INFONAME:U${F:T}}}@}
.endif

__infoinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} \
	    -o ${INFOOWN_${.ALLSRC:T}:U${INFOOWN}} \
	    -g ${INFOGRP_${.ALLSRC:T}:U${INFOGRP}} \
	    -m ${INFOMODE_${.ALLSRC:T}:U${INFOMODE}} \
	    ${.ALLSRC} ${.TARGET}
	@${INSTALL_INFO} --remove --info-dir=${DESTDIR}${INFODIR} ${.TARGET}
	${INSTALL_INFO} --info-dir=${DESTDIR}${INFODIR} ${.TARGET}

.for F in ${INFOFILES}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${INFODIR_${F}:U${INFODIR}}/${INFONAME_${F}:U${INFONAME:U${F:T}}}: .MADE
.endif
${DESTDIR}${INFODIR_${F}:U${INFODIR}}/${INFONAME_${F}:U${INFONAME:U${F:T}}}: ${F} __infoinstall
.endfor
.else
cleaninfo:
.endif

# Make sure all of the standard targets are defined, even if they do nothing.
clean depend includes lint regress tags:
