#!/usr/bin/awk -f
#
# Copyright (c) 2023 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

function quotify(x) {
	gsub(/'/, "'\\''", x)
	return "'"x"'"
}

function err(s) {
	printf "%s: line %s: error: %s\n", ARGV[0], NR, s >"/dev/stderr"
	printf "# %s\n", $0 >"/dev/stderr"
	errors++
}

function symlink(target, link,  cmd, status) {
	cmd = sprintf("ln -sfn %s %s", quotify(target), quotify(link))
	status = system(cmd)
	if (status != 0)
		err(sprintf("ln failed with status %d", status))
}

function reset() {
	cka_class = ""
	label = ""
	lolab = ""
	certpem = ""
	certworkdir = ""
	skipping = 0
}

function skip() {
	if (VERBOSE)
		printf "line %d: skip from: %s\n", NR, $0
	skipping = 1
}

function parseoctal(s, i, n,  x) {
	x = 0
	n += i
	for (; i < n; i++) {
		x *= 8
		x += int(substr(s, i, 1))
	}
	return x
}

function writeoctaldata(f, desc,  warned, status) {
	if ($2 != "MULTILINE_OCTAL") {
		err(sprintf("%s: Invalid %s type: %s", label, desc, $2))
		skip()
		return -1
	}
	warned = 0
	while (getline) {
		if ($0 == "END")
			break
		for (i = 0; i < length($0); i += 4) {
			if (substr($0, i + 1, 4) !~ /\\[0-7][0-7][0-7]/) {
				if (!warned)
					err(sprintf("%s: Invalid %s data",
					    label, desc))
				warned = 1
				break
			}
			printf "%c", parseoctal($0, i + 2, 3) >f
		}
	}
	status = close(f)
	if ($0 != "END") {
		err(sprintf("%s: Invalid octal data", label))
		warned = 1
	}
	if (warned)
		return -1
	return status
}

function checkoctaldata(f, desc,  fcheck) {
	fcheck = f".tmp"
	if (writeoctaldata(fcheck, desc) != 0)
		return
	cmd = sprintf("cmp -- %s %s >/dev/null && rm -- %s",
	    quotify(f), quotify(fcheck), quotify(fcheck))
	if (system(cmd) != 0) {
		err(sprintf("%s: %s mismatch", label, desc))
		skip()
	}
}

function writecheckoctaldata(f, desc, dowrite) {
	if (dowrite)
		writeoctaldata(f, desc)
	else
		checkoctaldata(f, desc)
}

function skipoctaldata(desc,  warned) {
	if ($2 != "MULTILINE_OCTAL") {
		err(sprintf("%s: Invalid %s type: %s", label, desc, $2))
		skip()
		return -1
	}
	warned = 0
	while (getline) {
		if ($0 == "END")
			return 0
		if ($0 !~ /(\\[0-7][0-7][0-7])*/ && !warned) {
			err(sprintf("%s: Invalid %s data", label, desc))
			warned = 1
		}
	}
	err(sprintf("%s: Invalid octal data", label))
	skip()
	return 1
}

function distrust_after(desc) {
	if ($2 == "CK_BBOOL" && $3 == "CK_FALSE")
		return
	if ($2 == "MULTILINE_OCTAL") {
		skipoctaldata(sprintf("%s distrust deadline", $2))
	} else {
		err(sprintf("%s: Unknown %s distrust type %d, value: %s",
		    label, desc, $2, $3))
	}
	if (VERBOSE)
		printf "line %d: distrust for %s: %s\n", NR, desc, label
	distrusted[lolab] = 1
}

function addtrust(trustfile, desc) {
	if (desc":"lolab in trust_lineno) {
		err(sprintf("%s: Multiple trust lines for %s, first at %d",
		    label, desc, trust_lineno[desc":"lolab]))
		skip()
		return
	}
	trust_lineno[desc":"lolab] = NR
	if ($3 == "CKT_NSS_TRUSTED" || $3 == "CKT_NSS_TRUSTED_DELEGATOR") {
		if (distrusted[lolab]) {
			if (VERBOSE) {
				printf "line %d: distrusted for %s\n", \
				    NR, desc
			}
		} else {
			if (VERBOSE) {
				printf "line %d: trusted for %s\n", \
				    NR, desc
			}
			printf "%s\n", label >trustfile
		}
	} else if ($3 == "CKT_NSS_MUST_VERIFY_TRUST" ||
	    $3 == "CKT_NSS_UNTRUSTED" ||
	    $3 == "CKT_NSS_NOT_TRUSTED") {
		if (VERBOSE) {
			printf "line %d: untrusted for %s\n", \
			    NR, desc
		}
	} else {
		err(sprintf("%s: Unknown trust designation for %s: %s",
		    label, desc, $3))
	}
}


BEGIN {
	if (ARGV[0] == "awk")
		ARGV[0] = "certdata"
	if (!CERTDIR) {
		printf "%s: specify -v CERTDIR=...", ARGV[0] >"/dev/stderr"
		exit 1
	}
	if (!WORKDIR) {
		printf "%s: specify -v WORKDIR=...", ARGV[0] >"/dev/stderr"
		exit 1
	}
	if (!OPENSSL) {
		printf "%s: specify -v OPENSSL=...", ARGV[0] >"/dev/stderr"
		exit 1
	}
	if (!SERVERTRUST)
		SERVERTRUST = "/dev/null"
	if (!EMAILTRUST)
		EMAILTRUST = "/dev/null"
	if (!CODETRUST)
		CODETRUST = "/dev/null"
	printf "" >SERVERTRUST
	printf "" >EMAILTRUST
	printf "" >CODETRUST
	errors = 0
	reset()

	# Special cases.  See
	# https://wiki.mozilla.org/CA/Additional_Trust_Changes for
	# details.

	# `The Turkish Government CA is name-constrained to *.tr.'
	#
	# Implemented in Firefox by
	# https://phabricator.services.mozilla.com/D177242, but OpenSSL
	# has no mechanism to implement this constraint, so we just
	# exclude it altogether.
	special_distrust["TUBITAK_Kamu_SM_SSL_Kok_Sertifikasi_-_Surum_1"] = 1
}

END {
	# Make sure the special cases have been applied.
	for (label in special_distrust)
		err("special distrust not found: %s.pem", label)

	fflush()		# flush all
	close(SERVERTRUST)
	close(EMAILTRUST)
	close(CODETRUST)
	if (errors) {
		printf "%s: exiting with failure on %d error%s\n", ARGV[0], \
		    errors, (errors == 1 ? "" : "s") \
		    >"/dev/stderr"
		exit 1
	}
}

/^ *#/ {			# comment
	next
}

/^ *$/ {
	next
}

$1 == "BEGINDATA" {
	next
}

$1 == "CKA_CLASS" {
	reset()
}

skipping {
	if (VERBOSE)
		printf "line %d: skipping: %s\n", NR, $0
	next
}

$1 == "CKA_CLASS" && $2 != "CK_OBJECT_CLASS" {
	err(sprintf("Invalid class: %s", $2))
	skip()
	next
}

$1 == "CKA_CLASS" && $3 == "CKO_NSS_BUILTIN_ROOT_LIST" {
	skip()
	next
}

$1 == "CKA_CLASS" {
	cka_class = $3
	next
}

$1 == "CKA_TOKEN" ||
$1 == "CKA_NSS_MOZILLA_CA_POLICY" ||
0 {
	if ($2 != "CK_BBOOL") {
		err(sprintf("Invalid %s type: %s; value: %s", $1, $2, $3))
		next
	}
	if ($3 != "CK_TRUE")
		err(sprintf("%s is false", $1))
	next
}

$1 == "CKA_MODIFIABLE" ||
$1 == "CKA_PRIVATE" ||
$1 == "CKA_TRUST_STEP_UP_APPROVED" ||
0 {
	if ($2 != "CK_BBOOL") {
		err(sprintf("Invalid %s type: %s; value: %s", $1, $2, $3))
		next
	}
	if ($3 != "CK_FALSE")
		err(sprintf("%s is true", $1))
	next
}

$1 == "CKA_LABEL" {
	if ($2 != "UTF8") {
		err(sprintf("Non-UTF8 label type: %s; value: %s", $2, $3))
		skip()
		next
	}

	# Clear the `CKA_LABEL UTF8' fields.  (`shift 2', in sh, except
	# that doesn't work here in awk.)
	sub(/CKA_LABEL +UTF8 +/, "", $0)

	# Forbid embedded ", \, and /, as well as bunch of others.
	#
	# - We forbid embedded " because it's not clear what the escape
	#   sequence is.
	#
	# - We forbid \ in case there are escape sequences we don't
	#   know.
	#
	# - We forbid / so that we can always form a directory
	#   component
	#
	# We immediately forbid a bunch of others that might be
	# metacharacters or otherwise problematic, so that the next
	# person to update certdata will be forced to consciously think
	# about how to handle them.
	if ($0 !~ /^"[^[:cntrl:]!"#$%&\*\/:;?\[\\\]\^`\|~]*"$/) {
		err(sprintf("Invalid characters in label: %s", $3))
		skip()
		next
	}

	# Nix the "quotes".
	label = substr($0, 2, length($0) - 2)

	# XXX The `renaming to' messages are inconsistent about whether
	# they apply pre-substitution or post-substitution, so some
	# have spaces and some have underscores.  Oh well.

	# Special cases: Avoid parentheses in two CA names, and
	# non-US-ASCII in one CA name.  It is regrettable to limit
	# ourselves to an anglocentric worldview like this, but this
	# will avoid potential problems with file system pathname
	# encoding and canonicalization downstream.
	if (label ~ /^NetLock Arany \(Class Gold\) F.*$/) {
		label = "NetLock Arany Class Gold"
		if (cka_class == "CKO_CERTIFICATE") {
			printf "line %s: special characters," \
			    " renaming to \"%s\"\n",	      \
			    NR, label
		}
	}
	if (label == "LAWtrust Root CA2 (4096)") {
		label = "LAWtrust Root CA2 4096"
		if (cka_class == "CKO_CERTIFICATE") {
			printf "line %s: special characters," \
			    " renaming to \"%s\"\n",	      \
			    NR, label
		}
	}

	# Avoid spaces in filenames, because Unix.  Not that filenames
	# can't have spaces in Unix, but a lot of downstream tools may
	# get confused by them.
	gsub(/ /, "_", label)

	# Make sure it uses onlypathname-safe characters.
	if (label ~ /[^[:alnum:]._-]/ || label ~ /^\./) {
		err(sprintf("Special CA label: %s", label))
		skip()
		next
	}

	# Make sure it's not empty.
	if (length(label) == 0) {
		err("Empty label")
		skip()
		next
	}

	# Make sure it fits within a reasonable limit as a filename.
	if (length(label) > 100) {
		err(sprintf("Label too long, %d bytes > max %d: %s",
		    length(label), 100, label))
		skip()
		next
	}

	# If this defines the certificate, check for duplicates; if a
	# duplicate is found, assign a counter suffix.
	#
	# XXX This collision numbering might not be stable across
	# updates.  What to do?  Use the serial number?
	#
	# XXX This doesn't use Unicode case-folding.  Let's hope we
	# don't have anything that is a collision under casefold but
	# not under US-ASCII-limited tolower.
	lolab = tolower(label)
	if (cka_class == "CKO_CERTIFICATE") {
		if (VERBOSE)
			printf "line %d: CA \"%s\"\n", NR, label
		if (lolab in label_lineno) {
			label = sprintf("%s.%d", label, ++label_counter[lolab])
			lolab = tolower(label)
			printf "line %s: duplicate, renaming to \"%s\"\n", \
			    NR, label
		}
		label_lineno[lolab] = NR
	} else {
		if (VERBOSE)
			printf "line %d: trust \"%s\"\n", NR, label
		# Hack: Take the highest-numbered counter for this label.
		if (lolab in label_counter) {
			label = sprintf("%s.%d", label, label_counter[lolab])
			lolab = tolower(label)
			printf "line %s: assuming duplicate is \"%s\"\n", \
			    NR, label
		}
		if (!(lolab in label_lineno)) {
			err(sprintf("Missing label: %s", label))
			skip()
			next
		}
	}

	# Apply special cases.
	if (cka_class == "CKO_CERTIFICATE") {
		if (label in special_distrust) {
			printf "line %s: specially distrusting \"%s\"\n", \
			    NR, label
			distrusted[lolab] = 1
			delete special_distrust[label]
		}
	}

	# Compute where this certificate will lives and a workspace.
	certpem = CERTDIR"/"label".pem"
	certworkdir = WORKDIR"/"label

	# If this defines the certificate, create the directory.
	# Otherwise, make sure the directory is there already.
	if (cka_class == "CKO_CERTIFICATE") {
		if (system(sprintf("mkdir -- %s", quotify(certworkdir))) \
		    != 0) {
			errors++
			skip()
			next
		}
	} else {
		if (system(sprintf("test -f %s", quotify(certpem))) != 0) {
			err("%s: Missing certificate for %s", label,
			    cka_class)
		}
		if (system(sprintf("test -d %s", quotify(certworkdir))) != 0)
			err("%s: Missing directory for %s", label, cka_class)
	}

	next
}

# Remaining rules assume we are in the middle of an object block and we
# have a label.

!label {
	err(sprintf("%s: missing label", $1))
	skip()
	next
}

$1 == "CKA_CERTIFICATE_TYPE" {
	if ($2 != "CK_CERTIFICATE_TYPE") {
		err(sprintf("%s: Invalid certificate type type: %s",
		    label, $2))
		skip()
		next
	}
	if ($3 != "CKC_X_509") {
		err(sprintf("%s: Unknown certificate type: %s", label, $2))
		skip()
		next
	}
	next
}

$1 == "CKA_SUBJECT" {
	writeoctaldata(certworkdir"/subject", "subject")
	next
}

$1 == "CKA_ID" {
	if ($0 != "CKA_ID UTF8 \"0\"") {
		err(sprintf("%s: Invalid id: %s", label, $0))
		skip()
		next
	}
	next
}

$1 == "CKA_ISSUER" {
	writecheckoctaldata(certworkdir"/issuer", "issuer",
	    cka_class == "CKO_CERTIFICATE")
	next
}

$1 == "CKA_SERIAL_NUMBER" {
	writecheckoctaldata(certworkdir"/serial", "serial number",
	    cka_class == "CKO_CERTIFICATE")
	next
}

$1 == "CKA_VALUE" {
	if (writeoctaldata(certworkdir"/cert.der", "certificate data"))
		next
	if (system(sprintf("%s x509 -inform DER -outform PEM <%s >%s",
	    quotify(OPENSSL),
	    quotify(certworkdir"/cert.der"),
	    quotify(certpem))))
		err(sprintf("%s: openssl x509 failed", label))
	next
}

$1 == "CKA_CERT_SHA1_HASH" {
	writeoctaldata(certworkdir"/hash.sha1", "SHA-1 hash")
	next
}

$1 == "CKA_CERT_MD5_HASH" {
	writeoctaldata(certworkdir"/hash.md5", "MD5 hash")
	next
}

$1 == "CKA_NSS_SERVER_DISTRUST_AFTER" {
	distrust_after("server")
	next
}

$1 == "CKA_NSS_EMAIL_DISTRUST_AFTER" {
	distrust_after("email")
	next
}

$1 !~ /^CKA_TRUST_/ {
	err(sprintf("%s: Unknown line: %s", label, $0))
	skip()
	next
}

$2 != "CK_TRUST" {
	err(sprintf("%s: Invalid trust line: %s", label, $0))
	skip()
	next
}

# Remaining rules assume we are on a valid CKA_TRUST_* attribute.

$1 == "CKA_TRUST_SERVER_AUTH" {
	addtrust(SERVERTRUST, "server authentication")
	next
}

$1 == "CKA_TRUST_EMAIL_PROTECTION" {
	addtrust(EMAILTRUST, "email protection")
	next
}

$1 == "CKA_TRUST_CODE_SIGNING" {
	addtrust(CODETRUST, "code signing")
	next
}

{
	err(sprintf("%s: Unknown trust domain: %s", label, $1))
}
