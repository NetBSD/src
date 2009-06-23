#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:51 tron Exp $

# The first four/five of these are libraries and have to be built first.

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"
SUBDIR=	util global dns masterlib tls .WAIT tlsmgr milter xsasl .WAIT
.else
SUBDIR=	util global dns masterlib milter xsasl .WAIT
.endif
SUBDIR+=README_FILES anvil bounce cleanup discard error flush html \
	local man master oqmgr pickup pipe postalias \
	postcat postconf postdrop postfix postkick postlock \
	postlog postmap postmulti postqueue postsuper proxymap qmgr scache \
	sendmail showq smtp smtpd spawn trivial-rewrite verify \
	virtual proxymap

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

# We don't want to build postfix-script (at least for now) because
# we're now treating it as a configuration file, per Wietse Venema's
# request. We now have conf/Makefile install a copy into
# share/examples/postfix, and postfix-etc/Makefile (only called by
# make configinstall) installs a copy in /etc/postfix

# also not built: fsstone (unneeded), smtpstone (unneeded), qmqpd (unneeded)

# The configinstall: target is invoked from the make distribution
# target in src/etc/Makefile. It is there for building the /etc/
# set. You don't want to run it normally as it would blow away your
# /etc/postfix directory.

configinstall:
	${MAKEDIRTARGET} postfix-etc configinstall

.include "Makefile.inc"

.include <bsd.subdir.mk>
