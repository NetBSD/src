#
# Usage: newvers.sh <historyrecord>
#
while read vers comment
do
	version=$vers
done < $1

cat <<EoF >vers.c
const char bootprog_rev[] = "$version";
EoF
