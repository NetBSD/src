#!/bin/sh
#	convert "jbsr" for external symbol to "bsr"
#				--written by Yasha
#
#	usage: nm foo.o | sh all_bsr.sh foo.s >foo_new.s

src="$1"
case "$src" in
'')
	echo 'Convert "jbsr" for external symbol to "bsr"'	>&2
	echo "usage: nm foo.o | $0 foo.s >foo_new.s"		>&2
	exit 1;;
esac

exts=
while read line
do
	set - $line
	case "$1" in
	U)	exts="$exts $2";;
	esac
done

sed '
/jbsr[ 	]/{
	h
	s/.*jbsr[ 	]*//
	s/$/:'"$exts"' /
	/^\([^:]*\):.* \1 /{
		g
		s/jbsr[	 ]/bsr /
		p
		d
	}
	g
}' "$src"
