#	$NetBSD: Makefile,v 1.2.2.2 2023/09/04 17:33:27 martin Exp $
#

MOZCERTS=	/usr/share/certs/mozilla
FILESDIR=	${MOZCERTS}/all

TRUSTDOMAINS=	server email code

# Normally it is a bad idea to use file system globs to list sources in
# a makefile, but we replace the directory wholesale on regen using the
# same pattern below.  So in this case, maintaining an explicit list of
# files would be more error-prone.
CERTS!=		cd ${.CURDIR:Q} && echo certs/*.pem
FILES+=		${CERTS}

.for D in ${TRUSTDOMAINS}
$D_CERTS!=	cat ${.CURDIR:Q}/$D.trust
.  for C in ${$D_CERTS}
SYMLINKS+=	../all/$C.pem ${MOZCERTS}/$D/$C.pem
.  endfor
.endfor

# The upstream Mozilla certdata.txt lives in the Mozilla nss repository
# at https://hg.mozilla.org/projects/nss, under
# lib/ckfw/builtins/certdata.txt.
#
# Updates:
#
#	1. Go to:
#	   https://hg.mozilla.org/projects/nss/log/tip/lib/ckfw/builtins/certdata.txt
#
#	2. Find the top revision and follow the link to `diff'.
#
#	3. For the file lib/ckfw/builtins/certdata.txt, follow the link to
#	   `file'.
#
#	4. Follow the link to `raw'.
#
#	5. Record the date of the latest revision and the URL to the
#	   raw file in the comment below (includes hg revision).
#
#	6. Verify that the file matches when downloaded from at least
#	   three different networks.  (Suggestions: Your home
#	   residential network, a TNF server, and Tor.)
#
#	7. Once you have verified this, commit certdata.txt.
#
#	8. Review https://wiki.mozilla.org/CA/Additional_Trust_Changes
#	   for new special cases and apply to certdata.awk if
#	   appropriate.
#
#	9. After committing certdata.txt and updating certdata.awk, run
#	   `make regen' and verify that it builds and installs and
#	   generally looks sensible.
#
#	10. Once you have verified that it builds and installs, cvs add
#	   any new files and cvs rm any deleted files under certs/,
#	   and commit certs/ and *.trust.
#
# Latest revision, from 2023-07-19:
#
# https://hg.mozilla.org/projects/nss/raw-file/f479bdba756c78ef9355a48c88744c69fdb4768e/lib/ckfw/builtins/certdata.txt
#
regen: .PHONY
	rm -f certs/*.pem
	rm -f ${TRUSTDOMAINS:=.trust}
	mkdir tmp
	awk -f certdata.awk \
		-v CERTDIR=certs \
		-v CODETRUST=code.trust \
		-v EMAILTRUST=email.trust \
		-v OPENSSL=openssl \
		-v SERVERTRUST=server.trust \
		-v WORKDIR=tmp \
		<${.CURDIR:Q}/../dist/certdata.txt
	rm -rf tmp

.include <bsd.files.mk>
.include <bsd.inc.mk>		# XXX
.include <bsd.links.mk>
