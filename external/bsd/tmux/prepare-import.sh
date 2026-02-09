#! /bin/sh
# $NetBSD: prepare-import.sh,v 1.7 2026/02/09 08:36:49 wiz Exp $
#
# Use this script to recreate the 'dist' subdirectory from a newly released
# distfile.  The script takes care of unpacking the distfile, removing any
# files that are not relevant to NetBSD and checking if there are any new
# files in the new release that need to be addressed.
#
# See the README file for general instructions.
#

set -e

ProgName=${0##*/}

err() {
	echo "${ProgName}:" "${@}" 1>&2
	exit 1
}

log() {
	echo "${ProgName}:" "${@}"
}

backup_dist() {
	if [ -d dist.old ]; then
		log "Removing dist; dist.old exists"
		rm -rf dist
	else
		log "Backing up dist as dist.old"
		mv dist dist.old
	fi
}

extract_distfile() {
	local distfile="${1}"; shift
	local distname="${1}"; shift

	log "Extracting ${distfile}"
	tar -xzf "${distfile}"
	[ -d "${distname}" ] || err "Distfile did not create ${distname}"
	log "Renaming ${distname} to dist"
	mv "${distname}" dist
}

get_distname() {
	local distfile="${1}"; shift
	basename "${distfile}" | sed -e 's,\.tar.*,,'
}

cleanup_dist() {
	log "Removing unnecessary files from dist"
	find dist -name .deps -exec rm -fr {} +
	find dist -name .dirstamp -exec rm -f {} +
	rm dist/cmd-parse.c
	rm dist/compat/asprintf.c
	rm dist/compat/bitstring.h
	rm dist/compat/closefrom.c
	rm dist/compat/daemon.c
	rm dist/compat/fgetln.c
	rm dist/compat/forkpty-aix.c
	rm dist/compat/forkpty-hpux.c
	rm dist/compat/forkpty-sunos.c
	rm dist/compat/getopt_long.c
	rm dist/compat/queue.h
	rm dist/compat/setenv.c
	rm dist/compat/strcasestr.c
	rm dist/compat/strlcat.c
	rm dist/compat/strlcpy.c
	rm dist/compat/strsep.c
	rm dist/compat/strtonum.c
	rm dist/compat/tree.h
	rm dist/compat/unvis.c
	rm dist/compat/vis.c
	rm dist/compat/vis.h
}

diff_dirs() {
	local old_dir="${1}"; shift
	local new_dir="${1}"; shift

	local old_list=$(mktemp -t tmux-import.XXXXXX)
	local new_list=$(mktemp -t tmux-import.XXXXXX)
	local diff=$(mktemp -t tmux-import.XXXXXX)
	trap "rm -f '${old_list}' '${new_list}' '${diff}'; exit 1" \
	    HUP INT QUIT TERM

	( cd "${old_dir}" && find . | sort >>"${old_list}" )
	( cd "${new_dir}" && find . | sort >>"${new_list}" )

	diff -u "${old_list}" "${new_list}" | grep '^+\.' >>"${diff}" || true
	if [ -s "${diff}" ]; then
		log "New files found"
		diff -u "${old_list}" "${new_list}" | grep '^+\.'
		log "Check if any files have to be cleaned up and update" \
		    "the prepare-import.sh script accordingly"
	else
		log "No new files; all good!"
	fi

	rm -f "${old_list}" "${new_list}" "${diff}"
}

main() {
	[ ${#} -eq 1 ] || err "Must provide a distfile name"
	local distfile="${1}"; shift

	[ -f Makefile -a -f prepare-import.sh ] || \
	    err "Must be run from the src/external/bsd/tmux subdirectory"

	local distname="$(get_distname ${distfile})"

	backup_dist
	extract_distfile "${distfile}" "${distname}"
	cleanup_dist
	diff_dirs dist.old dist
	cleantags dist
	log "Don't forget to update the -D flags in usr.bin/tmux/Makefile" \
	    "and to update the version in doc/3RDPARTY"
}

main "${@}"
