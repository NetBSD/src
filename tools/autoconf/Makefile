#	$NetBSD: Makefile,v 1.6 2013/08/07 17:50:55 apb Exp $

MODULE=		autoconf

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}

.include "${.CURDIR}/../Makefile.gnuhost"

# This value of M4=... makes our ${TOOL_M4} emulate GNU M4.
# Makefile.gnuhost stores a different value of M4=... in CONFIGURE_ENV,
# so we have to append to CONFIGURE_ENV *after* including
# Makefile.gnuhost.
#
CONFIGURE_ENV+=	M4=${TOOL_M4:Q}" -g -D__gnu__"
