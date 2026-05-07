#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

from collections.abc import Iterator
from re import Match, Pattern
from re import compile as Re
from typing import TextIO

import abc
import re

FlexPattern = str | Pattern


def compile_pattern(string: FlexPattern) -> Pattern:
    if isinstance(string, Pattern):
        return string
    if isinstance(string, str):
        return Re(re.escape(string))
    raise TypeError("only string and re.Pattern allowed")


class Grep(abc.ABC):
    """
    Implement a grep-like interface for pattern matching in texts and files.
    """

    @abc.abstractmethod
    def readlines(self) -> Iterator[str]:
        raise NotImplementedError

    def igrep(self, pattern: FlexPattern) -> Iterator[Match]:
        """
        Iterate over the lines matching the pattern.
        """
        regex = compile_pattern(pattern)

        for line in self.readlines():
            match = regex.search(line)
            if match:
                yield match

    def grep(self, pattern: FlexPattern) -> list[Match]:
        """
        Get list of lines matching the pattern.
        """
        return list(self.igrep(pattern))

    def __contains__(self, pattern: FlexPattern) -> bool:
        """
        Return whether any of the lines in the log contains matches the pattern.
        """
        try:
            next(self.igrep(pattern))
        except StopIteration:
            return False
        return True


class Text(Grep, str):  # type: ignore
    """
    Wrapper around classic string with grep support.
    """

    def readlines(self):
        yield from self.splitlines(keepends=True)


class TextFile(Grep):
    """
    Text file wrapper with grep support.
    """

    def __init__(self, path: str):
        self.path = path

    def readlines(self) -> Iterator[str]:
        with open(self.path, encoding="utf-8") as f:
            yield from f

    def __repr__(self):
        return self.path


class LineReader(Grep):
    """
    >>> import io

    >>> file = io.StringIO("complete line\\n")
    >>> line_reader = LineReader(file)
    >>> for line in line_reader.readlines():
    ...     print(line.strip())
    complete line

    >>> file = io.StringIO("complete line\\nand then incomplete line")
    >>> line_reader = LineReader(file)
    >>> for line in line_reader.readlines():
    ...     print(line.strip())
    complete line

    >>> file = io.StringIO("complete line\\nand then another complete line\\n")
    >>> line_reader = LineReader(file)
    >>> for line in line_reader.readlines():
    ...     print(line.strip())
    complete line
    and then another complete line

    >>> file = io.StringIO()
    >>> line_reader = LineReader(file)
    >>> for chunk in (
    ...     "first line\\nsecond line\\nthi",
    ...     "rd ",
    ...     "line\\nfour",
    ...     "th line\\n\\nfifth line\\n"
    ... ):
    ...     print("=== OUTER ITERATION ===")
    ...     pos = file.tell()
    ...     print(chunk, end="", file=file)
    ...     _ = file.seek(pos)
    ...     for line in line_reader.readlines():
    ...         print("--- inner iteration ---")
    ...         print(line.strip() or "<blank>")
    === OUTER ITERATION ===
    --- inner iteration ---
    first line
    --- inner iteration ---
    second line
    === OUTER ITERATION ===
    === OUTER ITERATION ===
    --- inner iteration ---
    third line
    === OUTER ITERATION ===
    --- inner iteration ---
    fourth line
    --- inner iteration ---
    <blank>
    --- inner iteration ---
    fifth line
    """

    def __init__(self, stream: TextIO):
        self._stream = stream
        self._linebuf = ""

    def readline(self) -> str | None:
        """
        Wrapper around io.readline() function to handle unfinished lines.

        If a line ends with newline character, it's returned immediately.
        If a line doesn't end with a newline character, the read contents are
        buffered until the next call of this function and None is returned
        instead.
        """
        read = self._stream.readline()
        if not read.endswith("\n"):
            self._linebuf += read
            return None
        read = self._linebuf + read
        self._linebuf = ""
        return read

    def readlines(self) -> Iterator[str]:
        """
        Wrapper around io.readline() which only returns finished lines.
        """
        while True:
            line = self.readline()
            if line is None:
                return
            yield line
