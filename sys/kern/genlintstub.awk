#	$NetBSD: genlintstub.awk,v 1.7 2003/05/20 20:25:31 kristerw Exp $
#
# Copyright 2001 Wasabi Systems, Inc.
# All rights reserved.
#
# Written by Perry E. Metzger for Wasabi Systems, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed for the NetBSD Project by
#      Wasabi Systems, Inc.
# 4. The name of Wasabi Systems, Inc. may not be used to endorse
#    or promote products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# This awk script is used by kernel Makefiles to construct C lint
# stubs automatically from properly formatted comments in .S files. In
# general, a .S file should have a special comment for anything with
# something like an ENTRY designation. The special formats are:
#
# /* LINTSTUB: Empty */
# This is used as an indicator that the file contains no stubs at
# all. It generates a /* LINTED */ comment to quiet lint.
#
# /* LINTSTUB: Func: type function(args) */
# type must be void, int or long. A return is faked up for ints and longs.
#
# /* LINTSTUB: Var: type variable, variable; */
# This is often appropriate for assembly bits that the rest of the
# kernel has declared as char * and such, like various bits of
# trampoline code.
#
# /* LINTSTUB: include foo */
# Turns into a literal `#include foo' line in the source. Useful for
# making sure the stubs are checked against system prototypes like
# systm.h, cpu.h, etc., and to make sure that various types are
# properly declared.
#
# /* LINTSTUB: Ignore */
# This is used as an indicator to humans (and possible future
# automatic tools) that the entry is only used internally by other .S
# files and does not need a stub. You want this so you know you
# haven't just forgotten to put a stub in for something and you are
# *deliberately* ignoring it.

BEGIN	{
		printf "/* DO NOT EDIT! DO NOT EDIT! DO NOT EDIT! */\n";
		printf "/* DO NOT EDIT! DO NOT EDIT! DO NOT EDIT! */\n";
		printf "/* This file was automatically generated. */\n";
		printf "/* see genlintstub.awk for details.       */\n";
		printf "/* This file was automatically generated. */\n";
		printf "/* DO NOT EDIT! DO NOT EDIT! DO NOT EDIT! */\n";
		printf "/* DO NOT EDIT! DO NOT EDIT! DO NOT EDIT! */\n";
		printf "\n\n";
	}

/^\/\* LINTSTUB: Empty.*\*\/[ \t]*$/ { 
		printf "/* LINTED (empty translation unit) */\n";
		next;
	}

/^\/\* LINTSTUB: Func:.*\)[ \t]*[;]?[ \t]+\*\/[ \t]*$/ { 
		if (($4 == "int") || ($4 == "long"))
			retflag = 1;
		else if ($4 == "void")
			retflag = 0;
		else {
			printf "ERROR: %s: type is not int or void\n", $4 > "/dev/stderr";
			exit 1;
		}
		print "/* ARGSUSED */";
		for (i = 4; i < NF; i++) {
			if (i != (NF - 1))
				printf "%s ", $i;
			else {
				sub(";$", "", $i);
				printf "%s\n", $i;
			}
		}
		print "{";
		if (retflag)
			print "\treturn(0);"
		print "}\n";
		next;
	}

/^\/\* LINTSTUB: Func:/ { 
	  printf "ERROR: bad function declaration: %s\n", $0 > "/dev/stderr";
	  exit 1;
	}
			
/^\/\* LINTSTUB: Var:.*[ \t]+\*\/[ \t]*$/ { 
		for (i = 4; i < NF; i++) {
			if (i != (NF - 1))
				printf "%s ", $i;
			else {
				gsub(";$", "", $i);
				printf "%s;\n\n", $i;
			}
		}
		next;
	}

/^\/\* LINTSTUB: Var:/ { 
	  printf "ERROR: bad variable declaration: %s\n", $0 > "/dev/stderr";
	  exit 1;
	}

/^\/\* LINTSTUB: include[ \t]+.*\*\/[ \t]*$/ {
		printf "#include %s\n", $4;
		next;
	}

/^\/\* LINTSTUB: Ignore.*\*\/[ \t]*$/ { next; }

/^\/\* LINTSTUB: Ignore/ { 
	  printf "ERROR: bad ignore declaration: %s\n", $0 > "/dev/stderr";
	  exit 1;
	}

/^\/\* LINTSTUBS:/ { 
	  printf "ERROR: LINTSTUB, not LINTSTUBS: %s\n", $0 > "/dev/stderr";
	  exit 1;
	}

/^\/\* LINTSTUB:/ { 
	  printf "ERROR: bad declaration: %s\n", $0 > "/dev/stderr";
	  exit 1;
	}
