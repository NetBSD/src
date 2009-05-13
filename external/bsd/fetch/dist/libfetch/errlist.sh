#!/bin/sh
# $NetBSD: errlist.sh,v 1.1.1.2.6.1 2009/05/13 18:51:51 jym Exp $

printf "static struct fetcherr $1[] = {\n"
while read code type msg; do
	[ "${code}" = "#" ] && continue
	printf "\t{ ${code}, FETCH_${type}, \"${msg}\" },\n"
done < $3

printf "\t{ -1, FETCH_UNKNOWN, \"Unknown $2 error\" }\n"
printf "};\n"
