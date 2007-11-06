#
# Usage: newvers.sh <historyrecord>
#
while read vers comment
do
	version=$vers
done < $1
developer=${USER:-"releng"}
[ -f /bin/hostname ] && buildhost=@`/bin/hostname`
date=`date`

cat <<EoF >vers.c
const char bootprog_rev[] = "$version";
const char bootprog_date[] = "$date";
const char bootprog_maker[] = "$developer$buildhost";
EoF
