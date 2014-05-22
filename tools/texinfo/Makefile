#	$NetBSD: Makefile,v 1.9.54.1 2014/05/22 11:42:35 yamt Exp $

MODULE=		texinfo

CONFIGURE_ARGS=	--program-prefix=${_TOOL_PREFIX}

.include "${.CURDIR}/../Makefile.gnuhost"

.if !empty(.MAKE.OS:M*CYGWIN*) && target(install)
.END: install-texinfo
install-texinfo: 
	${HOST_SH} ${NETBSDSRCDIR}/tools/binstall/mkmanifest ${TOOLDIR}/bin/${_TOOL_PREFIX}install-info.exe
.endif
