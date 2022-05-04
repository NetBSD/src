#!/bin/sh
#
# Copy files from an Elftoolchain checkout and prepare them for import
# into the NetBSD 'src' tree.
#
# Usage:
#
#   netbsd-base-system-import.sh [-D] -s SRCDIR -d DISTDIR -m MODULE \
#     [-m MODULE]...

usage() {
  echo "Usage: $0 [options]"
  echo
  echo "Prepare elftoolchain sources for importing into NetBSD src."
  echo
  echo "Options:"
  echo "  -D            Only show diffs."
  echo "  -d DISTDIR    Set the 'dist' directory for the elftoolchain import."
  echo "                Defaults to './dist'."
  echo "  -h            Display this help text."
  echo "  -m MODULE     A subdirectory of the elftoolchain tree to be"
  echo "                imported, e.g. 'libelf', 'common', 'libdwarf', etc."
  echo "  -s SRCDIR     The 'trunk' directory of an elftoolchain checkout."
  echo "  -v            Be verbose."
}

err() {
  echo ERROR: "$@" 1>&2
  echo
  usage
  exit 1
}

## Parse options.
diff_only=NO
verbose=NO
options=":d:hs:m:vD"
while getopts "$options" var; do
  case $var in
  d) dstdir="$OPTARG";;
  h) usage; exit 0;;
  m) modules="$OPTARG $modules";;
  s) srcdir="$OPTARG";;
  v) verbose=YES;;
  D) diff_only=YES;;
  '?') err "Unknown option: '-$OPTARG'.";;
  ':') err "Option '-$OPTARG' expects an argument.";;
  esac
  shift $((OPTIND - 1))
done

[ -n "${srcdir}" ] || err "Option -s must be specified."
[ -n "${modules}" ] || err "Option -m must be specified at least once."

if [ -z "${dstdir}" ]; then 
  dstdir="./dist"
fi

[ -d ${srcdir} ] || err "Missing source directory '$srcdir'."
[ -d ${dstdir} ] || err "Missing destination directory '$dstdir'."

# Verify that the source modules exist.
for m in ${modules}; do
  [ -d ${srcdir}/${m} ] || err "Missing source module '${srcdir}/${m}'"
done

## Helpers.
rename_svn_id() {
  sed -e '/\$Id:/ {
    s/\$Id:/Id:/;
    s/[ ]*\$//
  }'
}

handle_block_comment() {
  sed -e '/^\/\*-/ { i\
/*	\$NetBSD\$	*/\

}'
}

transform_placeholders() {
  sed -e \
'/@ELFTC-DECLARE-DOWNSTREAM-VCSID@/ {
c \
#if !defined(__RCSID)\
#define __RCSID(ID) /**/\
#endif  /* !defined(__RCSID) */
}' -e \
'/@ELFTC-DEFINE-ELFTC-VCSID@/ {
c \
#ifndef	ELFTC_VCSID\
#define	ELFTC_VCSID(ID)		/**/\
#endif
}' -e \
'/@ELFTC-USE-DOWNSTREAM-VCSID@/ {
c \
__RCSID("$NetBSD: netbsd-base-system-import.sh,v 1.1 2022/05/04 11:07:43 jkoshy Exp $");
}' -e \
'/@ELFTC-INCLUDE-SYS-CDEFS@/ {
c \
#include <sys/cdefs.h>
}'
}

# compare_and_move_or_diff filename generated_temp_file
compare_and_move_or_diff() {
  local dstfile=${dstdir}/${1}

  egrep -v '\$NetBSD.*\$' ${2}       > ${srccmptmp}
  egrep -v '\$NetBSD.*\$' ${dstfile} > ${dstcmptmp} 2> /dev/null

  if cmp -s ${srccmptmp} ${dstcmptmp}; then
    return 0
  fi

  if [ "${diff_only}" = YES ]; then
    # Show the changes needed to update the destination.
    if [ -f ${dstfile} ]; then
      diff -u ${dstfile} ${2}
    else
      echo '--- new file' ${file}
      diff -u /dev/null ${2}
    fi
  else
    mv ${2} ${dstfile} || exit ${?}
    changed_file="${1}"
  fi
}

# Manual pages need a CVS ID, and renaming of their SVN IDs.
handle_manual_page() {
  echo '.\"	$NetBSD: netbsd-base-system-import.sh,v 1.1 2022/05/04 11:07:43 jkoshy Exp $'         > ${srctmp}
  echo '.\"'                     >> ${srctmp}
  rename_svn_id < ${srcdir}/${1} >> ${srctmp}

  compare_and_move_or_diff ${1} ${srctmp}
}

# M4 files need a NetBSD RCS Id prepended, and any embedded
# VCS IDs transformed.
handle_m4_file() {
  echo 'dnl 	$NetBSD: netbsd-base-system-import.sh,v 1.1 2022/05/04 11:07:43 jkoshy Exp $'  > ${srctmp}
  transform_placeholders   <  ${srcdir}/${1} | \
    rename_svn_id         >> ${srctmp}

  compare_and_move_or_diff ${1} ${srctmp}
}

# Regular files only need their SVN IDs renamed.
handle_regular_file() {
  rename_svn_id < ${srcdir}/${1} > ${srctmp}

  compare_and_move_or_diff ${1} ${srctmp}
}

# C sources need a NetBSD RCS Id prepended, the
# ELFTC macros and SVN ids transformed.
handle_c_source() {
  handle_block_comment < ${srcdir}/${1} | \
    transform_placeholders | \
    rename_svn_id > ${srctmp}

  compare_and_move_or_diff ${1} ${srctmp}
}

# Prepare temporary files.
get_temporary_file() {
  mktemp -p ${TMPDIR:-/tmp} -t import-et.XXXXXX
}

srctmp=`get_temporary_file`
srccmptmp=`get_temporary_file`
dstcmptmp=`get_temporary_file`

trap "rm ${srctmp} ${srcmptmp} ${dstcmptmp};" 0 1 2 3 15

# For each module:
#  - Create new directories in the destination.
#  - For each file in the source directory.
#    - Transform the source file to its imported content.
#    - Ignore files that differ only in RCS 
#    - Display diffs or move changed files to the destination directory.

for m in ${modules}; do
  [ "$verbose" = YES ] && echo Examining module "'$m'".

  # Create any new directories under the destination root.
  (cd "${srcdir}" && find "${m}" -depth -type d) | \
    while read dir; do
      [ "${verbose}" = YES ] && echo "Creating '$dir'."
      mkdir -p "${dstdir}/${dir}"
    done

  # Import files, transforming them along the way.
  (cd "${srcdir}" && find "${m}" -depth -type f $pattern) | \
    egrep -v '.o$|.a$|.po$|.so$|.swp$|*~$' | \
    while read file; do
      changed_file=''  # Set by 'compare_and_move_or_diff'.

      if [ "${diff_only}" = NO ]; then
        [ "${verbose}" = YES ] && echo -n "Importing file '$file'"
      fi

      case "${file##*/}" in
        *.[0-9])
	  handle_manual_page "${file}"
	  ;;
        *.m4 )
	  handle_m4_file "${file}"
	  ;;
        Makefile | Version.map | *.m4 | *.mk) 
	  handle_regular_file "${file}"
	  ;;
        *.[ch])
 	  handle_c_source "${file}"
          ;;
        * ) error "Unsupported file: ${file}."
          ;;
      esac

      if [ "${diff_only}" = NO -a "${verbose}" = YES ]; then
	if [ -n "${changed_file}" ]; then
	  echo '- changed.'
	else
	  echo '- unchanged.'
	fi
      fi
    done
done
