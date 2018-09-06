#!/bin/sh
# Copyright (C) 2015  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Original script contributed by Jeffry A. Spain <spainj@countryday.net>

HELP="
Generates a set of <count> successive DNSSEC keys for <zone>
Key timings are based on a pre-publication rollover strategy

 <life>  (lifetime) is the key active lifetime in days [default 180]
 <intro> (introduction time) is the number of days from publication
         to activation of a key [default 30]
 <ret>   (retirement time) is the number of days from inactivation
         to deletion of a key [default 30]

Options:
 -a <alg>    Cryptographic algorithm. See man dnssec-keygen for defaults.
 -b <bits>   Number of bits in the key. See man dnssec-keygen for defaults.
 -k          if present, generate Key Signing Keys (KSKs). Otherwise,
             generate Zone Signing Keys (ZSKs).
 -3          If present and if -a is not specified, use an NSEC3-
             capable algorithm. See man dnssec-keygen for defaults.
 -i <date>   Inception date of the set of keys, in 'mm/dd/yyyy' format.
             The first two keys will be published by this date, and the
             first one will be activated. Default is today.
 -f <index>  Index of first key generated. Defaults to 0.
 -K <dir>    Key repository: write keys to this directory. Defaults to CWD. 
 -d          Dry run. No actual keys generated if present."

USAGE="Usage:
`basename $0` [-a <alg>] [-b <bits>] [-k] [-3] [-i <date>]
              [-f <index>] [-d] <zone> <count> [<life>] [<intro>] [<ret>]"

ALGFLAG=''
BITSFLAG=''
KSKFLAG=''
NSEC3FLAG=''
KEYREPO=''
DRYRUN=false
OPTKSK=false
K=0
INCEP=`date +%m/%d/%Y`

# Parse command line options
while getopts ":a:b:df:hkK:3i:" thisOpt
do
    case $thisOpt in
        a)
            ALGFLAG=" -a $OPTARG"
            ;;
        b)
            BITSFLAG=" -b $OPTARG"
            ;;
        d)
            DRYRUN=true
            ;;
        f)
            OPTKSK=true
            K=$OPTARG
            ;;
        h)
            echo "$USAGE"
            echo "$HELP"
            exit 0
            ;;
        k)
            KSKFLAG=" -f KSK"
            ;;
        K)
            KEYREPO=$OPTARG
            ;;
        3)
            NSEC3FLAG=" -3"
            ;;
        i)
            INCEP=$OPTARG
            ;;
        *)
            echo 'Unrecognized option.'
            echo "$USAGE"
            exit 1
            ;;
    esac
done
shift `expr $OPTIND - 1`

# Check that required arguments are present
if [ $# -gt 5 -o $# -lt 2 ]; then
    echo "$USAGE"
    exit 1
fi

# Remaining arguments:
# DNS zone name
ZONE=$1
shift

# Number of keys to be generated
COUNT=$1
shift

# Key active lifetime
LIFE=${1:-180}
[ $# -ne 0 ] && shift

# Key introduction time (publication to activation)
INTRO=${1:-30}
[ $# -ne 0 ] && shift

# Key retirement time (inactivation to deletion)
RET=${1:-30}

# Today's date in dnssec-keygen format (YYYYMMDD)
TODAY=`date +%Y%m%d`

# Key repository defaults to CWD
if [ -z "$KEYREPO" ]; then
    KEYREPO="."
fi

if $DRYRUN; then
    echo 'Dry Run (no key files generated)'
elif [ ! -d "$KEYREPO" ]; then
    # Create the key repository if it does not currently exist
    mkdir -p "$KEYREPO"
fi

# Iterate through the key set. K is the index, zero-based.
KLAST=`expr $K + $COUNT`
while [ $K -lt $KLAST ]; do
    KEYLABEL="Key `printf \"%02d\" $K`:"
    # Epoch of the current key
    # (zero for the first key, increments of key lifetime)
    # The epoch is in days relative to the inception date of the key set
    EPOCH=`expr $LIFE \* $K`
    # Activation date in days is the same as the epoch
    ACTIVATE=$EPOCH
    # Publication date in days relative to the key epoch
    PUBLISH=`expr $EPOCH - $LIFE - $INTRO`
    # Inactivation date in days relative to the key epoch
    INACTIVE=`expr $EPOCH + $LIFE`
    # Deletion date in days relative to the key epoch
    DELETE=`expr $EPOCH + $LIFE + $RET`

    # ... these values should not precede the key epoch
    [ $ACTIVATE -lt 0 ] && ACTIVATE=0
    [ $PUBLISH -lt 0 ] && PUBLISH=0
    [ $INACTIVE -lt 0 ] && INACTIVE=0
    [ $DELETE -lt 0 ] && DELETE=0

    # Key timing dates in dnssec-keygen format (YYYYMMDD):
    # publication, activation, inactivation, deletion
    PDATE=`date -d "$INCEP +$PUBLISH day" +%Y%m%d`
    ADATE=`date -d "$INCEP +$ACTIVATE day" +%Y%m%d`
    IDATE=`date -d "$INCEP +$INACTIVE day" +%Y%m%d`
    DDATE=`date -d "$INCEP +$DELETE day" +%Y%m%d`

    # Construct the dnssec-keygen command including all the specified options.
    # Suppress key generation progress information, and save the key in
    # the $KEYREPO directory.
    KEYGENCMD="dnssec-keygen -q$ALGFLAG$BITSFLAG$NSEC3FLAG$KSKFLAG -P $PDATE -A $ADATE -I $IDATE -D $DDATE -K $KEYREPO $ZONE"
    echo "$KEYLABEL $KEYGENCMD"

    # Generate the key and retrieve its name
    if $DRYRUN; then
        KEYNAME="DryRunKey-`printf \"%02d\" $K`"
    else
        KEYNAME=`$KEYGENCMD`
    fi

    # Indicate the key status based on key timing dates relative to today
    if [ $TODAY -ge $DDATE ]; then
        echo "$KEYLABEL $KEYNAME is obsolete post deletion date."
    elif [ $TODAY -ge $IDATE ]; then
        echo "$KEYLABEL $KEYNAME is published and inactive prior to deletion date."
    elif [ $TODAY -ge $ADATE ]; then
        echo "$KEYLABEL $KEYNAME is published and active."
    elif [ $TODAY -ge $PDATE ]; then
        echo "$KEYLABEL $KEYNAME is published prior to activation date."
    else
        echo "$KEYLABEL $KEYNAME is pending publication."
    fi

    # For published KSKs, generate the required DS records,
    # saving them to the file $KEYREPO/DS-$KEYNAME
    if $OPTKSK && [ $TODAY -ge $PDATE -a $TODAY -lt $DDATE ]; then
        echo "$KEYLABEL $KEYNAME (KSK) requires the publication of DS records in the parent zone."
        if $DRYRUN; then
            echo "$KEYLABEL No DS-$KEYNAME file created."
        else
            dnssec-dsfromkey "$KEYREPO/$KEYNAME" > "$KEYREPO/DS-$KEYNAME"
            echo "$KEYLABEL See $KEYREPO/DS-$KEYNAME."
        fi
    fi
    K=`expr $K + 1`
done

exit 0
