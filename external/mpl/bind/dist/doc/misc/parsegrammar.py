############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

"""
Read ISC config grammar description produced by "cfg_test --grammar",
transform it into JSON, and print it to stdout.

Beware: This parser is pretty dumb and heavily depends on cfg_test output
format. See parse_mapbody() for more details.

Maps are recursively parsed into sub-dicts, all other elements (lists etc.)
are left intact and returned as one string.

Output example from named.conf grammar showing three variants follow.
Keys "_flags" and "_id" are present only if non-empty. Key "_grammar" denotes
end node, key "_mapbody" denotes a nested map.

{
    "acl": {
        "_flags": [
            "may occur multiple times"
        ],
        "_grammar": "<string> { <address_match_element>; ... }"
    },
    "http": {
        "_flags": [
            "may occur multiple times"
        ],
        "_id": "<string>",
        "_mapbody": {
            "endpoints": {
                "_grammar": "{ <quoted_string>; ... }"
            },
            "streams-per-connection": {
                "_grammar": "<integer>"
            }
        }
    },
    "options": {
        "_mapbody": {
            "rate-limit": {
                "_mapbody": {
                    "all-per-second": {
                        "_grammar": "<integer>"
                    }
                }
            }
        }
    }
}
"""
import fileinput
import json
import re

FLAGS = [
    "may occur multiple times",
    "obsolete",
    "deprecated",
    "experimental",
    "test only",
]

KEY_REGEX = re.compile("[a-zA-Z0-9-]+")


def split_comments(line):
    """Split line on comment boundary and strip right-side whitespace.
    Supports only #, //, and /* comments which end at the end of line.
    It does NOT handle:
    - quoted strings
    - /* comments which do not end at line boundary
    - multiple /* comments on a single line
    """
    assert '"' not in line, 'lines with " are not supported'
    data_end_idx = len(line)
    for delimiter in ["#", "//", "/*"]:
        try:
            data_end_idx = min(line.index(delimiter), data_end_idx)
        except ValueError:
            continue
        if delimiter == "/*":
            # sanity checks
            if not line.rstrip().endswith("*/"):
                raise NotImplementedError(
                    "unsupported /* comment, does not end at the end of line", line
                )
            if "/*" in line[data_end_idx + 1 :]:
                raise NotImplementedError(
                    "unsupported line with multiple /* comments", line
                )

    noncomment = line[:data_end_idx]
    comment = line[data_end_idx:]
    return noncomment, comment


def parse_line(filein):
    """Consume single line from input, return non-comment and comment."""
    for line in filein:
        line, comment = split_comments(line)
        line = line.strip()
        comment = comment.strip()
        if not line:
            continue
        yield line, comment


def parse_flags(comments):
    """Extract known flags from comments. Must match exact strings used by cfg_test."""
    out = []
    for flag in FLAGS:
        if flag in comments:
            out.append(flag)
    return out


def parse_mapbody(filein):
    """Parse body of a "map" in ISC config format.

    Input lines can be only:
    - whitespace & comments only -> ignore
    - <keyword> <anything>; -> store <anything> as "_grammar" for this keyword
    - <keyword> <anything> { -> parse sub-map and store (optional) <anything> as "_id",
                                producing nested dict under "_mapbody"
    Also store known strings found at the end of line in "_flags".

    Returns:
    - tuple (map dict, map comment) when }; line is reached
    - map dict when we run out of lines without the closing };
    """
    thismap = {}
    for line, comment in parse_line(filein):
        flags = parse_flags(comment)
        if line == "};":  # end of a nested map
            return thismap, flags

        # first word - a map key name
        # beware: some statements do not have parameters, e.g. "null;"
        key = line.split()[0].rstrip(";")
        # map key sanity check
        if not KEY_REGEX.fullmatch(key):
            raise NotImplementedError("suspicious keyword detected", line)

        # omit keyword from the grammar
        grammar = line[len(key) :].strip()
        # also skip final ; or {
        grammar = grammar[:-1].strip()

        thismap[key] = {}
        if line.endswith("{"):
            # nested map, recurse, but keep "extra identifiers" if any
            try:
                subkeys, flags = parse_mapbody(filein)
            except ValueError:
                raise ValueError("unfinished nested map, missing }; detected") from None
            if flags:
                thismap[key]["_flags"] = flags
            if grammar:
                # for lines which look like "view <name> {" store "<name>"
                thismap[key]["_id"] = grammar
            thismap[key]["_mapbody"] = subkeys
        else:
            assert line.endswith(";")
            if flags:
                thismap[key]["_flags"] = flags
            thismap[key]["_grammar"] = grammar

    # Ran out of lines: can happen only on the end of the top-level map-body!
    # Intentionally do not return second parameter to cause ValueError
    # if we reach this spot with a missing }; in a nested map.
    assert len(thismap)
    return thismap


def main():
    """Read stdin or filename provided on command line"""
    with fileinput.input() as filein:
        grammar = parse_mapbody(filein)
    print(json.dumps(grammar, indent=4))


if __name__ == "__main__":
    main()
