#!/bin/sh

#	$NetBSD: t_certctl.sh,v 1.8.2.3 2023/09/06 15:04:33 martin Exp $
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

CERTCTL="certctl -C certs.conf -c certs -u untrusted"

# setupconf <subdir>...
#
#	Create certs/ and set up certs.conf to search the specified
#	subdirectories of the source directory.
#
setupconf()
{
	local sep subdir dir

	mkdir certs
	cat <<EOF >certs.conf
netbsd-certctl 20230816

# comment at line start
	# comment not at line start, plus some intentional whitespace
   
# THE WHITESPACE ABOVE IS INTENTIONAL, DO NOT DELETE
EOF
	# Start with a continuation line separator; then switch to
	# non-continuation lines.
	sep=$(printf ' \\\n\t')
	for subdir; do
		dir=$(atf_get_srcdir)/$subdir
		cat <<EOF >>certs.conf
path$sep$(printf '%s' "$dir" | vis -M)
EOF
		sep=' '
	done
}

# check_empty
#
#	Verify the certs directory is empty after dry runs or after
#	clearing the directory.
#
check_empty()
{
	local why

	why=${1:-dry run}
	for x in certs/*; do
		if [ -e "$x" -o -h "$x" ]; then
			atf_fail "certs/ should be empty after $why"
		fi
	done
}

# check_nonempty
#
#	Verify the certs directory is nonempty.
#
check_nonempty()
{
	for x in certs/*.0; do
		test -e "$x" && test -h "$x" && return
	done
	atf_fail "certs/ should be nonempty"
}

# checks <certsN>...
#
#	Run various checks with certctl.
#
checks()
{
	local certs1 diginotar_base diginotar diginotar_hash subdir srcdir

	certs1=$(atf_get_srcdir)/certs1
	diginotar_base=Explicitly_Distrust_DigiNotar_Root_CA.pem
	diginotar=$certs1/$diginotar_base
	diginotar_hash=$(openssl x509 -hash -noout <$diginotar)

	# Do a dry run of rehash and make sure the directory is still
	# empty.
	atf_check -s exit:0 $CERTCTL -n rehash
	check_empty

	# Distrust and trust one CA, as a dry run.  The trust should
	# fail because it's not currently distrusted.
	atf_check -s exit:0 $CERTCTL -n untrust "$diginotar"
	check_empty
	atf_check -s not-exit:0 -e match:currently \
	    $CERTCTL -n trust "$diginotar"
	check_empty

	# Do a real rehash, not a dry run.
	atf_check -s exit:0 $CERTCTL rehash

	# Make sure all the certificates are trusted.
	for subdir; do
		case $subdir in
		/*)	srcdir=$subdir;;
		*)	srcdir=$(atf_get_srcdir)/$subdir;;
		esac
		for cert in "$srcdir"/*.pem; do
			# Verify the certificate is linked by its base name.
			certbase=$(basename "$cert")
			atf_check -s exit:0 -o inline:"$cert" \
			    readlink -n "certs/$certbase"

			# Verify the certificate is linked by a hash.
			hash=$(openssl x509 -hash -noout <$cert)
			counter=0
			found=false
			while [ $counter -lt 10 ]; do
				if cmp -s "certs/$hash.$counter" "$cert"; then
					found=true
					break
				fi
				counter=$((counter + 1))
			done
			if ! $found; then
				atf_fail "missing $cert"
			fi

			# Delete both links.
			rm "certs/$certbase"
			rm "certs/$hash.$counter"
		done
	done

	# Verify the certificate bundle is there with the right
	# permissions (0644) and delete it.
	#
	# XXX Verify its content.
	atf_check -s exit:0 test -f certs/ca-certificates.crt
	atf_check -s exit:0 test ! -h certs/ca-certificates.crt
	atf_check -s exit:0 -o inline:'100644\n' \
	    stat -f %p certs/ca-certificates.crt
	rm certs/ca-certificates.crt

	# Make sure after deleting everything there's nothing left.
	check_empty "removing all expected certificates"

	# Distrust, trust, and re-distrust one CA, and verify that it
	# ceases to appear, reappears, and again ceases to appear.
	# (This one has no subject hash collisions to worry about, so
	# we hard-code the `.0' suffix.)
	atf_check -s exit:0 $CERTCTL untrust "$diginotar"
	atf_check -s exit:0 test -e "untrusted/$diginotar_base"
	atf_check -s exit:0 test -h "untrusted/$diginotar_base"
	atf_check -s exit:0 test ! -e "certs/$diginotar_base"
	atf_check -s exit:0 test ! -h "certs/$diginotar_base"
	atf_check -s exit:0 test ! -e "certs/$diginotar_hash.0"
	atf_check -s exit:0 test ! -h "certs/$diginotar_hash.0"
	check_nonempty

	atf_check -s exit:0 $CERTCTL trust "$diginotar"
	atf_check -s exit:0 test ! -e "untrusted/$diginotar_base"
	atf_check -s exit:0 test ! -h "untrusted/$diginotar_base"
	atf_check -s exit:0 test -e "certs/$diginotar_base"
	atf_check -s exit:0 test -h "certs/$diginotar_base"
	atf_check -s exit:0 test -e "certs/$diginotar_hash.0"
	atf_check -s exit:0 test -h "certs/$diginotar_hash.0"
	rm "certs/$diginotar_base"
	rm "certs/$diginotar_hash.0"
	check_nonempty

	atf_check -s exit:0 $CERTCTL untrust "$diginotar"
	atf_check -s exit:0 test -e "untrusted/$diginotar_base"
	atf_check -s exit:0 test -h "untrusted/$diginotar_base"
	atf_check -s exit:0 test ! -e "certs/$diginotar_base"
	atf_check -s exit:0 test ! -h "certs/$diginotar_base"
	atf_check -s exit:0 test ! -e "certs/$diginotar_hash.0"
	atf_check -s exit:0 test ! -h "certs/$diginotar_hash.0"
	check_nonempty
}

atf_test_case empty
empty_head()
{
	atf_set "descr" "Test empty certificates store"
}
empty_body()
{
	setupconf		# no directories
	check_empty "empty cert path"
	atf_check -s exit:0 $CERTCTL -n rehash
	check_empty
	atf_check -s exit:0 $CERTCTL rehash
	atf_check -s exit:0 test -f certs/ca-certificates.crt
	atf_check -s exit:0 test \! -h certs/ca-certificates.crt
	atf_check -s exit:0 test \! -s certs/ca-certificates.crt
	atf_check -s exit:0 rm certs/ca-certificates.crt
	check_empty "empty cert path"
}

atf_test_case onedir
onedir_head()
{
	atf_set "descr" "Test one certificates directory"
}
onedir_body()
{
	setupconf certs1
	checks certs1
}

atf_test_case twodir
twodir_head()
{
	atf_set "descr" "Test two certificates directories"
}
twodir_body()
{
	setupconf certs1 certs2
	checks certs1 certs2
}

atf_test_case collidehash
collidehash_head()
{
	atf_set "descr" "Test colliding hashes"
}
collidehash_body()
{
	# certs3 has two certificates with the same subject hash
	setupconf certs1 certs3
	checks certs1 certs3
}

atf_test_case collidebase
collidebase_head()
{
	atf_set "descr" "Test colliding base names"
}
collidebase_body()
{
	# certs1 and certs4 both have DigiCert_Global_Root_CA.pem,
	# which should cause list and rehash to fail and mention
	# duplicates.
	setupconf certs1 certs4
	atf_check -s not-exit:0 -o ignore -e match:duplicate $CERTCTL list
	atf_check -s not-exit:0 -o ignore -e match:duplicate $CERTCTL rehash
}

atf_test_case manual
manual_head()
{
	atf_set "descr" "Test manual operation"
}
manual_body()
{
	local certs1 diginotar_base diginotar diginotar_hash

	certs1=$(atf_get_srcdir)/certs1
	diginotar_base=Explicitly_Distrust_DigiNotar_Root_CA.pem
	diginotar=$certs1/$diginotar_base
	diginotar_hash=$(openssl x509 -hash -noout <$diginotar)

	setupconf certs1 certs2
	cat <<EOF >>certs.conf
manual
EOF
	touch certs/bogus.pem
	ln -s bogus.pem certs/0123abcd.0

	# Listing shouldn't mention anything in the certs/ cache.
	atf_check -s exit:0 -o not-match:bogus $CERTCTL list
	atf_check -s exit:0 -o not-match:bogus $CERTCTL untrusted

	# Rehashing and changing the configuration should succeed, but
	# mention `manual' in a warning message and should not touch
	# the cache.
	atf_check -s exit:0 -e match:manual $CERTCTL rehash
	atf_check -s exit:0 -e match:manual $CERTCTL untrust "$diginotar"
	atf_check -s exit:0 -e match:manual $CERTCTL trust "$diginotar"

	# The files we created should still be there.
	atf_check -s exit:0 test -f certs/bogus.pem
	atf_check -s exit:0 test -h certs/0123abcd.0
}

atf_test_case evilcertsdir
evilcertsdir_head()
{
	atf_set "descr" "Test certificate directory with evil characters"
}
evilcertsdir_body()
{
	local certs1 diginotar_base diginotar evilcertsdir evildistrustdir

	certs1=$(atf_get_srcdir)/certs1
	diginotar_base=Explicitly_Distrust_DigiNotar_Root_CA.pem
	diginotar=$certs1/$diginotar_base

	evilcertsdir=$(printf '-evil certs\n.')
	evilcertsdir=${evilcertsdir%.}
	evildistrustdir=$(printf '-evil untrusted\n.')
	evildistrustdir=${evildistrustdir%.}

	setupconf certs1

	# initial (re)hash, nonexistent certs directory
	atf_check -s exit:0 $CERTCTL rehash
	atf_check -s exit:0 certctl -C certs.conf \
	    -c "$evilcertsdir" -u "$evildistrustdir" \
	    rehash
	atf_check -s exit:0 diff -ruN -- certs "$evilcertsdir"
	atf_check -s exit:0 test ! -e untrusted
	atf_check -s exit:0 test ! -h untrusted
	atf_check -s exit:0 test ! -e "$evildistrustdir"
	atf_check -s exit:0 test ! -h "$evildistrustdir"

	# initial (re)hash, empty certs directory
	atf_check -s exit:0 rm -rf -- certs
	atf_check -s exit:0 rm -rf -- "$evilcertsdir"
	atf_check -s exit:0 mkdir -- certs
	atf_check -s exit:0 mkdir -- "$evilcertsdir"
	atf_check -s exit:0 $CERTCTL rehash
	atf_check -s exit:0 certctl -C certs.conf \
	    -c "$evilcertsdir" -u "$evildistrustdir" \
	    rehash
	atf_check -s exit:0 diff -ruN -- certs "$evilcertsdir"
	atf_check -s exit:0 test ! -e untrusted
	atf_check -s exit:0 test ! -h untrusted
	atf_check -s exit:0 test ! -e "$evildistrustdir"
	atf_check -s exit:0 test ! -h "$evildistrustdir"

	# test distrusting a CA
	atf_check -s exit:0 $CERTCTL untrust "$diginotar"
	atf_check -s exit:0 certctl -C certs.conf \
	    -c "$evilcertsdir" -u "$evildistrustdir" \
	    untrust "$diginotar"
	atf_check -s exit:0 diff -ruN -- certs "$evilcertsdir"
	atf_check -s exit:0 diff -ruN -- untrusted "$evildistrustdir"

	# second rehash
	atf_check -s exit:0 $CERTCTL rehash
	atf_check -s exit:0 certctl -C certs.conf \
	    -c "$evilcertsdir" -u "$evildistrustdir" \
	    rehash
	atf_check -s exit:0 diff -ruN -- certs "$evilcertsdir"
	atf_check -s exit:0 diff -ruN -- untrusted "$evildistrustdir"
}

atf_test_case evilpath
evilpath_head()
{
	atf_set "descr" "Test certificate paths with evil characters"
}
evilpath_body()
{
	local evildir

	evildir=$(printf 'evil\n.')
	evildir=${evildir%.}
	mkdir "$evildir"

	cp -p "$(atf_get_srcdir)/certs2"/*.pem "$evildir"/

	setupconf certs1
	cat <<EOF >>certs.conf
path $(printf '%s' "$(pwd)/$evildir" | vis -M)
EOF
	checks certs1 "$(pwd)/$evildir"
}

atf_test_case missingconf
missingconf_head()
{
	atf_set "descr" "Test certctl with missing certs.conf"
}
missingconf_body()
{
	mkdir certs
	atf_check -s exit:0 test ! -e certs.conf
	atf_check -s not-exit:0 -e match:'certs\.conf' \
	    $CERTCTL rehash
}

atf_test_case nonexistentcertsdir
nonexistentcertsdir_head()
{
	atf_set "descr" "Test certctl succeeds when certsdir is nonexistent"
}
nonexistentcertsdir_body()
{
	setupconf certs1
	rmdir certs
	checks certs1
}

atf_test_case symlinkcertsdir
symlinkcertsdir_head()
{
	atf_set "descr" "Test certctl fails when certsdir is a symlink"
}
symlinkcertsdir_body()
{
	setupconf certs1
	rmdir certs
	mkdir empty
	ln -sfn empty certs

	atf_check -s not-exit:0 -e match:symlink $CERTCTL -n rehash
	atf_check -s not-exit:0 -e match:symlink $CERTCTL rehash
	atf_check -s exit:0 rmdir empty
}

atf_test_case regularfilecertsdir
regularfilecertsdir_head()
{
	atf_set "descr" "Test certctl fails when certsdir is a regular file"
}
regularfilecertsdir_body()
{
	setupconf certs1
	rmdir certs
	echo 'hello world' >certs

	atf_check -s not-exit:0 -e match:directory $CERTCTL -n rehash
	atf_check -s not-exit:0 -e match:directory $CERTCTL rehash
	atf_check -s exit:0 rm certs
}

atf_test_case prepopulatedcerts
prepopulatedcerts_head()
{
	atf_set "descr" "Test certctl fails when directory is prepopulated"
}
prepopulatedcerts_body()
{
	local cert certbase target

	setupconf certs1
	ln -sfn "$(atf_get_srcdir)/certs2"/*.pem certs/

	atf_check -s not-exit:0 -e match:manual $CERTCTL -n rehash
	atf_check -s not-exit:0 -e match:manual $CERTCTL rehash
	for cert in "$(atf_get_srcdir)/certs2"/*.pem; do
		certbase=$(basename "$cert")
		atf_check -s exit:0 -o inline:"$cert" \
		    readlink -n "certs/$certbase"
		rm "certs/$certbase"
	done
	check_empty
}

atf_init_test_cases()
{
	atf_add_test_case collidebase
	atf_add_test_case collidehash
	atf_add_test_case empty
	atf_add_test_case evilcertsdir
	atf_add_test_case evilpath
	atf_add_test_case manual
	atf_add_test_case missingconf
	atf_add_test_case nonexistentcertsdir
	atf_add_test_case onedir
	atf_add_test_case prepopulatedcerts
	atf_add_test_case regularfilecertsdir
	atf_add_test_case symlinkcertsdir
	atf_add_test_case twodir
}
