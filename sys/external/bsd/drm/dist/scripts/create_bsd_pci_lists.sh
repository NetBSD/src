#!/bin/sh 
#
# Script to output BSD compatible pci ids file
#  - Copyright Dave Airlie 2004 (airlied@linux.ie)
#
OUTFILE=drm_pciids.h

finished=0

cat > $OUTFILE <<EOF
/*
   This file is auto-generated from the drm_pciids.txt in the DRM CVS
   Please contact dri-devel@lists.sf.net to add new cards to this list
*/
EOF

while read pcivend pcidev attribs pciname
do
	if [ "x$pcivend" = "x" ]; then
		if [ "$finished" = "0" ]; then
			finished=1
			echo "	{0, 0, 0, NULL}" >> $OUTFILE
			echo >> $OUTFILE
		fi
	else
	
		cardtype=`echo "$pcivend" | cut -s -f2 -d'[' | cut -s -f1 -d']'`
		if [ "x$cardtype" = "x" ];
		then
			echo "	{$pcivend, $pcidev, $attribs, $pciname}, \\" >> $OUTFILE
		else
			echo "#define "$cardtype"_PCI_IDS \\" >> $OUTFILE
			finished=0
		fi
	fi
done

if [ "$finished" = "0" ]; then
	echo "	{0, 0, 0, NULL}" >> $OUTFILE
fi
