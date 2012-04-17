#! /bin/sh
#
#	$NetBSD: makeerrnos.sh,v 1.5.20.1 2012/04/17 00:09:33 yamt Exp $

if [ $# -ne 3 ]; then
	echo "usage: makeerrnos.sh errno.h signal.h output"
	exit 1;
fi

ERRNOH=$1
SIGNALH=$2
CFILE=$3.c
HFILE=$3.h

: ${AWK:=awk}
: ${CPP:=cpp}
: ${CPPFLAGS:=}

cat <<__EOF__ > $CFILE
#include "misc.h"

struct systab errnos[] = {
__EOF__
cat ${ERRNOH} | ${CPP} ${CPPFLAGS} -dM |
${AWK} '
/^#[ 	]*define[ 	]*E[A-Z0-9]*[ 	]*[0-9-][0-9]*[ 	]*.*/ {
	for (i = 1; i <= NF; i++)
		if ($i ~ /define/) 
			break;
	i++;
	j = i + 1;
	#
	printf("\t{ \"%s\", %s },\n", $i, $j);
}
END {
	print "	{ \"0\", 0 },\n";
}
' | sort -n -k 3 >> $CFILE
echo "	{ 0L, 0},
};" >> $CFILE
lines=`wc -l $CFILE | ${AWK} ' { print $1; } ' -`
lines=`expr $lines - 4`

cat <<__EOF__ >> $CFILE

struct systab signals[] = {
__EOF__
cat ${SIGNALH} | ${CPP} ${CPPFLAGS} -dM |
${AWK} '
/^#[ 	]*define[ 	]*S[A-Z0-9]*[ 	]*[0-9-][0-9]*[ 	]*.*/ {
	for (i = 1; i <= NF; i++)
		if ($i ~ /define/) 
			break;
	i++;
	j = i + 1;
	#
	printf("\t{ \"%s\", %s },\n", $i, $j);
}
END {
	print "	{ \"0\", 0 },\n";
}
' | sort -n -k 3 >> $CFILE
echo "	{ 0L, 0},
};" >> $CFILE
elines=`grep '{ "SIG' $CFILE | wc -l`
elines=`expr $elines + 1`

cat <<__EOF__ >$HFILE
struct	systab	{
	const char	*name;
	int		value;
};

extern struct systab errnos[$lines + 1];
extern struct systab signals[$elines + 1];

#define	MAXERRNOS	$lines
#define	MAXSIGNALS	$elines
__EOF__
