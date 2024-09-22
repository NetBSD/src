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

from typing import Iterator, Optional, TextIO, Dict, Any, Union, Pattern

import abc
import os
import time


class WatchLogException(Exception):
    pass


class LogFile:
    """
    Log file wrapper with a path and means to find a string in its contents.
    """

    def __init__(self, path: str):
        self.path = path

    @property
    def _lines(self) -> Iterator[str]:
        with open(self.path, encoding="utf-8") as f:
            yield from f

    def __contains__(self, substring: str) -> bool:
        """
        Return whether any of the lines in the log contains a given string.
        """
        for line in self._lines:
            if substring in line:
                return True
        return False

    def expect(self, msg: str):
        """Check the string is present anywhere in the log file."""
        if msg in self:
            return
        assert False, f"log message not found in log {self.path}: {msg}"

    def prohibit(self, msg: str):
        """Check the string is not present in the entire log file."""
        if msg in self:
            assert False, f"forbidden message appeared in log {self.path}: {msg}"


class WatchLog(abc.ABC):
    """
    Wait for a log message to appear in a text file.

    This class should not be used directly; instead, its subclasses,
    `WatchLogFromStart` and `WatchLogFromHere`, should be used.  For `named`
    instances used in system tests, it is recommended to use the
    `watch_log_from_start()` and `watch_log_from_here()` helper methods exposed
    by the `NamedInstance` class (see below for recommended usage patterns).
    """

    def __init__(self, path: str) -> None:
        """
        `path` is the path to the log file to watch.

        Every instance of this class must call one of the `wait_for_*()`
        methods exactly once or else an `Exception` is thrown.

        >>> with WatchLogFromStart("/dev/null") as watcher:
        ...     print("Just print something without waiting for a log line")
        Traceback (most recent call last):
          ...
        Exception: wait_for_*() was not called

        >>> with WatchLogFromHere("/dev/null") as watcher:
        ...     try:
        ...         watcher.wait_for_line("foo", timeout=0)
        ...     except TimeoutError:
        ...         pass
        ...     try:
        ...         watcher.wait_for_lines({"bar": 42}, timeout=0)
        ...     except TimeoutError:
        ...         pass
        Traceback (most recent call last):
          ...
        Exception: wait_for_*() was already called
        """
        self._fd = None  # type: Optional[TextIO]
        self._path = path
        self._wait_function_called = False

    def wait_for_line(self, string: str, timeout: int = 10) -> None:
        """
        Block execution until a line containing the provided `string` appears
        in the log file.  Return `None` once the line is found or raise a
        `TimeoutError` after `timeout` seconds (default: 10) if `string` does
        not appear in the log file (strings and regular expressions are
        supported).  (Catching this exception is discouraged as it indicates
        that the test code did not behave as expected.)

        Recommended use:

        ```python
        import isctest

        def test_foo(servers):
            with servers["ns1"].watch_log_from_here() as watcher:
                # ... do stuff here ...
                watcher.wait_for_line("foo bar")
        ```

        One of `wait_for_line()` or `wait_for_lines()` must be called exactly
        once for every `WatchLogFrom*` instance.

        >>> # For `WatchLogFromStart`, `wait_for_line()` returns without
        >>> # raising an exception as soon as the line being looked for appears
        >>> # anywhere in the file, no matter whether that happens before of
        >>> # after the `with` statement is reached.
        >>> import tempfile
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         retval = watcher.wait_for_line("foo", timeout=1)
        >>> print(retval)
        None
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         print("foo", file=file, flush=True)
        ...         retval = watcher.wait_for_line("foo", timeout=1)
        >>> print(retval)
        None

        >>> # For `WatchLogFromHere`, `wait_for_line()` only returns without
        >>> # raising an exception if the string being looked for appears in
        >>> # the log file after the `with` statement is reached.
        >>> import tempfile
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromHere(file.name) as watcher:
        ...         watcher.wait_for_line("foo", timeout=1) #doctest: +ELLIPSIS
        Traceback (most recent call last):
          ...
        TimeoutError: Timeout reached watching ...
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromHere(file.name) as watcher:
        ...         print("foo", file=file, flush=True)
        ...         retval = watcher.wait_for_line("foo", timeout=1)
        >>> print(retval)
        None
        """
        return self._wait_for({string: None}, timeout)

    def wait_for_lines(
        self, strings: Dict[Union[str, Pattern], Any], timeout: int = 10
    ) -> None:
        """
        Block execution until a line of interest appears in the log file.  This
        function is a "multi-match" variant of `wait_for_line()` which is
        useful when some action may cause several different (mutually
        exclusive) messages to appear in the log file.

        `strings` is a `dict` associating each string to look for with the
        value this function should return when that string is found in the log
        file (strings and regular expressions are supported).  If none of the
        `strings` being looked for appear in the log file after `timeout`
        seconds, a `TimeoutError` is raised.  (Catching this exception is
        discouraged as it indicates that the test code did not behave as
        expected.)

        Since `strings` is a `dict` and preserves key order (in CPython 3.6 as
        implementation detail, since 3.7 by language design), each line is
        checked against each key in order until the first match.  Values provided
        in the `strings` dictionary (i.e. values which this function is expected
        to return upon a successful match) can be of any type.

        Recommended use:

        ```python
        import isctest

        def test_foo(servers):
            triggers = {
                "message A": "value returned when message A is found",
                "message B": "value returned when message B is found",
            }
            with servers["ns1"].watch_log_from_here() as watcher:
                # ... do stuff here ...
                retval = watcher.wait_for_lines(triggers)
        ```

        One of `wait_for_line()` or `wait_for_lines()` must be called exactly
        once for every `WatchLogFromHere` instance.

        >>> # Different values must be returned depending on which line is
        >>> # found in the log file.
        >>> import tempfile
        >>> triggers = {"foo": 42, "bar": 1337}
        >>> with tempfile.NamedTemporaryFile("w") as file:
        ...     print("foo", file=file, flush=True)
        ...     with WatchLogFromStart(file.name) as watcher:
        ...         retval1 = watcher.wait_for_lines(triggers, timeout=1)
        ...     with WatchLogFromHere(file.name) as watcher:
        ...         print("bar", file=file, flush=True)
        ...         retval2 = watcher.wait_for_lines(triggers, timeout=1)
        >>> print(retval1)
        42
        >>> print(retval2)
        1337
        """
        return self._wait_for(strings, timeout)

    def _wait_for(self, patterns: Dict[Union[str, Pattern], Any], timeout: int) -> Any:
        """
        Block execution until one of the `strings` being looked for appears in
        the log file.  Raise a `TimeoutError` if none of the `strings` being
        looked for are found in the log file for `timeout` seconds.
        """
        if self._wait_function_called:
            raise WatchLogException("wait_for_*() was already called")
        self._wait_function_called = True
        if not self._fd:
            raise WatchLogException("No file to watch")
        leftover = ""
        assert timeout, "Do not use this class unless you want to WAIT for something."
        deadline = time.time() + timeout
        while time.time() < deadline:
            for line in self._fd.readlines():
                if line[-1] != "\n":
                    # Line is not completely written yet, buffer and keep on waiting
                    leftover += line
                else:
                    line = leftover + line
                    leftover = ""
                    for string, retval in patterns.items():
                        if isinstance(string, Pattern) and string.search(line):
                            return retval
                        if isinstance(string, str) and string in line:
                            return retval
            time.sleep(0.1)
        raise TimeoutError(
            "Timeout reached watching {} for {}".format(
                self._path, list(patterns.keys())
            )
        )

    def __enter__(self) -> Any:
        self._fd = open(self._path, encoding="utf-8")
        self._seek_on_enter()
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
        if self._fd:
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
        if self._fd:
            self._fd.seek(0, os.SEEK_END)
