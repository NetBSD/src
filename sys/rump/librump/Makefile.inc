#	$NetBSD: Makefile.inc,v 1.3 2009/01/01 19:07:43 pooka Exp $
#

#
# Mandatory flags
#
CPPFLAGS+=	-DMODULAR

#
# Optional flags
#

# Comment this out if you want the build to default to allowing threads
CPPFLAGS+=	-DRUMP_WITHOUT_THREADS
