#	$NetBSD: Makefile,v 1.3.2.2 2023/09/04 17:33:27 martin Exp $
#

MAN=		certctl.8
SCRIPTS=	certctl.sh

# XXX This is a hack to install certs.conf both in /etc/openssl (in the
# etc set) and in /usr/share/examples/certctl (in the base set).
# Really, all files in /etc should have /usr/share/examples versions;
# once you arrange the build to do that, you can get rid of this hack.
etc-certs.conf base-certs.conf: certs.conf
	cat ${.ALLSRC} >${.TARGET}.tmp
	${MV} -f ${.TARGET}.tmp ${.TARGET}

CONFIGFILES+=	etc-certs.conf
FILESBUILD_etc-certs.conf=	yes
FILESDIR_etc-certs.conf=	/etc/openssl
FILESNAME_etc-certs.conf=	certs.conf
FILESMODE_etc-certs.conf=	644

FILES+=		base-certs.conf
FILESBUILD_base-certs.conf=	yes
FILESDIR_base-certs.conf=	/usr/share/examples/certctl
FILESNAME_base-certs.conf=	certs.conf

.include <bsd.prog.mk>
