#!/bin/sh

#	$NetBSD: prepare-import.sh,v 1.1 2018/08/27 00:46:21 riastradh Exp $
#
# $ /path/to/prepare-import.sh
#
# Run from the directory that will be imported as
# sys/external/bsd/drm2/dist.  Be sure to also run drm/drm2netbsd and
# nouveau/nouveau2netbsd in their respective directories.

set -Ceu

find . -name '*.h' \
| while read f; do
	cleantags "$f"
	(printf '/*\t$NetBSD: prepare-import.sh,v 1.1 2018/08/27 00:46:21 riastradh Exp $\t*/\n\n' && cat -- "$f") > "$f".tmp
	mv -f -- "$f".tmp "$f"
done

find . -name '*.c' \
| while read f; do
        # Probably not necessary -- Linux tends not to have RCS ids --
        # but a precaution out of paranoia.
	cleantags "$f"
	# Heuristically apply NetBSD RCS ids: a comment at the top of
	# the file, and a __KERNEL_RCSID before the first cpp line,
	# which, with any luck, should be the first non-comment line
	# and lie between the copyright notice and the header.
	awk '
		BEGIN {
			done = 0
			printf("/*\t%c%s%c\t*/\n\n", "$","NetBSD","$")
		}
		/^#/ && !done {
			printf("#include <sys/cdefs.h>\n")
			printf("__KERNEL_RCSID(0, \"%c%s%c\");\n",
			    "$","NetBSD","$")
			printf("\n")
			done = 1
		}
		{
			print
		}
	' < "$f" > "$f".tmp
	mv -f -- "$f".tmp "$f"
done
