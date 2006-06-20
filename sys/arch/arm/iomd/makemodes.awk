#	$NetBSD: makemodes.awk,v 1.1.66.1 2006/06/20 15:33:51 gdamore Exp $

#
# Copyright (c) 1998 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Mark Brinicombe
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
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# This parses a Acorn monitor definition file and constructs an array of
# parameters for each mode.
# Once the file has been parsed the list of modes is examined to find modes
# that match the mode specifications specified on the command line.
# The matching mode definitions are written to stdout in the form of a C file.
# Parsing information is written to stderr.
#
#
# Syntax for using this program
#
# awk -f makemodes.awk <MDF file> <mode spec> [<mode spec> ...]
#
# where <mode spec> is
#	<x>,<y>
#	<x>,<y>,<f>
#	<x>,<y>,<c>,<f>
#
# Note: Spaces are NOT allowed in a mode specifier
#
# where	x = x resolution
#	y = y resolution
#	f = frame rate
#	c = colour depth (16, 256, 32768, 65536)
#

BEGIN {
	# Number of modes parsed and valid in the modes array.
	mode = 0;

	# MDF file globals
	monitor = "";
	dpms = 0;

	# Non zero if we are translating a mode
	translate = 0;

	# ':'  character is used to separate the tokens.
	FS=":";

	# Note the real number of arguments and truncate ARGC so that only the first
	# argument is used as a filename.
	realargc = ARGC;
	ARGC=2;
}

# MDF File format
/^file_format/ {
	# Currently we only understand format 1 MDF files
	if ($2 != 1) {
		printf("Unrecognised MDF format (%d)\n", $2);
		exit;
	}
}

# Monitor name
/^monitor_title/ {
	monitor = $2;
}

# Monitor DPMS state
/^DPMS_state/ {
	dpms = $2;
}

# Start of mode definition
/^startmode/ {
	translate = 1;
}

# End of mode definition
/^endmode/ {
	translate = 0;
	mode = mode + 1;
}

# The mode definition name (only valid within startmode/endmode section)
/^mode_name:/ {
	if (!translate)
		next;
	modes[mode, 0] = $2;
	next;
}

# The horizontal resolution (only valid within startmode/endmode section)
/^x_res:/ {
	if (!translate)
		next;
	modes[mode, 1] = $2;
	next;
}

# The vertical resolution (only valid within startmode/endmode section)
/^y_res:/ {
	if (!translate)
		next;
	modes[mode, 2] = $2;
	next;
}

# The pixel rate (only valid within startmode/endmode section)
/^pixel_rate:/ {
	if (!translate)
		next;
	modes[mode, 3] = $2;
	next;
}

# The horizontal timings (only valid within startmode/endmode section)
/^h_timings:/ {
	if (!translate)
		next;
	modes[mode, 4] = $2;
	next;
}

# The vertical timings (only valid within startmode/endmode section)
/^v_timings:/ {
	if (!translate)
		next;
	modes[mode, 5] = $2;
	next;
}

# The sync polarity (only valid within startmode/endmode section)
/^sync_pol:/ {
	if (!translate)
		next;
	modes[mode, 6] = $2;
	next;
}

END {
	#
	# Now generate the C file
	#

	# Create the file header
	printf("/*\n");
	printf(" * MACHINE GENERATED: DO NOT EDIT\n");
	printf(" *\n");
	printf(" * Created from %s\n", FILENAME);
	printf(" */\n\n");
	printf("#include <sys/types.h>\n");
	printf("#include <arm/iomd/vidc.h>\n\n");
	printf("const char *monitor = \"%s\";\n", monitor);
	printf("const int dpms = %d;\n", dpms);
	printf("\n");

	# Now define the modes array
	printf("struct vidc_mode vidcmodes[] = {\n");

	# Loop round all the modespecs on the command line
	for (res = 2; res < realargc; res = res + 1) {
		pos = -1;
		found = -1;
		closest = 200;

		# Report the mode specifier being processed
		printf("%s ==> ", ARGV[res]) | "cat 1>&2";

		# Pull apart the modespec
		args = split(ARGV[res], modespec, ",");

		# We need at least 2 arguments
		if (args < 2) {
			printf("Invalid mode specifier\n") | "cat 1>&2";
			continue;
		}

		# If we only have x,y default c and f
		if (args == 2) {
			modespec[3] = 256;
			modespec[4] = -1;
		}
		# If we have x,y,f default c and re-arrange.
		if (args == 3) {
			modespec[4] = modespec[3];
			modespec[3] = 256;
		}

		# Report the full mode specifier
		printf("%d x %d x %d x %d : ", modespec[1], modespec[2],
		    modespec[3], modespec[4]) | "cat 1>&2";

		# Now loop round all the modes we parsed and find the matches
		for (loop = 0; loop < mode; loop = loop + 1) {
			# Match X & Y
			if (modespec[1] != modes[loop, 1]) continue;
			if (modespec[2] != modes[loop, 2]) continue;

			# Split the horizontal and vertical timings
			# This is needed for the frame rate calculation
			ht = split(modes[loop, 4], htimings, ",");
			if (ht != 6) continue;
			vt = split(modes[loop, 5], vtimings, ",");
			if (vt != 6) continue;

			# Calculate the frame rate
			fr = modes[loop, 3] / (htimings[1] + htimings[2] + \
			    htimings[3] + htimings[4] + htimings[5] + \
			    htimings[6]) / ( vtimings[1] + vtimings[2] + \
			    vtimings[3] + vtimings[4] + vtimings[5] + \
			    vtimgings[6]);
			fr = fr * 1000;

			# Remember the frame rate
			modes[loop, 7] = int(fr + 0.5);

			# Report the frame rate
			printf("%d ", modes[loop, 7]) | "cat 1>&2";

			# Is this the closest
			if (closest > mod(modes[loop, 7] - modespec[4])) {
				closest = mod(modes[loop, 7] - modespec[4]);
				pos = loop;
			}

			# Do we have an exact match ?
			if (modes[loop, 7] == modespec[4])
				found = pos;
		}

		# If no exact match use the nearest
		if (found == -1)
			found = pos;

		# Did we find any sort of match ?
		if (found == -1) {
			printf("Cannot find mode") | "cat 1>&2";
			continue;
		}

		# Report the frame rate matched
		printf("- %d", modes[found, 7]) | "cat 1>&2";

		# Output the mode as part of the mode definition array
		printf("\t{ %6d, %22s, %22s, %d, %d, %d },\n",
		    modes[found, 3], modes[found, 4], modes[found, 5],
		    cdepth(modespec[3]), modes[found, 6], modes[found, 7]);

		printf("\n") | "cat 1>&2";
	}

	# Add a terminating entry and close the array.
	printf("\t{ 0 }\n");
	printf("};\n");
}

#
# cdepth() function
#
# This returns the colour depth as a power of 2 + 1
#
function cdepth(depth) {
	if (depth == 16)
		return 5;
	if (depth == 256)
		return 9;
	if (depth == 32768)
		return 16;
	if (depth == 65536)
		return 17;
	return 9;
}

#
# Simple mod() function
#
function mod(a) {
	if (a < 0)
		return -a;
	return a;
}
