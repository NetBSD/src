#!/bin/sh

#	$NetBSD: certctl.sh,v 1.4.2.3 2023/09/06 15:04:33 martin Exp $
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

set -o pipefail
set -Ceu

progname=$(basename -- "$0")

### Options and arguments

usage()
{
	exec >&2
	printf 'Usage: %s %s\n' \
	    "$progname" \
	    "[-nv] [-C <config>] [-c <certsdir>] [-u <untrusted>]"
	printf '               <cmd> <args>...\n'
	printf '       %s list\n' "$progname"
	printf '       %s rehash\n' "$progname"
	printf '       %s trust <cert>\n' "$progname"
	printf '       %s untrust <cert>\n' "$progname"
	printf '       %s untrusted\n' "$progname"
	exit 1
}

certsdir=/etc/openssl/certs
config=/etc/openssl/certs.conf
distrustdir=/etc/openssl/untrusted
nflag=false			# dry run
vflag=false			# verbose

# Options used by FreeBSD:
#
#	-D destdir
#	-M metalog
#	-U		(unprivileged)
#	-d distbase
#
while getopts C:c:nu:v f; do
	case $f in
	C)	config=$OPTARG;;
	c)	certsdir=$OPTARG;;
	n)	nflag=true;;
	u)	distrustdir=$OPTARG;;
	v)	vflag=true;;
	\?)	usage;;
	esac
done
shift $((OPTIND - 1))

if [ $# -lt 1 ]; then
	usage
fi
cmd=$1

### Global state

config_paths=
config_manual=false
tmpfile=

# If tmpfile is set to nonempty, clean it up on exit.

trap 'test -n "$tmpfile" && rm -f "$tmpfile"' EXIT HUP INT TERM

### Subroutines

# error <msg> ...
#
#	Print an error message to stderr.
#
#	Does not exit the process.
#
error()
{
	echo "$progname:" "$@" >&2
}

# run <cmd> <args>...
#
#	Print a command if verbose, and run it unless it's a dry run.
#
run()
{
	local t q cmdline

	if $vflag; then	       # print command if verbose
		for t; do
			case $t in
			''|*[^[:alnum:]+,-./:=_@]*)
				# empty or unsafe -- quotify
				;;
			*)
				# nonempty and safe-only -- no quotify
				cmdline="${cmdline:+$cmdline }$t"
				continue
				;;
			esac
			q=$(printf '%s' "$t" | sed -e "s/'/'\\\''/g'")
			cmdline="${cmdline:+$cmdline }'$q'"
		done
		printf '%s\n' "$cmdline"
	fi
	if ! $nflag; then	# skip command if dry run
		"$@"
	fi
}

# configure
#
#	Parse the configuration file, initializing config_*.
#
configure()
{
	local lineno status formatok vconfig line contline op path vpath vop

	# Count line numbers, record a persistent error status to
	# return at the end, and record whether we got a format line.
	lineno=0
	status=0
	formatok=false

	# vis the config name for terminal-safe error messages.
	vconfig=$(printf '%s' "$config" | vis -M)

	# Read and process each line of the config file.
	while read -r line; do
		lineno=$((lineno + 1))

		# If the line ends in an odd number of backslashes, it
		# has a continuation line, so read on.
		while expr "$line" : '^\(\\\\\)*\\' >/dev/null ||
		    expr "$line" : '^.*[^\\]\(\\\\\)*\\$' >/dev/null; do
			if ! read -r contline; then
				error "$vconfig:$lineno: premature end of file"
				return 1
			fi
			line="$line$contline"
		done

		# Skip blank lines and comments.
		case $line in
		''|'#'*)
			continue
			;;
		esac

		# Require the first non-blank/comment line to identify
		# the config file format.
		if ! $formatok; then
			if [ "$line" = "netbsd-certctl 20230816" ]; then
				formatok=true
				continue
			else
				error "$vconfig:$lineno: missing format line"
				status=1
				break
			fi
		fi

		# Split the line into words and dispatch on the first.
		set -- $line
		op=$1
		case $op in
		manual)
			config_manual=true
			;;
		path)
			if [ $# -lt 2 ]; then
				error "$vconfig:$lineno: missing path"
				status=1
				continue
			fi
			if [ $# -gt 3 ]; then
				error "$vconfig:$lineno: excess args"
				status=1
				continue
			fi

			# Unvis the path.  Hack: if the user has had
			# the audacity to choose a path ending in
			# newlines, prevent the shell from consuming
			# them so we don't choke on their subterfuge.
			path=$(printf '%s.' "$2" | unvis)
			path=${path%.}

			# Ensure the path is absolute.  It is unclear
			# what directory it should be relative to if
			# not.
			case $path in
			/*)
				;;
			*)
				error "$vconfig:$lineno:" \
				    "relative path forbidden"
				status=1
				continue
				;;
			esac

			# Record the vis-encoded path in a
			# space-separated list.
			vpath=$(printf '%s' "$path" | vis -M)
			config_paths="$config_paths $vpath"
			;;
		*)
			vop=$(printf '%s' "$op" | vis -M)
			error "$vconfig:$lineno: unknown command: $vop"
			;;
		esac
	done <$config || status=$?

	return $status
}

# list_default_trusted
#
#	List the vis-encoded certificate paths and their base names,
#	separated by a space, for the certificates that are trusted by
#	default according to the configuration.
#
#	No order guaranteed; caller must sort.
#
list_default_trusted()
{
	local vpath path cert base vcert vbase

	for vpath in $config_paths; do
		path=$(printf '%s.' "$vpath" | unvis)
		path=${path%.}

		# Enumerate the .pem, .cer, and .crt files.
		for cert in "$path"/*.pem "$path"/*.cer "$path"/*.crt; do
			# vis the certificate path.
			vcert=$(printf '%s' "$cert" | vis -M)

			# If the file doesn't exist, then either:
			#
			# (a) it's a broken symlink, so fail;
			# or
			# (b) the shell glob failed to match,
			#     so ignore it and move on.
			if [ ! -e "$cert" ]; then
				if [ -h "$cert" ]; then
					error "broken symlink: $vcert"
					status=1
				fi
				continue
			fi

			# Print the vis-encoded absolute path to the
			# certificate and base name on a single line.
			vbase=$(basename -- "$vcert.")
			vbase=${vbase%.}
			printf '%s %s\n' "$vcert" "$vbase"
		done
	done
}

# list_distrusted
#
#	List the vis-encoded certificate paths and their base names,
#	separated by a space, for the certificates that have been
#	distrusted by the user.
#
#	No order guaranteed; caller must sort.
#
list_distrusted()
{
	local status link vlink cert vcert

	status=0

	for link in "$distrustdir"/*; do
		# vis the link for terminal-safe error messages.
		vlink=$(printf '%s' "$link" | vis -M)

		# The distrust directory must only have symlinks to
		# certificates.  If we find a non-symlink, print a
		# warning and arrange to fail.
		if [ ! -h "$link" ]; then
			if [ ! -e "$link" ] && \
			    [ "$link" = "$distrustdir/*" ]; then
				# Shell glob matched nothing -- just
				# ignore it.
				break
			fi
			error "distrusted non-symlink: $vlink"
			status=1
			continue
		fi

		# Read the target of the symlink, nonrecursively.  If
		# the user has had the audacity to make a symlink whose
		# target ends in newline, prevent the shell from
		# consuming them so we don't choke on their subterfuge.
		cert=$(readlink -n -- "$link" && printf .)
		cert=${cert%.}

		# Warn if the target is relative.  Although it is clear
		# what directory it would be relative to, there might
		# be issues with canonicalization.
		case $cert in
		/*)
			;;
		*)
			vlink=$(printf '%s' "$link" | vis -M)
			vcert=$(printf '%s' "$cert" | vis -M)
			error "distrusted relative symlink: $vlink -> $vcert"
			;;
		esac

		# Print the vis-encoded absolute path to the
		# certificate and base name on a single line.
		vcert=$(printf '%s' "$cert" | vis -M)
		vbase=$(basename -- "$vcert.")
		vbase=${vbase%.}
		printf '%s %s\n' "$vcert" "$vbase"
	done

	return $status
}

# list_trusted
#
#	List the trusted certificates, excluding the distrusted one, as
#	one vis(3) line per certificate.  Reject duplicate base names,
#	since we will be creating symlinks to the same base names in
#	the certsdir.  Sorted lexicographically by vis-encoding.
#
list_trusted()
{

	# XXX Use dev/ino to match files instead of symlink targets?

	{
		list_default_trusted \
		| while read -r vcert vbase; do
			printf 'trust %s %s\n' "$vcert" "$vbase"
		done

		# XXX Find a good way to list the default-untrusted
		# certificates, so if you have already distrusted one
		# and it is removed from default-trust on update,
		# nothing warns about this.

		# list_default_untrusted \
		# | while read -r vcert vbase; do
		# 	printf 'distrust %s %s\n' "$vcert" "$vbase"
		# done

		list_distrusted \
		| while read -r vcert vbase; do
			printf 'distrust %s %s\n' "$vcert" "$vbase"
		done
	} | awk -v progname="$progname" '
		BEGIN			{ status = 0 }
		$1 == "trust" && $3 in trust && $2 != trust[$3] {
			printf "%s: duplicate base name %s\n  %s\n  %s\n", \
			    progname, $3, trust[$3], $2 >"/dev/stderr"
			status = 1
			next
		}
		$1 == "trust"		{ trust[$3] = $2 }
		$1 == "distrust" && !trust[$3] && !distrust[$3] {
			printf "%s: distrusted certificate not found: %s\n", \
			    progname, $3 >"/dev/stderr"
			status = 1
		}
		$1 == "distrust" && $2 in trust && $2 != trust[$3] {
			printf "%s: distrusted certificate %s" \
			    " has multiple paths\n" \
			    "  %s\n  %s\n",
			    progname, $3, trust[$3], $2 >"/dev/stderr"
			status = 1
		}
		$1 == "distrust"	{ distrust[$3] = 1 }
		END			{
			for (vbase in trust) {
				if (!distrust[vbase])
					print trust[vbase]
			}
			exit status
		}
	' | sort -u
}

# rehash
#
#	Delete and rebuild certsdir.
#
rehash()
{
	local vcert cert certbase hash counter bundle vbundle

	# If manual operation is enabled, refuse to rehash the
	# certsdir, but succeed anyway so this can safely be used in
	# automated scripts.
	if $config_manual; then
		error "manual certificates enabled, not rehashing"
		return
	fi

	# Delete the active certificates symlink cache, if either it is
	# empty or nonexistent, or it is tagged for use by certctl.
	if [ -f "$certsdir/.certctl" ]; then
		# Directory exists and is managed by certctl(8).
		# Safe to delete it and everything in it.
		run rm -rf -- "$certsdir"
	elif [ -h "$certsdir" ]; then
		# Paranoia: refuse to chase a symlink.  (Caveat: this
		# is not secure against an adversary who can recreate
		# the symlink at any time.  Just a helpful check for
		# mistakes.)
		error "certificates directory is a symlink"
		return 1
	elif [ ! -e "$certsdir" ]; then
		# Directory doesn't exist at all.  Nothing to do!
	elif [ ! -d "$certsdir" ]; then
		error "certificates directory is not a directory"
		return 1
	elif ! find -f "$certsdir" -- -maxdepth 0 -type d -empty -exit 1; then
		# certsdir exists, is a directory, and is empty.  Safe
		# to delete it with rmdir and take it over.
		run rmdir -- "$certsdir"
	else
		error "existing certificates; set manual or move them"
		return 1
	fi
	run mkdir -- "$certsdir"
	if $vflag; then
		printf '# initialize %s\n' "$certsdir"
	fi
	if ! $nflag; then
		printf 'This directory is managed by certctl(8).\n' \
		    >$certsdir/.certctl
	fi

	# Create a temporary file for the single-file bundle.  This
	# will be automatically deleted on normal exit or
	# SIGHUP/SIGINT/SIGTERM.
	if ! $nflag; then
		tmpfile=$(mktemp -t "$progname.XXXXXX")
	fi

	# Recreate symlinks for all of the trusted certificates.
	list_trusted \
	| while read -r vcert; do
		cert=$(printf '%s.' "$vcert" | unvis)
		cert=${cert%.}
		run ln -s -- "$cert" "$certsdir"

		# Add the certificate to the single-file bundle.
		if ! $nflag; then
			cat -- "$cert" >>$tmpfile
		fi
	done

	# Hash the directory with openssl.
	#
	# XXX Pass `-v' to openssl in a way that doesn't mix with our
	# shell-safe verbose commands?  (Need to handle `-n' too.)
	run openssl rehash -- "$certsdir"

	# Install the single-file bundle.
	bundle=$certsdir/ca-certificates.crt
	vbundle=$(printf '%s' "$bundle" | vis -M)
	$vflag && printf '# create %s\n' "$vbundle"
	if ! $nflag; then
		(umask 0022; cat <$tmpfile >${bundle}.tmp)
		mv -f -- "${bundle}.tmp" "$bundle"
		rm -f -- "$tmpfile"
		tmpfile=
	fi
}

### Commands

usage_list()
{
	exec >&2
	printf 'Usage: %s list\n' "$progname"
	exit 1
}
cmd_list()
{
	test $# -eq 1 || usage_list

	configure

	list_trusted \
	| while read -r vcert vbase; do
		printf '%s\n' "$vcert"
	done
}

usage_rehash()
{
	exec >&2
	printf 'Usage: %s rehash\n' "$progname"
	exit 1
}
cmd_rehash()
{
	test $# -eq 1 || usage_rehash

	configure

	rehash
}

usage_trust()
{
	exec >&2
	printf 'Usage: %s trust <cert>\n' "$progname"
	exit 1
}
cmd_trust()
{
	local cert vcert certbase vcertbase

	test $# -eq 2 || usage_trust
	cert=$2

	configure

	# XXX Accept base name.

	# vis the certificate path for terminal-safe error messages.
	vcert=$(printf '%s' "$cert" | vis -M)

	# Verify the certificate actually exists.
	if [ ! -f "$cert" ]; then
		error "no such certificate: $vcert"
		return 1
	fi

	# Verify we currently distrust a certificate by this base name.
	certbase=$(basename -- "$cert.")
	certbase=${certbase%.}
	if [ ! -h "$distrustdir/$certbase" ]; then
		error "not currently distrusted: $vcert"
		return 1
	fi

	# Verify the certificate we distrust by this base name is the
	# same one.
	target=$(readlink -n -- "$distrustdir/$certbase" && printf .)
	target=${target%.}
	if [ "$cert" != "$target" ]; then
		vcertbase=$(basename -- "$vcert")
		error "distrusted $vcertbase does not point to $vcert"
		return 1
	fi

	# Remove the link from the distrusted directory, and rehash --
	# quietly, so verbose output emphasizes the distrust part and
	# not the whole certificate set.
	run rm -- "$distrustdir/$certbase"
	$vflag && echo '# rehash'
	vflag=false
	rehash
}

usage_untrust()
{
	exec >&2
	printf 'Usage: %s untrust <cert>\n' "$progname"
	exit 1
}
cmd_untrust()
{
	local cert vcert certbase vcertbase target vtarget

	test $# -eq 2 || usage_untrust
	cert=$2

	configure

	# vis the certificate path for terminal-safe error messages.
	vcert=$(printf '%s' "$cert" | vis -M)

	# Verify the certificate actually exists.  Otherwise, you might
	# fail to distrust a certificate you intended to distrust,
	# e.g. if you made a typo in its path.
	if [ ! -f "$cert" ]; then
		error "no such certificate: $vcert"
		return 1
	fi

	# Check whether this certificate is already distrusted.
	# - If the same base name points to the same path, stop here.
	# - Otherwise, fail noisily.
	certbase=$(basename "$cert.")
	certbase=${certbase%.}
	if [ -h "$distrustdir/$certbase" ]; then
		target=$(readlink -n -- "$distrustdir/$certbase" && printf .)
		target=${target%.}
		if [ "$target" = "$cert" ]; then
			$vflag && echo '# already distrusted'
			return
		fi
		vcertbase=$(printf '%s' "$certbase" | vis -M)
		vtarget=$(printf '%s' "$target" | vis -M)
		error "distrusted $vcertbase at different path $vtarget"
		return 1
	fi

	# Create the distrustdir if needed, create a symlink in it, and
	# rehash -- quietly, so verbose output emphasizes the distrust
	# part and not the whole certificate set.
	test -d "$distrustdir" || run mkdir -- "$distrustdir"
	run ln -s -- "$cert" "$distrustdir"
	$vflag && echo '# rehash'
	vflag=false
	rehash
}

usage_untrusted()
{
	exec >&2
	printf 'Usage: %s untrusted\n' "$progname"
	exit 1
}
cmd_untrusted()
{
	test $# -eq 1 || usage_untrusted

	configure

	list_distrusted \
	| while read -r vcert vbase; do
		printf '%s\n' "$vcert"
	done
}

### Main

# We accept the following aliases for user interface compatibility with
# FreeBSD:
#
#	blacklist = untrust
#	blacklisted = untrusted
#	unblacklist = trust

case $cmd in
list)	cmd_list "$@"
	;;
rehash)	cmd_rehash "$@"
	;;
trust|unblacklist)
	cmd_trust "$@"
	;;
untrust|blacklist)
	cmd_untrust "$@"
	;;
untrusted|blacklisted)
	cmd_untrusted "$@"
	;;
*)	vcmd=$(printf '%s' "$cmd" | vis -M)
	printf '%s: unknown command: %s\n' "$progname" "$vcmd" >&2
	usage
	;;
esac
