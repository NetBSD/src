#! /bin/sh
# shdeps.sh: Generate OS dependency fixups, for `groff' shell scripts
#
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
#      Written by Keith Marshall (keith.d.marshall@ntlworld.com)
#
# Invoked only by `make', as:
#    $(SHELL) shdeps.sh "$(RT_SEP)" "$(SH_SEP)" "$(bindir)" > shdeps.sed
# 
# This file is part of groff.
# 
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.
# 
# groff is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with groff; see the file COPYING.  If not, write to the Free Software
# Foundation, 51 Franklin St - Fifth Floor, Boston, MA 02110-1301, USA.

cat << ETX
# shdeps.sed: Script generated automatically by \`make' -- do not modify!

/^$/N
/@GROFF_BIN_PATH_SETUP@/c\\
ETX

if [ "$1$2" = "::" ]
then
  # `PATH_SEPARATOR' is `:' both at `groff' run time, and in `make',
  # implying an implementation which is completely POSIX compliant.
  # Simply apply the `GROFF_BIN_PATH' and `PATH_SEPARATOR' values
  # determined by `configure', in all cases.

  cat << ETX
\\
GROFF_RUNTIME="\${GROFF_BIN_PATH=$3}:"
/@PATH_SEARCH_SETUP@/d
ETX

else
  # `PATH_SEPARATOR' is NOT always `:',
  # which suggests an implementation for a Microsoft platform.
  # We need to choose the `GROFF_BIN_PATH' format and `PATH_SEPARATOR'
  # which will suit the user's choice of shell.
  #
  # Note that some Windows users may specify the `--prefix' path
  # using backslash characters, instead of `/', preferred by POSIX,
  # so we will also fix that up.

  POSIX_BINDIR=`echo $3 | tr '\\\\' /`
  cat << ETX
# (The value required is dependent on the user's choice of shell,\\
#  and its associated POSIX emulation capabilities.)\\
\\
case "\$OSTYPE" in\\
  msys)\\
    GROFF_RUNTIME=\${GROFF_BIN_PATH="`
      case "$POSIX_BINDIR" in
	[a-zA-Z]:*)
	  IFS=':'
	  set -- $POSIX_BINDIR
	  case "$2" in
	    /*) POSIX_BINDIR="/$1$2"  ;;
	     *) POSIX_BINDIR="/$1/$2" ;;
	  esac
	  shift 2
	  for dir
	    do
	      POSIX_BINDIR="$POSIX_BINDIR:$dir"
	    done
	  ;;
      esac
      echo "$POSIX_BINDIR"`"}":" ;;\\
  cygwin)\\
    : \${GROFF_BIN_PATH="\`cygpath -w '$POSIX_BINDIR'\`"}\\
    GROFF_RUNTIME=\`cygpath "\$GROFF_BIN_PATH"\`":" ;;\\
  *)\\
    GROFF_RUNTIME=\${GROFF_BIN_PATH="$POSIX_BINDIR"}";" ;;\\
esac
ETX
  # On Microsoft platforms, we may also need to configure
  # the PATH search function, used in the `pdfroff' script,
  # to use ';', instead of ':', as the PATH_SEPARATOR.

  cat << ETX
/@PATH_SEARCH_SETUP@/c\\
#\\
# This implementation is configured for a Microsoft platform.\\
# Thus, the default PATH_SEPARATOR is ';', although some shells may\\
# use the POSIX standard ':' instead.  Therefore, we need to examine\\
# the OSTYPE environment variable, to identify which is appropriate\\
# to make PATH searches work correctly.\\
#\\
  case "\$OSTYPE" in\\
#\\
    msys | cygwin)\\
    #\\
    # These emulate POSIX, and use ':'\\
    #\\
      PATH_SEPARATOR=\${PATH_SEPARATOR-':'} ;;\\
#\\
    *)\\
    #\\
    # For anything else, default to ';'\\
    #\\
      PATH_SEPARATOR=\${PATH_SEPARATOR-';'} ;;\\
  esac
ETX

fi

# eof
