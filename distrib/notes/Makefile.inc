#	$NetBSD: Makefile.inc,v 1.16 2001/01/14 21:30:58 mycroft Exp $
#

# Ross Harvey <ross@netbsd.org>

.include <bsd.own.mk>	# So we get /etc/mk.conf vars.

M?=	${.CURDIR:T}
V!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh
V_S!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s

MAIN=	${.CURDIR}/../common/main ${.CURDIR}/../common/macros ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
SRCS=	${MAIN} whatis contents hardware xfer prep install\
	upgrade donations legal.common legal postinstall ../Makefile.inc\
	${MERGED_SRCS}
ICMD=	${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
	-o root -g wheel -m ${NONBINMODE} 

.if defined(DESTDIR)
PRESET=	${GFLAGS} -dM=${M} -dV=${V} -d.CURDIR=${.CURDIR} -r${M}=1 -M${DESTDIR}/usr/share/tmac
.else
PRESET=	${GFLAGS} -dM=${M} -dV=${V} -d.CURDIR=${.CURDIR} -r${M}=1
.endif

POST_PLAIN= -P-b -P-u -P-o

ARGS_PS=	${PRESET} -dformat=PostScript
ARGS_TXT=	${PRESET} -dformat=ASCII ${POST_PLAIN} -Tascii -mtty-char
ARGS_HTML=	${PRESET} -dformat=HTML ${POST_PLAIN} -Tlatin1 -ww
ARGS_MORE=	${PRESET} -dformat=more -P-h -Tascii -mtty-char

#
# For example...
#
#	.if ri386 ...stuff...
#	.Ss "Install notes for NetBSD/\*[MACHINE]"
#

CLEANFILES+=${TARGS}

.for i in ps txt html more
all: ${TARG}.$i
.endfor

${TARG}.ps: ${SRCS}
	groff ${ARGS_PS}   -mdoc      ${MAIN} > $@

${TARG}.pdf: ${TARG}.ps
	ps2pdf ${TARG}.ps $@

${TARG}.txt: ${SRCS}
	groff ${ARGS_TXT}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS}
	groff ${ARGS_HTML} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS}
	groff ${ARGS_MORE} -mdoc      ${MAIN} > $@

echosrcs! ${SRCS}
	@echo ${.ALLSRC}

echomore!
	@echo ${.OBJDIR}/${TARG}.more

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release:
.for i in ${TARGS}
	${ICMD} ${i} ${RELEASEDIR}/.
.endfor
.endif

.PATH: ${.CURDIR}/../common

# make the "afterdepend" rule happy
.depend:
	@touch $@
