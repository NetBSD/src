#!/bin/sh
# Filter to encapsulate text files in HP PJL format, suitable for use as an
# if filter for any printer supporting the HP PJL format, such as the
# HP LaserJet IIIsi
doescape () {
    printf "\33%%-12345X"
}
doescape
echo @PJL
echo @PJL JOB
cat
doescape
doescape
echo @PJL
echo @PJL EOJ
doescape
