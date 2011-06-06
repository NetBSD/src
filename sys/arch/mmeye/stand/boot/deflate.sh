#!/bin/sh

kernel=$1


CMF=0x78
FLG=0x9c
(echo $CMF && echo $FLG) | \
    ${TOOL_AWK} '{ printf "%c", int($0); }' > ${kernel}.gz

gzip -nc ${kernel} | dd bs=10 skip=1 of=${kernel}.gz.tmp
SIZE=`ls -l ${kernel}.gz.tmp | cut -d ' ' -f 8`
dd if=${kernel}.gz.tmp bs=`expr $SIZE - 8` count=1 >> ${kernel}.gz
rm ${kernel}.gz.tmp

# calculate adler-32
${TOOL_HEXDUMP} -v -e '1/1 "0x%02x\n"' ${kernel} | \
  ${TOOL_AWK} 'BEGIN { A = 1; B = 0 }
       {
	   A = (A + $0) % 65521;
	   B = (B + A) % 65521;
       }
       END { printf "%c%c%c%c", B / 256, B % 256, A / 256, A % 256; }' \
  >> ${kernel}.gz
