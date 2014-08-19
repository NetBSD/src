#	$NetBSD: Makefile,v 1.4.60.1 2014/08/20 00:04:53 tls Exp $

MODULE=		autoconf

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}

.include "${.CURDIR}/../Makefile.gnuhost"

# This value of M4=... makes our ${TOOL_M4} emulate GNU M4.
# Makefile.gnuhost stores a different value of M4=... in CONFIGURE_ENV,
# so we have to append to CONFIGURE_ENV *after* including
# Makefile.gnuhost.
#
CONFIGURE_ENV+=	M4=${TOOL_M4:Q}" -g -D__gnu__"
