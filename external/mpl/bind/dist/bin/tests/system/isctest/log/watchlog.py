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

from typing import Any, List, Match, Optional, Pattern, TextIO, TypeVar, Union

import abc
import os
import time

from isctest.text import compile_pattern, FlexPattern, LineReader


T = TypeVar("T")
OneOrMore = Union[T, List[T]]


class WatchLogException(Exception):
    pass


class WatchLogTimeout(WatchLogException):
    pass


class WatchLog(abc.ABC):
    """
    Wait for a log message to appear in a text file.

    This class should not be used directly; instead, its subclasses,
    `WatchLogFromStart` and `WatchLogFromHere`, should be used.  For `named`
    instances used in system tests, it is recommended to use the
    `watch_log_from_start()` and `watch_log_from_here()` helper methods exposed
    by the `NamedInstance` class (see below for recommended usage patterns).
    """

    DEFAULT_TIMEOUT = 10.0

    def __init__(self, path: str, timeout: float = DEFAULT_TIMEOUT) -> None:
        """
        `path` is the path to the log file to watch.
        `timeout` is the number of seconds (float) to wait for each wait call.

        Every instance of this class must call one of the `wait_for_*()`
        methods at least once or else an `Exception` is thrown.

        >>> with WatchLogFromStart("/dev/null") as watcher:
        ...     print("Just print something without waiting for a log line")
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogException: wait_for_*() was not called

        >>> with WatchLogFromHere("/dev/null", timeout=0.0) as watcher:
        ...     watcher.wait_for_line("foo")
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogException: timeout must be greater than 0
        """
        self._fd: Optional[TextIO] = None
        self._reader: Optional[LineReader] = None
        self._path = path
        self._wait_function_called = False
        if timeout <= 0.0:
            raise WatchLogException("timeout must be greater than 0")
        self._timeout = timeout
        self._deadline = 0.0

    def _setup_wait(self, patterns: OneOrMore[FlexPattern]) -> List[Pattern]:
        self._wait_function_called = True
        self._deadline = time.monotonic() + self._timeout
        return self._prepare_patterns(patterns)

    def _prepare_patterns(self, strings: OneOrMore[FlexPattern]) -> List[Pattern]:
        """
        Convert a mix of string(s) and/or pattern(s) into a list of patterns.

        Any strings are converted into regular expression patterns that match
        the string verbatim.
        """
        patterns = []
        if not isinstance(strings, list):
            strings = [strings]
        for string in strings:
            patterns.append(compile_pattern(string))
        return patterns

    def _wait_for_match(self, regexes: List[Pattern]) -> Match:
        if not self._reader:
            raise WatchLogException(
                "use WatchLog as context manager before calling wait_for_*() functions"
            )
        while time.monotonic() < self._deadline:
            for line in self._reader.readlines():
                for regex in regexes:
                    match = regex.search(line)
                    if match:
                        return match
            time.sleep(0.1)
        raise WatchLogTimeout(
            f"Timeout reached watching {self._path} for "
            f"{' | '.join([regex.pattern for regex in regexes])}"
        )

    def wait_for_line(self, patterns: OneOrMore[FlexPattern]) -> Match:
        """
        Block execution until any line of interest appears in the log file.

        `patterns` accepts one value or a list of values, with each value being
        either a regular expression pattern, or a string which should be
        matched verbatim (without interpreting it as a regular expression).

        If any of the patterns is found anywhere within a line in the log file,
        return the match, allowing access to the matched line, the regex
        groups, and the regex which matched. See re.Match for more.

        A `WatchLogTimeout` is raised if the function fails to find any of the
        `patterns` in the allotted time.

        Recommended use:

        ```python
        from re import compile as Re
        import isctest

        def test_foo(servers):
            with servers["ns1"].watch_log_from_start() as watcher:
                watcher.wait_for_line("all zones loaded")

            pattern = Re(r"next key event in ([0-9]+) seconds")
            with servers["ns1"].watch_log_from_here() as watcher:
                # ... do stuff here ...
                match = watcher.wait_for_line(pattern)
                seconds = int(match.groups(1))

            strings = [
                "freezing zone",
                "thawing zone",
            ]
            with servers["ns1"].watch_log_from_here() as watcher:
                # ... do stuff here ...
                match = watcher.wait_for_line(strings)
                line = match.string
        ```

        `wait_for_line()` must be called exactly once for every `WatchLog`
        instance.

        >>> # For `WatchLogFromStart`, `wait_for_line()` returns without
        >>> # raising an exception as soon as the line being looked for appears
        >>> # anywhere in the file, no matter whether that happens before of
        >>> # after the `with` statement is reached.
        >>> import tempfile
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo bar baz", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         match = watcher.wait_for_line("bar")
        >>> print(match.string.strip())
        foo bar baz
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         print("foo bar baz", file=file, flush=True)
        ...         match = watcher.wait_for_line("bar")
        >>> print(match.group(0))
        bar

        >>> # For `WatchLogFromHere`, `wait_for_line()` only returns without
        >>> # raising an exception if the string being looked for appears in
        >>> # the log file after the `with` statement is reached.
        >>> import tempfile
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo bar baz", file=file, flush=True)
        ...     with WatchLogFromHere(file.name, timeout=0.1) as watcher:
        ...         watcher.wait_for_line("bar") #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogTimeout: ...
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo bar baz", file=file, flush=True)
        ...     with WatchLogFromHere(file.name) as watcher:
        ...         print("bar qux", file=file, flush=True)
        ...         match = watcher.wait_for_line("bar")
        >>> print(match.string.strip())
        bar qux

        >>> # Different values must be returned depending on which line is
        >>> # found in the log file.
        >>> import tempfile
        >>> from re import compile as Re
        >>> patterns = [Re(r"bar ([0-9])"), "qux"]
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo bar 3", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         match1 = watcher.wait_for_line(patterns)
        ...     with WatchLogFromHere(file.name) as watcher:
        ...         print("baz qux", file=file, flush=True)
        ...         match2 = watcher.wait_for_line(patterns)
        >>> print(match1.group(1))
        3
        >>> print(match2.group(0))
        qux
        """
        regexes = self._setup_wait(patterns)

        return self._wait_for_match(regexes)

    def wait_for_sequence(self, patterns: List[FlexPattern]) -> List[Match]:
        """
        Block execution until the specified pattern sequence is found in the
        log file.

        `patterns` is a list of values, with each value being either a regular
        expression pattern, or a string which should be matched verbatim
        (without interpreting it as a regular expression). Order of patterns is
        important, as each pattern is looked for only after all the previous
        patterns have matched.

        All the matches are returned as a list.

        A `WatchLogTimeout` is raised if the function fails to find all of the
        `patterns` in the given order in the allotted time.

        >>> import tempfile
        >>> seq = ['a', 'b', 'c']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("b", file=file, flush=True)
        ...     print("a", file=file, flush=True)
        ...     print("b", file=file, flush=True)
        ...     print("z", file=file, flush=True)
        ...     print("c", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         ret = watcher.wait_for_sequence(seq)
        >>> assert ret[0].group(0) == "a"
        >>> assert ret[1].group(0) == "b"
        >>> assert ret[2].group(0) == "c"

        >>> import tempfile
        >>> seq = ['a', 'b', 'c']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("b", file=file, flush=True)
        ...     print("a", file=file, flush=True)
        ...     print("c", file=file, flush=True)
        ...     with WatchLogFromStart(file.name, timeout=0.1) as watcher:
        ...         ret = watcher.wait_for_sequence(seq)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogTimeout: ...

        >>> import tempfile
        >>> seq = ['a', 'b', 'c']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("b", file=file, flush=True)
        ...     print("a", file=file, flush=True)
        ...     print("b", file=file, flush=True)
        ...     with WatchLogFromStart(file.name, timeout=0.1) as watcher:
        ...         ret = watcher.wait_for_sequence(seq)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogTimeout: ...

        >>> import tempfile
        >>> seq = ['a', 'b', 'c']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("b", file=file, flush=True)
        ...     print("a", file=file, flush=True)
        ...     print("c", file=file, flush=True)
        ...     print("b", file=file, flush=True)
        ...     with WatchLogFromStart(file.name, timeout=0.1) as watcher:
        ...         ret = watcher.wait_for_sequence(seq)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogTimeout: ...
        """
        regexes = self._setup_wait(patterns)
        matches = []

        for regex in regexes:
            match = self._wait_for_match([regex])
            matches.append(match)

        return matches

    def wait_for_all(self, patterns: List[FlexPattern]) -> List[Match]:
        """
        Block execution until all the specified patterns are found in the
        log file in any order.

        `patterns` is a list of values, with each value being either a regular
        expression pattern, or a string which should be matched verbatim
        (without interpreting it as a regular expression). Order of patterns is
        irrelevant and they may appear in any order.

        All the matches are returned as a list. The matches are listed in the
        order of appearance. Pattern may match more than once, and all the
        matches are included. To pair matches with the patterns, re.Match.re
        may be used.

        A `WatchLogTimeout` is raised if the function fails to find all of the
        `patterns` in the allotted time.

        >>> import tempfile
        >>> patterns = ['foo', 'bar']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("bar", file=file, flush=True)
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         ret = watcher.wait_for_all(patterns)
        >>> assert ret[0].group(0) == "bar"
        >>> assert ret[1].group(0) == "foo"

        >>> import tempfile
        >>> from re import compile as Re
        >>> bar_pattern = Re('bar')
        >>> patterns = ['foo', bar_pattern]
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("bar", file=file, flush=True)
        ...     print("baz", file=file, flush=True)
        ...     print("bar", file=file, flush=True)
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         ret = watcher.wait_for_all(patterns)
        >>> assert len(ret) == 3
        >>> assert ret[0].group(0) == "bar"
        >>> assert ret[1].group(0) == "bar"
        >>> assert ret[2].group(0) == "foo"
        >>> assert ret[0].re == bar_pattern
        >>> assert ret[1].re == bar_pattern
        >>> assert ret[2].re.pattern == "foo"

        >>> import tempfile
        >>> patterns = ['foo', 'bar']
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo", file=file, flush=True)
        ...     print("quux", file=file, flush=True)
        ...     with WatchLogFromStart(file.name, timeout=0.1) as watcher:
        ...         ret = watcher.wait_for_all(patterns)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        isctest.log.watchlog.WatchLogTimeout: ...
        """
        regexes = self._setup_wait(patterns)
        unmatched_regexes = set(regexes)
        matches = []

        while unmatched_regexes:
            match = self._wait_for_match(regexes)
            matches.append(match)
            unmatched_regexes.discard(match.re)

        return matches

    def __enter__(self) -> Any:
        self._fd = open(self._path, encoding="utf-8")
        self._seek_on_enter()
        self._reader = LineReader(self._fd)
        return self

    @abc.abstractmethod
    def _seek_on_enter(self) -> None:
        """
        This method is responsible for setting the file position indicator for
        the file being watched when execution reaches the __enter__() method.
        It is expected to be set differently depending on which `WatchLog`
        subclass is used.  Since the base `WatchLog` class should not be used
        directly, raise an exception upon any attempt of such use.
        """
        raise NotImplementedError

    def __exit__(self, *_: Any) -> None:
        if not self._wait_function_called:
            raise WatchLogException("wait_for_*() was not called")
        self._reader = None
        assert self._fd
        self._fd.close()


class WatchLogFromStart(WatchLog):
    """
    A `WatchLog` subclass which looks for the provided string(s) in the entire
    log file.
    """

    def _seek_on_enter(self) -> None:
        pass


class WatchLogFromHere(WatchLog):
    """
    A `WatchLog` subclass which only looks for the provided string(s) in the
    portion of the log file which is appended to it after the `with` statement
    is reached.
    """

    def _seek_on_enter(self) -> None:
        assert self._fd
        self._fd.seek(0, os.SEEK_END)
