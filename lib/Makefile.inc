#	$NetBSD: Makefile.inc,v 1.4 2004/07/30 07:02:53 lukem Exp $

WARNS?=		2
WFORMAT?=	2


.if ${MKPIC:Uno} != "no"
.if defined(DEPLIBS)
.for _lib in ${DEPLIBS}
.if !defined(DEPLIBOD.${_lib})
DEPLIBOD.${_lib}!=	cd ${.CURDIR}/../lib${_lib} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	DEPLIBOD.${_lib}
.endif
LDADD+=			-L${DEPLIBOD.${_lib}} -l${_lib}
DPADD+=			${DEPLIBOD.${_lib}}/lib${_lib}.so
.endfor
.endif	# defined(DEPLIBS)
.endif	# MKPIC != no
