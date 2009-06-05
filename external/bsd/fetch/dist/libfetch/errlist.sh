#!/bin/sh
# $NetBSD: errlist.sh,v 1.1.1.4.4.2 2009/06/05 17:19:28 snj Exp $

printf "static struct fetcherr $1[] = {\n"
while read code type msg; do
	[ "${code}" = "#" ] && continue
	printf "\t{ ${code}, FETCH_${type}, \"${msg}\" },\n"
done < $3

printf "\t{ -1, FETCH_UNKNOWN, \"Unknown $2 error\" }\n"
printf "};\n"
