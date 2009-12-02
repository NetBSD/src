#!/bin/sh

# Copyright (C) 2009 Chris Procter All rights reserved.
# Copyright (C) 2009 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# vgimportclone: This script is used to rename the VG and change the associated
#                VG and PV UUIDs (primary application being HW snapshot restore)

# following external commands are used throughout the script
# echo and test are internal in bash at least
RM=rm
BASENAME=basename
MKTEMP=mktemp
AWK=awk
CUT=cut
TR=tr
READLINK=readlink
GREP=grep
GETOPT=getopt

# user may override lvm location by setting LVM_BINARY
LVM="${LVM_BINARY:-lvm}"

die() {
    code=$1; shift
    echo "Fatal: $@" 1>&2
    exit $code
}

"$LVM" version >& /dev/null || die 2 "Could not run lvm binary '$LVM'"


function getvgname {
### get a unique vg name
###        $1 = list of exists VGs
###        $2 = the name we want
    VGLIST=$1
    VG=$2
    NEWVG=$3

    BNAME="${NEWVG:-${VG}}"
    NAME="${BNAME}"
    I=0

    while [[ "${VGLIST}" =~ "${NAME}" ]]
    do
        I=$(($I+1))
        NAME="${BNAME}$I"
    done
    echo "${NAME}"
}


function checkvalue {
### check return value and error if non zero
    if [ $1 -ne 0 ]
    then
        die $1 "$2, error: $1"
    fi
}


function usage {
### display usage message
    echo "Usage: ${SCRIPTNAME} [options] PhysicalVolume [PhysicalVolume...]"
    echo "    -n|--basevgname - Base name for the new volume group(s)"
    echo "    -i|--import     - Import any exported volume groups found"
    echo "    -t|--test       - Run in test mode"
    echo "    --quiet         - Suppress output"
    echo "    -v|--verbose    - Set verbose level"
    echo "    -d|--debug      - Set debug level"
    echo "    --version       - Display version information"
    echo "    -h|--help       - Display this help message"
    echo ""
    exit 1
}


function cleanup {
    #set to use old lvm.conf
    LVM_SYSTEM_DIR=${ORIG_LVM_SYS_DIR}

    if [ $KEEP_TMP_LVM_SYSTEM_DIR -eq 1 ]; then
        echo "${SCRIPTNAME}: LVM_SYSTEM_DIR (${TMP_LVM_SYSTEM_DIR}) must be cleaned up manually."
    else
        "$RM" -rf -- "${TMP_LVM_SYSTEM_DIR}"
    fi
}

SCRIPTNAME=`"$BASENAME" $0`


if [ "$UID" != "0" -a "$EUID" != "0" ]
then
    die 3 "${SCRIPTNAME} must be run as root."
fi

LVM_OPTS=""
TEST_OPT=""
DISKS=""
# for compatibility: using mktemp -t rather than --tmpdir
TMP_LVM_SYSTEM_DIR=`"$MKTEMP" -d -t snap.XXXXXXXX`
KEEP_TMP_LVM_SYSTEM_DIR=0
CHANGES_MADE=0
IMPORT=0
DEBUG=""
VERBOSE=""
VERBOSE_COUNT=0
DEVNO=0

if [ -n "${LVM_SYSTEM_DIR}" ]; then
    export ORIG_LVM_SYS_DIR="${LVM_SYSTEM_DIR}"
fi

trap  cleanup 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18

#####################################################################
### Get and check arguments
#####################################################################
OPTIONS=`"$GETOPT" -o n:dhitv \
    -l basevgname:,debug,help,import,quiet,test,verbose,version \
    -n "${SCRIPTNAME}" -- "$@"`
[ $? -ne 0 ] && usage
eval set -- "$OPTIONS"

while true
do
    case $1 in
        -n|--basevgname)
            NEWVG="$2"; shift; shift
            ;;
        -i|--import)
            IMPORT=1; shift
            ;;
        -t|--test)
            TEST_OPT="-t"
            shift
            ;;
        --quiet)
            LVM_OPTS="--quiet ${LVM_OPTS}"
            shift
            ;;
        -v|--verbose)
            let VERBOSE_COUNT=VERBOSE_COUNT+1
            if [ -z "$VERBOSE" ]
            then
                VERBOSE="-v"
            else
                VERBOSE="${VERBOSE}v"
            fi
            shift
            ;;
        -d|--debug)
            if [ -z "$DEBUG" ]
            then
                DEBUG="-d"
                set -x
            else
                DEBUG="${DEBUG}d"
            fi
            shift
            ;;
        --version)
            "$LVM" version
            shift
            exit 0
            ;;
        -h|--help)
            usage; shift
            ;;
        --)
            shift; break
            ;;
        *)
            usage
            ;;
    esac
done

# turn on DEBUG (special case associated with -v use)
if [ -z "$DEBUG" -a $VERBOSE_COUNT -gt 3 ]; then
    DEBUG="-d"
    set -x
fi

# setup LVM_OPTS
if [ -n "${DEBUG}" -o -n "${VERBOSE}" ]
then
    LVM_OPTS="${LVM_OPTS} ${DEBUG} ${VERBOSE}"
fi

# process remaining arguments (which should be disks)
for ARG
do
    if [ -b "$ARG" ]
    then
        PVS_OUT=`"${LVM}" pvs ${LVM_OPTS} --noheadings -o vg_name "$ARG" 2>/dev/null`
        checkvalue $? "$ARG is not a PV."
        PV_VGNAME=$(echo $PVS_OUT | $GREP -v '[[:space:]]+$')
        [ -z "$PV_VGNAME" ] && die 3 "$ARG is not in a VG."

        ln -s "$ARG" ${TMP_LVM_SYSTEM_DIR}/vgimport${DEVNO}
        DISKS="${DISKS} ${TMP_LVM_SYSTEM_DIR}/vgimport${DEVNO}"
        DEVNO=$((${DEVNO}+1))
    else
        die 3 "$ARG is not a block device."
    fi
done

### check we have suitable values for important variables
if [ -z "${DISKS}" ]
then
    usage
fi

#####################################################################
### Get the existing state so we can use it later
#####################################################################

OLDVGS=`"${LVM}" vgs ${LVM_OPTS} -o name --noheadings 2>/dev/null`
checkvalue $? "Current VG names could not be collected without errors"

#####################################################################
### Prepare the temporary lvm environment
#####################################################################

for BLOCK in ${DISKS}
do
    FILTER="\"a|^${BLOCK}$|\", ${FILTER}"
done
export FILTER="filter=[ ${FILTER} \"r|.*|\" ]"

LVMCONF=${TMP_LVM_SYSTEM_DIR}/lvm.conf

"$LVM" dumpconfig ${LVM_OPTS} | \
"$AWK" -v DEV=${TMP_LVM_SYSTEM_DIR} -v CACHE=${TMP_LVM_SYSTEM_DIR}/.cache \
    -v CACHE_DIR=${TMP_LVM_SYSTEM_DIR}/cache \
    '/^[[:space:]]*filter[[:space:]]*=/{print ENVIRON["FILTER"];next} \
     /^[[:space:]]*scan[[:space:]]*=/{print "scan = [ \"" DEV "\" ]";next} \
     /^[[:space:]]*cache[[:space:]]*=/{print "cache = \"" CACHE "\"";next} \
     /^[[:space:]]*cache_dir[[:space:]]*=/{print "cache_dir = \"" CACHE_DIR "\"";next} \
     {print $0}' > ${LVMCONF}

checkvalue $? "Failed to generate ${LVMCONF}"
# Only keep TMP_LVM_SYSTEM_DIR if it contains something worth keeping
[ -n "${DEBUG}" ] && KEEP_TMP_LVM_SYSTEM_DIR=1

# verify the config contains the filter, scan and cache_dir (or cache) config keywords
"$GREP" -q '^[[:space:]]*filter[[:space:]]*=' ${LVMCONF} || \
    die 5 "Temporary lvm.conf must contain 'filter' config."
"$GREP" -q '^[[:space:]]*scan[[:space:]]*=' ${LVMCONF} || \
    die 6 "Temporary lvm.conf must contain 'scan' config."

# check for either 'cache' or 'cache_dir' config values
"$GREP" -q '[[:space:]]*cache[[:space:]]*=' ${LVMCONF}
CACHE_RET=$?
"$GREP" -q '^[[:space:]]*cache_dir' ${LVMCONF}
CACHE_DIR_RET=$?
[ $CACHE_RET -eq 0 -o $CACHE_DIR_RET -eq 0 ] || \
    die 7 "Temporary lvm.conf must contain 'cache' or 'cache_dir' config."

### set to use new lvm.conf
export LVM_SYSTEM_DIR=${TMP_LVM_SYSTEM_DIR}


#####################################################################
### Rename the VG(s) and change the VG and PV UUIDs.
#####################################################################

PVINFO=`"${LVM}" pvs ${LVM_OPTS} -o pv_name,vg_name,vg_attr --noheadings --separator : 2>/dev/null`
checkvalue $? "PV info could not be collected without errors"

# output VG info so each line looks like: name:exported?:disk1,disk2,...
VGINFO=`echo "${PVINFO}" | \
    "$AWK" -F : '{{sub(/^[[:space:]]*/,"")} \
    {sub(/unknown device/,"unknown_device")} \
    {vg[$2]=$1","vg[$2]} if($3 ~ /^..x/){x[$2]="x"}} \
    END{for(k in vg){printf("%s:%s:%s\n", k, x[k], vg[k])}}'`
checkvalue $? "PV info could not be parsed without errors"

for VG in ${VGINFO}
do
    VGNAME=`echo "${VG}" | "$CUT" -d: -f1`
    EXPORTED=`echo "${VG}" | "$CUT" -d: -f2`
    PVLIST=`echo "${VG}" | "$CUT" -d: -f3- | "$TR" , ' '`

    if [ -z "${VGNAME}" ]
    then
        FOLLOWLIST=""
        for DEV in $PVLIST; do
            FOLLOW=`"$READLINK" $DEV`
            FOLLOWLIST="$FOLLOW $FOLLOWLIST"
        done
        die 8 "Specified PV(s) ($FOLLOWLIST) don't belong to a VG."
    fi

    if [ -n "${EXPORTED}" ]
    then
        if [ ${IMPORT} -eq 1 ]
        then
            "$LVM" vgimport ${LVM_OPTS} ${TEST_OPT} "${VGNAME}"
            checkvalue $? "Volume Group ${VGNAME} could not be imported"
        else
            echo "Volume Group ${VGNAME} exported, skipping."
            continue
        fi
    fi

    ### change the pv uuids
    if [[ "${PVLIST}" =~ "unknown" ]]
    then
        echo "Volume Group ${VGNAME} has unknown PV(s), skipping."
        echo "- Were all associated PV(s) supplied as arguments?"
        continue
    fi

    for BLOCKDEV in ${PVLIST}
    do
        "$LVM" pvchange ${LVM_OPTS} ${TEST_OPT} --uuid ${BLOCKDEV} --config 'global{activation=0}'
        checkvalue $? "Unable to change PV uuid for ${BLOCKDEV}"
    done

    NEWVGNAME=`getvgname "${OLDVGS}" "${VGNAME}" "${NEWVG}"`

    "$LVM" vgchange ${LVM_OPTS} ${TEST_OPT} --uuid "${VGNAME}" --config 'global{activation=0}'
    checkvalue $? "Unable to change VG uuid for ${VGNAME}"

    ## if the name isn't going to get changed dont even try.
    if [ "${VGNAME}" != "${NEWVGNAME}" ]
    then
        "$LVM" vgrename ${LVM_OPTS} ${TEST_OPT} "${VGNAME}" "${NEWVGNAME}"
        checkvalue $? "Unable to rename ${VGNAME} to ${NEWVGNAME}"
    fi

    CHANGES_MADE=1
done

#####################################################################
### Restore the old environment
#####################################################################
### set to use old lvm.conf
if [ -z "${ORIG_LVM_SYS_DIR}" ]
then
    unset LVM_SYSTEM_DIR
else
    LVM_SYSTEM_DIR=${ORIG_LVM_SYS_DIR}
fi

### update the device cache and make sure all
### the device nodes we need are straight
if [ ${CHANGES_MADE} -eq 1 ]
then
    "$LVM" vgscan ${LVM_OPTS} --mknodes
fi

exit 0
