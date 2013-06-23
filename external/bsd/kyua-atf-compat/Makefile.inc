# $NetBSD: Makefile.inc,v 1.1.6.2 2013/06/23 06:26:38 tls Exp $

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-atf-compat/dist
.PATH:		${SRCDIR}

BUILD_SCRIPT = \
    sed -e 's,__ATF_CONFDIR__,/etc/atf,g' \
        -e 's,__ATF_SH__,/usr/bin/atf-sh,g' \
        -e 's,__BINDIR__,/usr/bin,g' \
        -e 's,__PKGDATADIR__,/usr/share/kyua-atf-compat,g' \
        -e 's,__SH__,/bin/sh,g' \
        ${SRCDIR}/$${src} >$${dst}.new && \
    chmod +x $${dst}.new && \
    mv $${dst}.new $${dst}
