#	$NetBSD: Makefile,v 1.6 2001/01/09 08:52:59 enami Exp $

# Note that the order here is picked to look much like that in the
# original Makefile.in in order to make it easier to visually compare
# when updating to a new version of Postfix.

# The first four of these are libraries and have to be built first.

.include <bsd.own.mk>

SUBDIR= util global dns masterlib .WAIT postfix master sendmail error \
	pickup cleanup smtpd local trivial-rewrite qmgr smtp bounce pipe \
	showq postalias postcat postconf postdrop postkick postlock postlog \
	postmap postsuper man # postfix-script

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

# We don't want to build postfix-script (at least for now) because
# we're now treating it as a configuration file, per Wietse Venema's
# request. We now have conf/Makefile install a copy into
# share/examples/postfix, and postfix-etc/Makefile (only called by
# make distribution) installs a copy in /etc/postfix

# also not built: smtpstone (unneeded), spawn (experimental),
# and html (unsure what to do with it (yet))

# The distribution: target is invoked from the make distribution
# target in src/etc/Makefile. It is there for building the /etc/
# set. You don't want to run it normally as it would blow away your
# /etc/postfix directory.

distribution:
	cd ${.CURDIR}/postfix-etc; ${MAKE} install


.include <bsd.subdir.mk>
