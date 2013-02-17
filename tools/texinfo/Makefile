#	$NetBSD: Makefile,v 1.10 2013/02/17 01:47:59 christos Exp $

MODULE=		texinfo

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}

.include "${.CURDIR}/../Makefile.gnuhost"

.if !empty(.MAKE.OS:M*CYGWIN*) && target(install)
.END: install-texinfo
install-texinfo: 
	${NETBSDSRCDIR}/tools/binstall/mkmanifest ${TOOLDIR}/bin/${_TOOL_PREFIX}install-info.exe
.endif
