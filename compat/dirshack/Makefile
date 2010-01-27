#	$NetBSD: Makefile,v 1.1.2.3 2010/01/27 06:52:47 matt Exp $

# hacky method to get compat multilib base objdirs created before
# make tries to go create the subdirs used for builds.

# the problem is that make handles objdir creation for subdirs before it
# handles this current directory, so when make cd's into $arch/$libtype
# and from there into the ../../lib dirs, it ends up setting the forced
# MAKEOBJDIRPREFIX to something based upon ${.CURDIR}, since the objdir
# doesn't exist yet.
# 
# our solution is simple - from this Makefile we traverse the same list
# of $arch/$libtype's with "BOOTSTRAP_SUBDIR=".  then the compat/Makefile
# handles these subdirs as normal, with the base objdir created.

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if make(obj)

.include "../Makefile.md_subdir"

MAKEDIRTARGETENV=	BOOTSTRAP_SUBDIRS=

SUBDIR=	${MD_SUBDIR:N.WAIT:C/^/..\//}

.endif	# make(obj)
.endif	# MKCOMPAT != no

.include <bsd.subdir.mk>
