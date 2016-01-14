#!/bin/sh
# Submit a problem report to a GNATS site.
# Copyright (C) 1993 Free Software Foundation, Inc.
# Contributed by Brendan Kehoe (brendan@cygnus.com), based on a
# version written by Heinz G. Seidl (hgs@cygnus.com).
#
# This file is part of GNU GNATS.
#
# GNU GNATS is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU GNATS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU GNATS; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

# The version of this send-pr.
VERSION=xVERSIONx

# The submitter-id for your site.
SUBMITTER=xSUBMITTERx

# Where the GNATS directory lives, if at all.
[ -z "$GNATS_ROOT" ] && 
GNATS_ROOT=xGNATS_ROOTx

# The default mail address for PR submissions. 
GNATS_ADDR=xGNATS_ADDRx

# Where the gnats category tree lives.
DATADIR=xDATADIRx

# If we've been moved around, try using GCC_EXEC_PREFIX.
[ ! -d $DATADIR/gnats -a -d "$GCC_EXEC_PREFIX" ] && 
  DATADIR=${GCC_EXEC_PREFIX}../../../lib

# The default release for this host.
DEFAULT_RELEASE="xDEFAULT_RELEASEx"

# The default organization.
DEFAULT_ORGANIZATION="xDEFAULT_ORGANIZATIONx"

# The default site to look for.
GNATS_SITE=xGNATS_SITEx

# Newer config information?
[ -f ${GNATS_ROOT}/gnats-adm/config ] && . ${GNATS_ROOT}/gnats-adm/config

# What mailer to use.  This must come after the config file, since it is
# host-dependent.
MAIL_AGENT="xMAIL_AGENTx"

# How to read the passwd database.
PASSWD="xPASSWDx"

ECHON=xECHONx

if [ $ECHON = bsd ] ; then
  ECHON1="echo -n"
  ECHON2=
elif [ $ECHON = sysv ] ; then
  ECHON1=echo
  ECHON2='\c'
else
  ECHON1=echo
  ECHON2=
fi

#

if [ -z "$TMPDIR" ]; then
  TMPDIR=/tmp
else
  if [ "`echo $TMPDIR | grep '/$'`" != "" ]; then
    TMPDIR="`echo $TMPDIR | sed -e 's,/$,,'`"
  fi
fi

TEMP=`mktemp -t p` || exit 1
BAD=`mktemp -t pbad` || exit 1
REF=`mktemp -t pf` || exit 1

# find a user name
if [ "$LOGNAME" = "" ]; then
	if [ "$USER" != "" ]; then
		LOGNAME="$USER"
	else
		LOGNAME="UNKNOWN"
	fi
fi

FROM="$LOGNAME"
if [ -z "$REPLYTO" ]; then
  REPLYTO="$LOGNAME"
fi

# Find out the name of the originator of this PR.
if [ -n "$NAME" ]; then
  ORIGINATOR="$NAME"
elif [ -f $HOME/.fullname ]; then
  ORIGINATOR="`sed -e '1q' $HOME/.fullname`"
else
  ORIGINATOR=`$PASSWD | sed -e /"^${LOGNAME}:"/'{s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^,:;]*\).*$/\1/' -e q -e } -e d`
  case "$ORIGINATOR" in
  *'&'*)
    AMP=`echo $LOGNAME | tr '[a-z]' '[A-Z]'`
    AMP=`echo $AMP $LOGNAME | sed 's/^\(.\)[^ ]* ./\1/'`
    ORIGINATOR=`echo "$ORIGINATOR" | sed "s/&/$AMP/"`
    ;;
  esac
fi

if [ -n "$ORGANIZATION" ]; then
  if [ -f "$ORGANIZATION" ]; then
    ORGANIZATION="`cat $ORGANIZATION`"
  fi
else
  if [ -n "$DEFAULT_ORGANIZATION" ]; then
    ORGANIZATION="$DEFAULT_ORGANIZATION"
  elif [ -f $HOME/.organization ]; then
    ORGANIZATION="`cat $HOME/.organization`"
  elif [ -f $HOME/.signature ]; then
    ORGANIZATION="`cat $HOME/.signature`"
  fi
fi

# If they don't have a preferred editor set, then use
if [ -z "$VISUAL" ]; then
  if [ -z "$EDITOR" ]; then
    EDIT=vi
  else
    EDIT="$EDITOR"
  fi
else
  EDIT="$VISUAL"
fi

# Find out some information.
if [ -f /bin/uname ]; then
  UNAME="/bin/uname"
elif [ -f /usr/bin/uname ]; then
  UNAME="/usr/bin/uname"
else
  UNAME="echo"
fi
SYSTEM=`$UNAME -a`
ARCH=`$UNAME -p`
if [ -z "$ARCH" -a -f /bin/arch ]; then
  ARCH=`/bin/arch`
fi
MACHINE=`$UNAME -m`
if [ -z "$ARCH" -a -f /bin/machine ]; then
  ARCH=`/bin/arch`
fi

COMMAND=`echo $0 | sed -e 's,.*/,,'`
USAGE="Usage: $COMMAND [-PVL] [-t address] [-f filename] [--request-id] 
[--version]"
REMOVE=
BATCH=

while [ $# -gt 0 ]; do
  case "$1" in
    -r) ;; 		# Ignore for backward compat.
    -t | --to) if [ $# -eq 1 ]; then echo "$USAGE"; exit 1; fi
	shift ; GNATS_ADDR="$1"
	EXPLICIT_GNATS_ADDR=true
        ;;
    -f | --file) if [ $# -eq 1 ]; then echo "$USAGE"; exit 1; fi
	shift ; IN_FILE="$1"
	if [ "$IN_FILE" != "-" -a ! -r "$IN_FILE" ]; then
	  echo "$COMMAND: cannot read $IN_FILE"
	  exit 1
	fi
	;;
    -b | --batch) BATCH=true ;;
    -p | -P | --print) PRINT=true ;;
    -L | --list) FORMAT=norm ;;
    -l | -CL | --lisp) FORMAT=lisp ;;
    --request-id) REQUEST_ID=true ;;
    -h | --help) echo "$USAGE"; exit 0 ;;
    -V | --version) echo "$VERSION"; exit 0 ;;
    -*) echo "$USAGE" ; exit 1 ;;
    *) if [ -z "$USER_GNATS_SITE" ]; then
	 if [ ! -r "$DATADIR/gnats/$1" ]; then
	   echo "$COMMAND: the GNATS site $1 does not have a categories list."
	   exit 1
	 else
	   # The site name is the alias they'll have to have created.
	   USER_GNATS_SITE=$1
	 fi
       else
	 echo "$USAGE" ; exit 1
       fi
       ;;
 esac
 shift
done

if [ -n "$USER_GNATS_SITE" ] && [ "$USER_GNATS_SITE" != "$GNATS_SITE" ]; then
  GNATS_SITE=$USER_GNATS_SITE
  GNATS_ADDR=$USER_GNATS_SITE-gnats
fi

if [ "$SUBMITTER" = "unknown" -a -z "$REQUEST_ID" -a -z "$IN_FILE" ]; then
  cat << '__EOF__'
It seems that send-pr is not installed with your unique submitter-id.
You need to run

          install-sid YOUR-SID

where YOUR-SID is the identification code you received with `send-pr'.
`send-pr' will automatically insert this value into the template field
`>Submitter-Id'.  If you've downloaded `send-pr' from the Net, use `net'
for this value.  If you do not know your id, run `send-pr --request-id' to 
get one from your support site.
__EOF__
  exit 1
fi

if [ -r "$DATADIR/gnats/$GNATS_SITE" ]; then
  CATEGORIES=`grep -v '^#' $DATADIR/gnats/$GNATS_SITE | sort`
else
  echo "$COMMAND: could not read $DATADIR/gnats/$GNATS_SITE for categories list."
  exit 1
fi

if [ -z "$CATEGORIES" ]; then
  echo "$COMMAND: the categories list for $GNATS_SITE was empty!"
  exit 1
fi

case "$FORMAT" in
  lisp) echo "$CATEGORIES" | \
        awk 'BEGIN {printf "( "} {printf "(\"%s\") ",$0} END {printf ")\n"}'
        exit 0
        ;;
  norm) l=`echo "$CATEGORIES" | \
	awk 'BEGIN {max = 0; } { if (length($0) > max) { max = length($0); } }
	     END {print max + 1;}'`
	c=`expr 70 / $l`
	if [ $c -eq 0 ]; then c=1; fi
	echo "$CATEGORIES" | \
        awk 'BEGIN {print "Known categories:"; i = 0 }
          { printf ("%-'$l'.'$l's", $0); if ((++i % '$c') == 0) { print "" } }
            END { print ""; }'
        exit 0
        ;;
esac

ORIGINATOR_C='<name of the PR author (one line)>'
ORGANIZATION_C='<organization of PR author (multiple lines)>'
CONFIDENTIAL_C='<[ no | yes ] (one line)>'
SYNOPSIS_C='<synopsis of the problem (one line)>'
SEVERITY_C='<[ non-critical | serious | critical ] (one line)>'
PRIORITY_C='<[ low | medium | high ] (one line)>'
CATEGORY_C='<problem report category - see top for list (one line)>'
CLASS_C='<[ sw-bug | doc-bug | change-request | support ] (one line)>'
RELEASE_C_1='<Please check that the above is correct for the bug being reported,>'
RELEASE_C_2='<and append source date of snapshot, if applicable (one line).>'
ENVIRONMENT_C_1='<The following information is extracted from your kernel. Please>'
ENVIRONMENT_C_2='<append output of "ldd", "ident" where relevant (multiple lines).>'
DESCRIPTION_C='<precise description of the problem (multiple lines)>'
HOW_TO_REPEAT_C='<code/input/activities to reproduce the problem (multiple lines)>'
FIX_C='<how to correct or work around the problem, if known (multiple lines)>'

# Catch some signals. ($xs kludge needed by Sun /bin/sh)
xs=0
TRAP_EXIT_ACTION='rm -f $REF $TEMP; exit $xs'
TRAP_IGNORE_ACTION=''
TRAP_ABORT_ACTION='echo "$COMMAND: Aborting ..."; rm -f $REF $TEMP; xs=1; exit'
TRAP_ABORTSAVE_ACTION='
    echo "$COMMAND: Aborting ...";
    if cmp -s $REF $TEMP ; then
	rm $REF
    else
	echo "$COMMAND: the problem report remains in $BAD and is not sent."
	mv $REF $BAD
    fi
    rm -f $TEMP; xs=1; exit'
trap "$TRAP_EXIT_ACTION" 0
trap "$TRAP_ABORT_ACTION" 1 2 3 13 15

# If they told us to use a specific file, then do so.
if [ -n "$IN_FILE" ]; then
  if [ "$IN_FILE" = "-" ]; then
    # The PR is coming from the standard input.
    if [ -n "$EXPLICIT_GNATS_ADDR" ]; then
      sed -e "s;^[Tt][Oo]:.*;To: $GNATS_ADDR;" > $TEMP
    else
      cat > $TEMP
    fi
  else
    # Use the file they named.
    if [ -n "$EXPLICIT_GNATS_ADDR" ]; then
      sed -e "s;^[Tt][Oo]:.*;To: $GNATS_ADDR;" $IN_FILE > $TEMP
    else
      cat $IN_FILE > $TEMP
    fi
  fi
else

  if [ -n "$PR_FORM" -a -z "$PRINT_INTERN" ]; then
    # If their PR_FORM points to a bogus entry, then bail.
    if [ ! -f "$PR_FORM" -o ! -r "$PR_FORM" -o ! -s "$PR_FORM" ]; then
      echo "$COMMAND: can't seem to read your template file (\`$PR_FORM'), ignoring PR_FORM"
      sleep 1
      PRINT_INTERN=bad_prform
    fi
  fi

  if [ -n "$PR_FORM" -a -z "$PRINT_INTERN" ]; then
    cp $PR_FORM $TEMP || 
      ( echo "$COMMAND: could not copy $PR_FORM" ; xs=1; exit )
  else
    for file in $TEMP $REF ; do
      cat  > $file << '__EOF__'
SEND-PR: -*- send-pr -*-
SEND-PR: Lines starting with `SEND-PR' will be removed automatically, as
SEND-PR: will all comments (text enclosed in `<' and `>').
SEND-PR: 
SEND-PR: Please consult the send-pr man page `send-pr(1)' or the Texinfo
SEND-PR: manual if you are not sure how to fill out a problem report.
SEND-PR:
SEND-PR: NOTE: If you include a patch that corrects the problem, the
SEND-PR: issue will probably be handled much more quickly.
SEND-PR:
SEND-PR: Choose from the following categories:
SEND-PR:
__EOF__

      # Format the categories so they fit onto lines.
	l=`echo "$CATEGORIES" | \
	awk 'BEGIN {max = 0; } { if (length($0) > max) { max = length($0); } }
	     END {print max + 1;}'`
	c=`expr 61 / $l`
	if [ $c -eq 0 ]; then c=1; fi
	echo "$CATEGORIES" | \
        awk 'BEGIN {printf "SEND-PR: "; i = 0 }
          { printf ("%-'$l'.'$l's", $0);
	    if ((++i % '$c') == 0) { printf "\nSEND-PR: " } }
            END { printf "\nSEND-PR:\n"; }' >> $file

      cat >> $file << __EOF__
To: $GNATS_ADDR
Subject: 
From: $FROM
Reply-To: $REPLYTO
X-send-pr-version: $VERSION


>Submitter-Id:	$SUBMITTER
>Originator:	$ORIGINATOR
>Organization:
${ORGANIZATION-	$ORGANIZATION_C}
>Confidential:	$CONFIDENTIAL_C
>Synopsis:	$SYNOPSIS_C
>Severity:	$SEVERITY_C
>Priority:	$PRIORITY_C
>Category:	$CATEGORY_C
>Class:		$CLASS_C
>Release:	${DEFAULT_RELEASE:-"unreleased"}
	$RELEASE_C_1
	$RELEASE_C_2
>Environment:
	$ENVIRONMENT_C_1
	$ENVIRONMENT_C_2
`[ -n "$SYSTEM" ] && echo System: $SYSTEM`
`[ -n "$ARCH" ] && echo Architecture: $ARCH`
`[ -n "$MACHINE" ] && echo Machine: $MACHINE`
>Description:
	$DESCRIPTION_C
>How-To-Repeat:
	$HOW_TO_REPEAT_C
>Fix:
	$FIX_C
__EOF__
    done
  fi

  if [ "$PRINT" = true -o "$PRINT_INTERN" = true ]; then
    cat $TEMP
    xs=0; exit
  fi

  chmod u+w $TEMP
  if [ -z "$REQUEST_ID" ]; then
    trap "$TRAP_IGNORE_ACTION" 2 3
    eval $EDIT $TEMP
    trap "$TRAP_ABORTSAVE_ACTION" 2 3
  else
    ed -s $TEMP << '__EOF__'
/^Subject/s/^Subject:.*/Subject: request for a customer id/
/^>Category/s/^>Category:.*/>Category: send-pr/
w
q
__EOF__
  fi

  if cmp -s $REF $TEMP ; then
    echo "$COMMAND: problem report not filled out, therefore not sent"
    xs=1; exit
  fi
fi

#
#	Check the enumeration fields

# This is a "sed-subroutine" with one keyword parameter 
# (with workaround for Sun sed bug)
#
SED_CMD='
/$PATTERN/{
s|||
s|<.*>||
s|^[ 	]*||
s|[ 	]*$||
p
q
}'


while [ -z "$REQUEST_ID" ]; do
  CNT=0

  # 1) Confidential
  #
  PATTERN=">Confidential:"
  CONFIDENTIAL=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  case "$CONFIDENTIAL" in
    ""|yes|no) CNT=`expr $CNT + 1` ;;
    *) echo "$COMMAND: \`$CONFIDENTIAL' is not a valid value for \`Confidential'." ;;
  esac
  #
  # 2) Severity
  #
  PATTERN=">Severity:"
  SEVERITY=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  case "$SEVERITY" in
    ""|non-critical|serious|critical) CNT=`expr $CNT + 1` ;;
    *)  echo "$COMMAND: \`$SEVERITY' is not a valid value for \`Severity'."
  esac
  #
  # 3) Priority
  #
  PATTERN=">Priority:"
  PRIORITY=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  case "$PRIORITY" in
    ""|low|medium|high) CNT=`expr $CNT + 1` ;;
    *)  echo "$COMMAND: \`$PRIORITY' is not a valid value for \`Priority'."
  esac
  #
  # 4) Category
  #
  PATTERN=">Category:"
  CATEGORY=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  FOUND=
  for C in $CATEGORIES
  do
    if [ "$C" = "$CATEGORY" ]; then FOUND=true ; break ; fi
  done
  if [ -n "$FOUND" ]; then
    CNT=`expr $CNT + 1`	
  else
    if [ -z "$CATEGORY" ]; then
      echo "$COMMAND: you must include a Category: field in your report."
    else
      echo "$COMMAND: \`$CATEGORY' is not a known category."
    fi
  fi
  #
  # 5) Class
  #
  PATTERN=">Class:"
  CLASS=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  case "$CLASS" in
    ""|sw-bug|doc-bug|change-request|support) CNT=`expr $CNT + 1` ;;
    *)  echo "$COMMAND: \`$CLASS' is not a valid value for \`Class'."
  esac
  #
  # 6) Release
  #
  PATTERN=">Release:"
  RELEASE=`eval sed -n -e "\"$SED_CMD\"" $TEMP`
  if [ -z "$RELEASE" ]; then
    echo "$COMMAND: you must include a Release: field in your report."
  else
    CNT=`expr $CNT + 1`
  fi

  [ $CNT -lt 6 -a -z "$BATCH" ] && 
    echo "Errors were found with the problem report."

  while true; do
    if [ -z "$BATCH" ]; then
      $ECHON1 "a)bort, e)dit or s)end? $ECHON2"
      read input
    else
      if [ $CNT -eq 6 ]; then
        input=s
      else
        input=a
      fi
    fi
    case "$input" in
      a*)
	if [ -z "$BATCH" ]; then
	  echo "$COMMAND: the problem report remains in $BAD and is not sent."
	  mv $TEMP $BAD
        else
	  echo "$COMMAND: the problem report is not sent."
	fi
	xs=1; exit
	;;
      e*)
	trap "$TRAP_IGNORE_ACTION" 2 3
	eval $EDIT $TEMP
	trap "$TRAP_ABORTSAVE_ACTION" 2 3
	continue 2
	;;
      s*)
	break 2
	;;
    esac
  done
done
#
#	Remove comments and send the problem report
#	(we have to use patterns, where the comment contains regex chars)
#
# /^>Originator:/s;$ORIGINATOR;;
sed  -e "
/^SEND-PR:/d
/^>Organization:/,/^>[A-Za-z-]*:/s;$ORGANIZATION_C;;
/^>Confidential:/s;<.*>;;
/^>Synopsis:/s;$SYNOPSIS_C;;
/^>Severity:/s;<.*>;;
/^>Priority:/s;<.*>;;
/^>Category:/s;$CATEGORY_C;;
/^>Class:/s;<.*>;;
/^>Release:/,/^>[A-Za-z-]*:/s;$RELEASE_C_1;;
/^>Release:/,/^>[A-Za-z-]*:/s;$RELEASE_C_2;;
/^>Environment:/,/^>[A-Za-z-]*:/s;$ENVIRONMENT_C_1;;
/^>Environment:/,/^>[A-Za-z-]*:/s;$ENVIRONMENT_C_2;;
/^>Description:/,/^>[A-Za-z-]*:/s;$DESCRIPTION_C;;
/^>How-To-Repeat:/,/^>[A-Za-z-]*:/s;$HOW_TO_REPEAT_C;;
/^>Fix:/,/^>[A-Za-z-]*:/s;$FIX_C;;
" $TEMP > $REF

if $MAIL_AGENT < $REF; then
  echo "$COMMAND: problem report sent"
  xs=0; exit
else
  echo "$COMMAND: mysterious mail failure."
  if [ -z "$BATCH" ]; then
    echo "$COMMAND: the problem report remains in $BAD and is not sent."
    mv $REF $BAD
  else
    echo "$COMMAND: the problem report is not sent."
  fi
  xs=1; exit
fi
