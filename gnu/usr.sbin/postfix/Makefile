#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:50 perry Exp $

# Note that the order here is picked to look much like that in the
# original Makefile.in in order to make it easier to visually compare
# when updating to a new version of Postfix.
# The first four of these are libraries and have to be built first anyway.
SUBDIR=	util global dns masterlib master postfix sendmail error \
	pickup cleanup smtpd local trivial-rewrite qmgr smtp bounce pipe \
	showq postalias postcat postconf postdrop postkick postlock postlog \
	postmap postsuper man postfix-script conf

# not built: smtpstone (unneeded), spawn (experimental),
# and html (unsure what to do with it (yet))

.include <bsd.subdir.mk>
