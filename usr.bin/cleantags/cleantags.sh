#!/bin/sh
# $NetBSD: cleantags.sh,v 1.2 2011/12/25 23:31:22 christos Exp $
# Remove the $'s from rcs tags

PROG="$(basename "$0")"
PAT='\$(Author|Date|CVSHeader|Header|Id|LocalId|Locker|Log|Name|RCSfile|Revision|Source|State|NetBSD)'
verbose=false

dosed() {
	sed \
	    -e 's/\$\(Author.*\)\$/\1/' \
	    -e 's/\$\(Date.*\)\$/\1/' \
	    -e 's/\$\(CVSHeader.*\)\$/\1/' \
	    -e 's/\$\(Header.*\)\$/\1/' \
	    -e 's/\$\(Id.*\)\$/\1/' \
	    -e 's/\$\(LocalId.*\)\$/\1/' \
	    -e 's/\$\(Locker.*\)\$/\1/' \
	    -e 's/\$\(Log.*\)\$/\1/' \
	    -e 's/\$\(Name.*\)\$/\1/' \
	    -e 's/\$\(RCSfile.*\)\$/\1/' \
	    -e 's/\$\(Revision.*\)\$/\1/' \
	    -e 's/\$\(Source.*\)\$/\1/' \
	    -e 's/\$\(State.*\)\$/\1/' \
	    -e 's/\$\(NetBSD.*\)\$/\1/' \
	    "$1" > "/tmp/$PROG$$" && mv "/tmp/$PROG$$" "$1"
	if $verbose
	then
		echo "$1"
	fi
}

usage() {
	echo "Usage: $PROG [-v] <files>|<directories>" 1>&2
	exit 1
}

while getopts "v" f
do
	case "$f" in
	v)
		verbose=true;;
	*)
		usage;;
	esac
done

shift "$(expr "$OPTIND" - 1)"

if [ -z "$1" ]
then
	usage
fi

for i
do
	if [ -d "$i" ]
	then
		find "$i" -type f -print0 | xargs -0 egrep -l "$PAT" |
		while read f
		do
			dosed "$f"
		done
	elif egrep -qs "$PAT" "$i" 
	then
		dosed "$i"
	fi
done
