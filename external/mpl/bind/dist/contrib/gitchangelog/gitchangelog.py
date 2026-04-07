#!/usr/bin/env python3
############################################################################
# Copyright (c) 2018, Valentin Lab
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Securactive nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL SECURACTIVE BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# SPDX-License-Identifier: BSD-3-Clause
#
############################################################################

from __future__ import print_function
from __future__ import absolute_import

import locale
import re
import os
import os.path
import sys
import glob
import textwrap
import datetime
import collections
import traceback
import contextlib
import itertools
import errno

from subprocess import Popen, PIPE

try:
    import pystache
except ImportError:  ## pragma: no cover
    pystache = None

try:
    import mako
except ImportError:  ## pragma: no cover
    mako = None


__version__ = "%%version%%"  ## replaced by autogen.sh

EBUG = None


##
## Platform and python compatibility
##

PY_VERSION = float("%d.%d" % sys.version_info[0:2])
PY3 = PY_VERSION >= 3

try:
    basestring
except NameError:
    basestring = str  ## pylint: disable=redefined-builtin

WIN32 = sys.platform == "win32"
if WIN32:
    PLT_CFG = {
        "close_fds": False,
    }
else:
    PLT_CFG = {
        "close_fds": True,
    }

##
##
##

if WIN32 and not PY3:

    ## Sorry about the following, all this code is to ensure full
    ## compatibility with python 2.7 under windows about sending unicode
    ## command-line

    import ctypes
    import subprocess
    import _subprocess
    from ctypes import (
        byref,
        windll,
        c_char_p,
        c_wchar_p,
        c_void_p,
        Structure,
        sizeof,
        c_wchar,
        WinError,
    )
    from ctypes.wintypes import BYTE, WORD, LPWSTR, BOOL, DWORD, LPVOID, HANDLE

    ##
    ## Types
    ##

    CREATE_UNICODE_ENVIRONMENT = 0x00000400
    LPCTSTR = c_char_p
    LPTSTR = c_wchar_p
    LPSECURITY_ATTRIBUTES = c_void_p
    LPBYTE = ctypes.POINTER(BYTE)

    class STARTUPINFOW(Structure):
        _fields_ = [
            ("cb", DWORD),
            ("lpReserved", LPWSTR),
            ("lpDesktop", LPWSTR),
            ("lpTitle", LPWSTR),
            ("dwX", DWORD),
            ("dwY", DWORD),
            ("dwXSize", DWORD),
            ("dwYSize", DWORD),
            ("dwXCountChars", DWORD),
            ("dwYCountChars", DWORD),
            ("dwFillAtrribute", DWORD),
            ("dwFlags", DWORD),
            ("wShowWindow", WORD),
            ("cbReserved2", WORD),
            ("lpReserved2", LPBYTE),
            ("hStdInput", HANDLE),
            ("hStdOutput", HANDLE),
            ("hStdError", HANDLE),
        ]

    LPSTARTUPINFOW = ctypes.POINTER(STARTUPINFOW)

    class PROCESS_INFORMATION(Structure):
        _fields_ = [
            ("hProcess", HANDLE),
            ("hThread", HANDLE),
            ("dwProcessId", DWORD),
            ("dwThreadId", DWORD),
        ]

    LPPROCESS_INFORMATION = ctypes.POINTER(PROCESS_INFORMATION)

    class DUMMY_HANDLE(ctypes.c_void_p):

        def __init__(self, *a, **kw):
            super(DUMMY_HANDLE, self).__init__(*a, **kw)
            self.closed = False

        def Close(self):
            if not self.closed:
                windll.kernel32.CloseHandle(self)
                self.closed = True

        def __int__(self):
            return self.value

    CreateProcessW = windll.kernel32.CreateProcessW
    CreateProcessW.argtypes = [
        LPCTSTR,
        LPTSTR,
        LPSECURITY_ATTRIBUTES,
        LPSECURITY_ATTRIBUTES,
        BOOL,
        DWORD,
        LPVOID,
        LPCTSTR,
        LPSTARTUPINFOW,
        LPPROCESS_INFORMATION,
    ]
    CreateProcessW.restype = BOOL

    ##
    ## Patched functions/classes
    ##

    def CreateProcess(
        executable,
        args,
        _p_attr,
        _t_attr,
        inherit_handles,
        creation_flags,
        env,
        cwd,
        startup_info,
    ):
        """Create a process supporting unicode executable and args for win32

        Python implementation of CreateProcess using CreateProcessW for Win32

        """

        si = STARTUPINFOW(
            dwFlags=startup_info.dwFlags,
            wShowWindow=startup_info.wShowWindow,
            cb=sizeof(STARTUPINFOW),
            ## XXXvlab: not sure of the casting here to ints.
            hStdInput=int(startup_info.hStdInput),
            hStdOutput=int(startup_info.hStdOutput),
            hStdError=int(startup_info.hStdError),
        )

        wenv = None
        if env is not None:
            ## LPCWSTR seems to be c_wchar_p, so let's say CWSTR is c_wchar
            env = (
                unicode("").join([unicode("%s=%s\0") % (k, v) for k, v in env.items()])
            ) + unicode("\0")
            wenv = (c_wchar * len(env))()
            wenv.value = env

        pi = PROCESS_INFORMATION()
        creation_flags |= CREATE_UNICODE_ENVIRONMENT

        if CreateProcessW(
            executable,
            args,
            None,
            None,
            inherit_handles,
            creation_flags,
            wenv,
            cwd,
            byref(si),
            byref(pi),
        ):
            return (
                DUMMY_HANDLE(pi.hProcess),
                DUMMY_HANDLE(pi.hThread),
                pi.dwProcessId,
                pi.dwThreadId,
            )
        raise WinError()

    class Popen(subprocess.Popen):
        """This superseeds Popen and corrects a bug in cPython 2.7 implem"""

        def _execute_child(
            self,
            args,
            executable,
            preexec_fn,
            close_fds,
            cwd,
            env,
            universal_newlines,
            startupinfo,
            creationflags,
            shell,
            to_close,
            p2cread,
            p2cwrite,
            c2pread,
            c2pwrite,
            errread,
            errwrite,
        ):
            """Code from part of _execute_child from Python 2.7 (9fbb65e)

            There are only 2 little changes concerning the construction of
            the the final string in shell mode: we preempt the creation of
            the command string when shell is True, because original function
            will try to encode unicode args which we want to avoid to be able to
            sending it as-is to ``CreateProcess``.

            """
            if not isinstance(args, subprocess.types.StringTypes):
                args = subprocess.list2cmdline(args)

            if startupinfo is None:
                startupinfo = subprocess.STARTUPINFO()
            if shell:
                startupinfo.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
                startupinfo.wShowWindow = _subprocess.SW_HIDE
                comspec = os.environ.get("COMSPEC", unicode("cmd.exe"))
                args = unicode('{} /c "{}"').format(comspec, args)
                if (
                    _subprocess.GetVersion() >= 0x80000000
                    or os.path.basename(comspec).lower() == "command.com"
                ):
                    w9xpopen = self._find_w9xpopen()
                    args = unicode('"%s" %s') % (w9xpopen, args)
                    creationflags |= _subprocess.CREATE_NEW_CONSOLE

            super(Popen, self)._execute_child(
                args,
                executable,
                preexec_fn,
                close_fds,
                cwd,
                env,
                universal_newlines,
                startupinfo,
                creationflags,
                False,
                to_close,
                p2cread,
                p2cwrite,
                c2pread,
                c2pwrite,
                errread,
                errwrite,
            )

    _subprocess.CreateProcess = CreateProcess


##
## Help and usage strings
##

usage_msg = """
  %(exname)s {-h|--help}
  %(exname)s {-v|--version}
  %(exname)s [--debug|-d] [REVLIST]"""

description_msg = """\
Run this command in a git repository to output a formatted changelog
"""

epilog_msg = """\
%(exname)s uses a config file to filter meaningful commit or do some
formatting in commit messages thanks to a config file.

Config file location will be resolved in this order:
  - in shell environment variable GITCHANGELOG_CONFIG_FILENAME
  - in git configuration: ``git config gitchangelog.rc-path``
  - as '.%(exname)s.rc' in the root of the current git repository

"""


##
## Shell command helper functions
##


def stderr(msg):
    print(msg, file=sys.stderr)


def err(msg):
    stderr("Error: " + msg)


def warn(msg):
    stderr("Warning: " + msg)


def die(msg=None, errlvl=1):
    if msg:
        stderr(msg)
    sys.exit(errlvl)


class ShellError(Exception):

    def __init__(self, msg, errlvl=None, command=None, out=None, err=None):
        self.errlvl = errlvl
        self.command = command
        self.out = out
        self.err = err
        super(ShellError, self).__init__(msg)


@contextlib.contextmanager
def set_cwd(directory):
    curdir = os.getcwd()
    os.chdir(directory)
    try:
        yield
    finally:
        os.chdir(curdir)


def format_last_exception(prefix="  | "):
    """Format the last exception for display it in tests.

    This allows to raise custom exception, without loosing the context of what
    caused the problem in the first place:

    >>> def f():
    ...     raise Exception("Something terrible happened")
    >>> try:  ## doctest: +ELLIPSIS
    ...     f()
    ... except Exception:
    ...     formated_exception = format_last_exception()
    ...     raise ValueError('Oups, an error occured:\\n%s'
    ...         % formated_exception)
    Traceback (most recent call last):
    ...
    ValueError: Oups, an error occured:
      | Traceback (most recent call last):
    ...
      | Exception: Something terrible happened

    """

    return "\n".join(
        str(prefix + line) for line in traceback.format_exc().strip().split("\n")
    )


##
## config file functions
##

_config_env = {
    "WIN32": WIN32,
    "PY3": PY3,
}


def available_in_config(f):
    _config_env[f.__name__] = f
    return f


def load_config_file(filename, default_filename=None, fail_if_not_present=True):
    """Loads data from a config file."""

    config = _config_env.copy()
    for fname in [default_filename, filename]:
        if fname and os.path.exists(fname):
            if not os.path.isfile(fname):
                die("config file path '%s' exists but is not a file !" % (fname,))
            content = file_get_contents(fname)
            try:
                code = compile(content, fname, "exec")
                exec(code, config)  ## pylint: disable=exec-used
            except SyntaxError as e:
                die(
                    "Syntax error in config file: %s\n%s"
                    "File %s, line %i"
                    % (
                        str(e),
                        (indent(e.text.rstrip(), "  | ") + "\n") if e.text else "",
                        e.filename,
                        e.lineno,
                    )
                )
        else:
            if fail_if_not_present:
                die("%s config file is not found and is required." % (fname,))

    return config


##
## Text functions
##


@available_in_config
class TextProc(object):

    def __init__(self, fun):
        self.fun = fun
        if hasattr(fun, "__name__"):
            self.__name__ = fun.__name__

    def __call__(self, text):
        return self.fun(text)

    def __or__(self, value):
        if isinstance(value, TextProc):
            return TextProc(lambda text: value.fun(self.fun(text)))
        import inspect

        _frame, filename, lineno, _function_name, lines, _index = inspect.stack()[1]
        raise SyntaxError(
            "Invalid syntax in config file",
            (
                filename,
                lineno,
                0,
                "Invalid chain with a non TextProc element %r:\n%s"
                % (value, indent("".join(lines).strip(), "  | ")),
            ),
        )


def set_if_empty(text, msg="No commit message."):
    if len(text):
        return text
    return msg


@TextProc
def ucfirst(msg):
    if len(msg) == 0:
        return msg
    return msg[0].upper() + msg[1:]


@TextProc
def final_dot(msg):
    if len(msg) and msg[-1].isalnum():
        return msg + "."
    return msg


def indent(text, chars="  ", first=None):
    """Return text string indented with the given chars

    >>> string = 'This is first line.\\nThis is second line\\n'

    >>> print(indent(string, chars="| "))  # doctest: +NORMALIZE_WHITESPACE
    | This is first line.
    | This is second line
    |

    >>> print(indent(string, first="- "))  # doctest: +NORMALIZE_WHITESPACE
    - This is first line.
      This is second line


    >>> string = 'This is first line.\\n\\nThis is second line'
    >>> print(indent(string, first="- "))  # doctest: +NORMALIZE_WHITESPACE
    - This is first line.
    <BLANKLINE>
      This is second line

    """
    if first:
        first_line = text.split("\n")[0]
        rest = "\n".join(text.split("\n")[1:])
        return "\n".join([(first + first_line).rstrip(), indent(rest, chars=chars)])
    return "\n".join([(chars + line).rstrip() for line in text.split("\n")])


def paragraph_wrap(text, regexp="\n\n", separator="\n"):
    r"""Wrap text by making sure that paragraph are separated correctly

        >>> string = 'This is first paragraph which is quite long don\'t you \
        ... think ? Well, I think so.\n\nThis is second paragraph\n'

        >>> print(paragraph_wrap(string)) # doctest: +NORMALIZE_WHITESPACE
        This is first paragraph which is quite long don't you think ? Well, I
        think so.
        This is second paragraph

    Notice that that each paragraph has been wrapped separately.

    """
    regexp = re.compile(regexp, re.MULTILINE)
    return separator.join(
        "\n".join(textwrap.wrap(paragraph.strip(), break_on_hyphens=False))
        for paragraph in regexp.split(text)
    ).strip()


def curryfy(f):
    return lambda *a, **kw: TextProc(lambda txt: f(txt, *a, **kw))


## these are curryfied version of their lower case definition

Indent = curryfy(indent)
Wrap = curryfy(paragraph_wrap)
ReSub = lambda p, r, **k: TextProc(lambda txt: re.sub(p, r, txt, **k))
noop = TextProc(lambda txt: txt)
strip = TextProc(lambda txt: txt.strip())
SetIfEmpty = curryfy(set_if_empty)

for _label in (
    "Indent",
    "Wrap",
    "ReSub",
    "noop",
    "final_dot",
    "ucfirst",
    "strip",
    "SetIfEmpty",
):
    _config_env[_label] = locals()[_label]

##
## File
##


def file_get_contents(filename):
    with open(filename) as f:
        out = f.read()
    if not PY3:
        if not isinstance(out, unicode):
            out = out.decode(_preferred_encoding)
        ## remove encoding declaration (for some reason, python 2.7
        ## don't like it).
        out = re.sub(
            r"^(\s*#.*\s*)coding[:=]\s*([-\w.]+\s*;?\s*)", r"\1", out, re.DOTALL
        )

    return out


def file_put_contents(filename, string):
    """Write string to filename."""
    if PY3:
        fopen = open(filename, "w", newline="")
    else:
        fopen = open(filename, "wb")

    with fopen as f:
        f.write(string)


##
## Inferring revision
##


def _file_regex_match(filename, pattern, **kw):
    if not os.path.isfile(filename):
        raise IOError("Can't open file '%s'." % filename)
    file_content = file_get_contents(filename)
    match = re.search(pattern, file_content, **kw)
    if match is None:
        stderr("file content: %r" % file_content)
        if isinstance(pattern, type(re.compile(""))):
            pattern = pattern.pattern
        raise ValueError(
            "Regex %s did not match any substring in '%s'." % (pattern, filename)
        )
    return match


@available_in_config
def FileFirstRegexMatch(filename, pattern):
    def _call():
        match = _file_regex_match(filename, pattern)
        dct = match.groupdict()
        if dct:
            if "rev" not in dct:
                warn(
                    "Named pattern used, but no one are named 'rev'. "
                    "Using full match."
                )
                return match.group(0)
            if dct["rev"] is None:
                die("Named pattern used, but it was not valued.")
            return dct["rev"]
        return match.group(0)

    return _call


@available_in_config
def Caret(l):
    def _call():
        return "^%s" % eval_if_callable(l)

    return _call


##
## System functions
##

## Note that locale.getpreferredencoding() does NOT follow
## PYTHONIOENCODING by default, but ``sys.stdout.encoding`` does. In
## PY2, ``sys.stdout.encoding`` without PYTHONIOENCODING set does not
## get any values set in subshells.  However, if _preferred_encoding
## is not set to utf-8, it leads to encoding errors.
_preferred_encoding = (
    os.environ.get("PYTHONIOENCODING") or locale.getpreferredencoding()
)
DEFAULT_GIT_LOG_ENCODING = "utf-8"


class Phile(object):
    """File like API to read fields separated by any delimiters

    It'll take care of file decoding to unicode.

    This is an adaptor on a file object.

        >>> if PY3:
        ...     from io import BytesIO
        ...     def File(s):
        ...         _obj = BytesIO()
        ...         _obj.write(s.encode(_preferred_encoding))
        ...         _obj.seek(0)
        ...         return _obj
        ... else:
        ...     from cStringIO import StringIO as File

        >>> f = Phile(File("a-b-c-d"))

    Read provides an iterator:

        >>> def show(l):
        ...     print(", ".join(l))
        >>> show(f.read(delimiter="-"))
        a, b, c, d

    You can change the buffersize loaded into memory before outputing
    your changes. It should not change the iterator output:

        >>> f = Phile(File("é-à-ü-d"), buffersize=3)
        >>> len(list(f.read(delimiter="-")))
        4

        >>> f = Phile(File("foo-bang-yummy"), buffersize=3)
        >>> show(f.read(delimiter="-"))
        foo, bang, yummy

        >>> f = Phile(File("foo-bang-yummy"), buffersize=1)
        >>> show(f.read(delimiter="-"))
        foo, bang, yummy

    """

    def __init__(self, filename, buffersize=4096, encoding=_preferred_encoding):
        self._file = filename
        self._buffersize = buffersize
        self._encoding = encoding

    def read(self, delimiter="\n"):
        buf = ""
        if PY3:
            delimiter = delimiter.encode(_preferred_encoding)
            buf = buf.encode(_preferred_encoding)
        while True:
            chunk = self._file.read(self._buffersize)
            if not chunk:
                yield buf.decode(self._encoding)
                return
            records = chunk.split(delimiter)
            records[0] = buf + records[0]
            for record in records[:-1]:
                yield record.decode(self._encoding)
            buf = records[-1]

    def write(self, buf):
        if PY3:
            buf = buf.encode(self._encoding)
        return self._file.write(buf)

    def close(self):
        return self._file.close()


class Proc(Popen):

    def __init__(self, command, env=None, encoding=_preferred_encoding):
        super(Proc, self).__init__(
            command,
            shell=True,
            stdin=PIPE,
            stdout=PIPE,
            stderr=PIPE,
            close_fds=PLT_CFG["close_fds"],
            env=env,
            universal_newlines=False,
        )

        self.stdin = Phile(self.stdin, encoding=encoding)
        self.stdout = Phile(self.stdout, encoding=encoding)
        self.stderr = Phile(self.stderr, encoding=encoding)


def cmd(command, env=None, shell=True):

    p = Popen(
        command,
        shell=shell,
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        close_fds=PLT_CFG["close_fds"],
        env=env,
        universal_newlines=False,
    )
    out, err = p.communicate()
    return (
        out.decode(getattr(sys.stdout, "encoding", None) or _preferred_encoding),
        err.decode(getattr(sys.stderr, "encoding", None) or _preferred_encoding),
        p.returncode,
    )


@available_in_config
def wrap(command, ignore_errlvls=[0], env=None, shell=True):
    """Wraps a shell command and casts an exception on unexpected errlvl

    >>> wrap('/tmp/lsdjflkjf') # doctest: +ELLIPSIS +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
    ...
    ShellError: Wrapped command '/tmp/lsdjflkjf' exited with errorlevel 127.
      stderr:
      | /bin/sh: .../tmp/lsdjflkjf: not found

    >>> print(wrap('echo hello'),  end='')
    hello

    >>> print(wrap('echo hello && false'),
    ...       end='')  # doctest: +ELLIPSIS +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
    ...
    ShellError: Wrapped command 'echo hello && false' exited with errorlevel 1.
      stdout:
      | hello

    """

    out, err, errlvl = cmd(command, env=env, shell=shell)

    if errlvl not in ignore_errlvls:

        formatted = []
        if out:
            if out.endswith("\n"):
                out = out[:-1]
            formatted.append("stdout:\n%s" % indent(out, "| "))
        if err:
            if err.endswith("\n"):
                err = err[:-1]
            formatted.append("stderr:\n%s" % indent(err, "| "))
        msg = "\n".join(formatted)

        raise ShellError(
            "Wrapped command %r exited with errorlevel %d.\n%s"
            % (command, errlvl, indent(msg, chars="  ")),
            errlvl=errlvl,
            command=command,
            out=out,
            err=err,
        )
    return out


@available_in_config
def swrap(command, **kwargs):
    """Same as ``wrap(...)`` but strips the output."""

    return wrap(command, **kwargs).strip()


##
## git information access
##


class SubGitObjectMixin(object):

    def __init__(self, repos):
        self._repos = repos

    @property
    def git(self):
        """Simple delegation to ``repos`` original method."""
        return self._repos.git


GIT_FORMAT_KEYS = {
    "sha1": "%H",
    "sha1_short": "%h",
    "subject": "%s",
    "author_name": "%an",
    "author_email": "%ae",
    "author_date": "%ad",
    "author_date_timestamp": "%at",
    "committer_name": "%cn",
    "committer_date_timestamp": "%ct",
    "raw_body": "%B",
    "body": "%b",
}

GIT_FULL_FORMAT_STRING = "%x00".join(GIT_FORMAT_KEYS.values())

REGEX_RFC822_KEY_VALUE = (
    r"(^|\n)(?P<key>[A-Z]\w+(-\w+)*): (?P<value>[^\n]*(\n\s+[^\n]*)*)"
)
REGEX_RFC822_POSTFIX = r"(%s)+$" % REGEX_RFC822_KEY_VALUE


class GitCommit(SubGitObjectMixin):
    r"""Represent a Git Commit and expose through its attribute many information

    Let's create a fake GitRepos:

        >>> from minimock import Mock
        >>> repos = Mock("gitRepos")

    Initialization:

        >>> repos.git = Mock("gitRepos.git")
        >>> repos.git.log.mock_returns_func = \
        ...     lambda *a, **kwargs: "\x00".join([{
        ...             'sha1': "000000",
        ...             'sha1_short': "000",
        ...             'subject': SUBJECT,
        ...             'author_name': "John Smith",
        ...             'author_date': "Tue Feb 14 20:31:22 2017 +0700",
        ...             'author_email': "john.smith@example.com",
        ...             'author_date_timestamp': "0",   ## epoch
        ...             'committer_name': "Alice Wang",
        ...             'committer_date_timestamp': "0", ## epoch
        ...             'raw_body': "my subject\n\n%s" % BODY,
        ...             'body': BODY,
        ...         }[key] for key in GIT_FORMAT_KEYS.keys()])
        >>> repos.git.rev_list.mock_returns = "123456"

    Query, by attributes or items:

        >>> SUBJECT = "fee fie foh"
        >>> BODY = "foo foo foo"

        >>> head = GitCommit(repos, "HEAD")
        >>> head.subject
        Called gitRepos.git.log(...'HEAD'...)
        'fee fie foh'
        >>> head.author_name
        'John Smith'

    Notice that on the second call, there's no need to call again git log as
    all the values have already been computed.

    Trailer
    =======

    ``GitCommit`` offers a simple direct API to trailer values. These
    are like RFC822's header value but are at the end of body:

        >>> BODY = '''\
        ... Stuff in the body
        ... Change-id: 1234
        ... Value-X: Supports multi
        ...   line values'''

        >>> head = GitCommit(repos, "HEAD")
        >>> head.trailer_change_id
        Called gitRepos.git.log(...'HEAD'...)
        '1234'
        >>> head.trailer_value_x
        'Supports multi\nline values'

    Notice how the multi-line value was unindented.
    In case of multiple values, these are concatened in lists:

        >>> BODY = '''\
        ... Stuff in the body
        ... Co-Authored-By: Bob
        ... Co-Authored-By: Alice
        ... Co-Authored-By: Jack
        ... '''

        >>> head = GitCommit(repos, "HEAD")
        >>> head.trailer_co_authored_by
        Called gitRepos.git.log(...'HEAD'...)
        ['Bob', 'Alice', 'Jack']


    Special values
    ==============

    Authors
    -------

        >>> BODY = '''\
        ... Stuff in the body
        ... Co-Authored-By: Bob
        ... Co-Authored-By: Alice
        ... Co-Authored-By: Jack
        ... '''

        >>> head = GitCommit(repos, "HEAD")
        >>> head.author_names
        Called gitRepos.git.log(...'HEAD'...)
        ['Alice', 'Bob', 'Jack', 'John Smith']

    Notice that they are printed in alphabetical order.

    """

    def __init__(self, repos, identifier):
        super(GitCommit, self).__init__(repos)
        self.identifier = identifier
        self._trailer_parsed = False

    def __getattr__(self, label):
        """Completes commits attributes upon request."""
        attrs = GIT_FORMAT_KEYS.keys()
        if label not in attrs:
            try:
                return self.__dict__[label]
            except KeyError:
                if self._trailer_parsed:
                    raise AttributeError(label)

        identifier = self.identifier

        ## Compute only missing information
        missing_attrs = [l for l in attrs if l not in self.__dict__]
        ## some commit can be already fully specified (see ``mk_commit``)
        if missing_attrs:
            aformat = "%x00".join(GIT_FORMAT_KEYS[l] for l in missing_attrs)
            try:
                ret = self.git.log(
                    [identifier, "--max-count=1", "--pretty=format:%s" % aformat, "--"]
                )
            except ShellError:
                if DEBUG:
                    raise
                raise ValueError(
                    "Given commit identifier %r doesn't exists" % self.identifier
                )
            attr_values = ret.split("\x00")
            for attr, value in zip(missing_attrs, attr_values):
                setattr(self, attr, value.strip())

        ## Let's interpret RFC822-like header keys that could be in the body
        match = re.search(REGEX_RFC822_POSTFIX, self.body)
        if match is not None:
            pos = match.start()
            postfix = self.body[pos:]
            self.body = self.body[:pos]
            for match in re.finditer(REGEX_RFC822_KEY_VALUE, postfix):
                dct = match.groupdict()
                key = dct["key"].replace("-", "_").lower()
                if "\n" in dct["value"]:
                    first_line, remaining = dct["value"].split("\n", 1)
                    value = "%s\n%s" % (first_line, textwrap.dedent(remaining))
                else:
                    value = dct["value"]
                try:
                    prev_value = self.__dict__["trailer_%s" % key]
                except KeyError:
                    setattr(self, "trailer_%s" % key, value)
                else:
                    setattr(
                        self,
                        "trailer_%s" % key,
                        (
                            prev_value
                            + [
                                value,
                            ]
                            if isinstance(prev_value, list)
                            else [
                                prev_value,
                                value,
                            ]
                        ),
                    )
        self._trailer_parsed = True
        return getattr(self, label)

    @property
    def author_names(self):
        return [
            re.sub(r"^([^<]+)<[^>]+>\s*$", r"\1", author).strip()
            for author in self.authors
        ]

    @property
    def authors(self):
        co_authors = getattr(self, "trailer_co_authored_by", [])
        co_authors = co_authors if isinstance(co_authors, list) else [co_authors]
        return sorted(co_authors + ["%s <%s>" % (self.author_name, self.author_email)])

    @property
    def date(self):
        d = datetime.datetime.utcfromtimestamp(float(self.author_date_timestamp))
        return d.strftime("%Y-%m-%d")

    @property
    def has_annotated_tag(self):
        try:
            self.git.rev_parse(["%s^{tag}" % self.identifier, "--"])
            return True
        except ShellError as e:
            if e.errlvl != 128:
                raise
            return False

    @property
    def tagger_date_timestamp(self):
        if not self.has_annotated_tag:
            raise ValueError(
                "Can't access 'tagger_date_timestamp' on commit without annotated tag."
            )
        tagger_date_utc = self.git.for_each_ref(
            "refs/tags/%s" % self.identifier, format="%(taggerdate:raw)"
        )
        return tagger_date_utc.split(" ", 1)[0]

    @property
    def tagger_date(self):
        d = datetime.datetime.fromtimestamp(
            float(self.tagger_date_timestamp), datetime.UTC
        )
        return d.strftime("%Y-%m-%d")

    def __le__(self, value):
        if not isinstance(value, GitCommit):
            value = self._repos.commit(value)
        try:
            self.git.merge_base(value.sha1, is_ancestor=self.sha1)
            return True
        except ShellError as e:
            if e.errlvl != 1:
                raise
            return False

    def __lt__(self, value):
        if not isinstance(value, GitCommit):
            value = self._repos.commit(value)
        return self <= value and self != value

    def __eq__(self, value):
        if not isinstance(value, GitCommit):
            value = self._repos.commit(value)
        return self.sha1 == value.sha1

    def __hash__(self):
        return hash(self.sha1)

    def __repr__(self):
        return "<%s %r>" % (self.__class__.__name__, self.identifier)


def normpath(path, cwd=None):
    """path can be absolute or relative, if relative it uses the cwd given as
    param.

    """
    if os.path.isabs(path):
        return path
    cwd = cwd if cwd else os.getcwd()
    return os.path.normpath(os.path.join(cwd, path))


class GitConfig(SubGitObjectMixin):
    """Interface to config values of git

    Let's create a fake GitRepos:

        >>> from minimock import Mock
        >>> repos = Mock("gitRepos")

    Initialization:

        >>> cfg = GitConfig(repos)

    Query, by attributes or items:

        >>> repos.git.config.mock_returns = "bar"
        >>> cfg.foo
        Called gitRepos.git.config('foo')
        'bar'
        >>> cfg["foo"]
        Called gitRepos.git.config('foo')
        'bar'
        >>> cfg.get("foo")
        Called gitRepos.git.config('foo')
        'bar'
        >>> cfg["foo.wiz"]
        Called gitRepos.git.config('foo.wiz')
        'bar'

    Notice that you can't use attribute search in subsection as ``cfg.foo.wiz``
    That's because in git config files, you can have a value attached to
    an element, and this element can also be a section.

    Nevertheless, you can do:

        >>> getattr(cfg, "foo.wiz")
        Called gitRepos.git.config('foo.wiz')
        'bar'

    Default values
    --------------

    get item, and getattr default values can be used:

        >>> del repos.git.config.mock_returns
        >>> repos.git.config.mock_raises = ShellError('Key not found',
        ...                                           errlvl=1, out="", err="")

        >>> getattr(cfg, "foo", "default")
        Called gitRepos.git.config('foo')
        'default'

        >>> cfg["foo"]  ## doctest: +ELLIPSIS
        Traceback (most recent call last):
        ...
        KeyError: 'foo'

        >>> getattr(cfg, "foo")  ## doctest: +ELLIPSIS
        Traceback (most recent call last):
        ...
        AttributeError...

        >>> cfg.get("foo", "default")
        Called gitRepos.git.config('foo')
        'default'

        >>> print("%r" % cfg.get("foo"))
        Called gitRepos.git.config('foo')
        None

    """

    def __init__(self, repos):
        super(GitConfig, self).__init__(repos)

    def __getattr__(self, label):
        try:
            res = self.git.config(label)
        except ShellError as e:
            if e.errlvl == 1 and e.out == "":
                raise AttributeError("key %r is not found in git config." % label)
            raise
        return res

    def get(self, label, default=None):
        return getattr(self, label, default)

    def __getitem__(self, label):
        try:
            return getattr(self, label)
        except AttributeError:
            raise KeyError(label)


class GitCmd(SubGitObjectMixin):

    def __getattr__(self, label):
        label = label.replace("_", "-")

        def dir_swrap(command, **kwargs):
            with set_cwd(self._repos._orig_path):
                return swrap(command, **kwargs)

        def method(*args, **kwargs):
            if len(args) == 1 and not isinstance(args[0], basestring):
                return dir_swrap(
                    [
                        "git",
                        label,
                    ]
                    + args[0],
                    shell=False,
                    env=kwargs.get("env", None),
                )
            cli_args = []
            for key, value in kwargs.items():
                cli_key = ("-%s" if len(key) == 1 else "--%s") % key.replace("_", "-")
                if isinstance(value, bool):
                    cli_args.append(cli_key)
                else:
                    cli_args.append(cli_key)
                    cli_args.append(value)

            cli_args.extend(args)

            return dir_swrap(
                [
                    "git",
                    label,
                ]
                + cli_args,
                shell=False,
            )

        return method


class GitRepos(object):

    def __init__(self, path):

        ## Saving this original path to ensure all future git commands
        ## will be done from this location.
        self._orig_path = os.path.abspath(path)

        ## verify ``git`` command is accessible:
        try:
            self._git_version = self.git.version()
        except ShellError:
            if DEBUG:
                raise
            raise EnvironmentError(
                "Required ``git`` command not found or broken in $PATH. "
                "(calling ``git version`` failed.)"
            )

        ## verify that we are in a git repository
        try:
            self.git.remote()
        except ShellError:
            if DEBUG:
                raise
            raise EnvironmentError(
                "Not in a git repository. (calling ``git remote`` failed.)"
            )

        self.bare = self.git.rev_parse(is_bare_repository=True) == "true"
        self.toplevel = None if self.bare else self.git.rev_parse(show_toplevel=True)
        self.gitdir = normpath(self.git.rev_parse(git_dir=True), cwd=self._orig_path)

    @classmethod
    def create(cls, directory, *args, **kwargs):
        os.mkdir(directory)
        return cls.init(directory, *args, **kwargs)

    @classmethod
    def init(cls, directory, user=None, email=None):
        with set_cwd(directory):
            wrap("git init .")
        self = cls(directory)
        if user:
            self.git.config("user.name", user)
        if email:
            self.git.config("user.email", email)
        return self

    def commit(self, identifier):
        return GitCommit(self, identifier)

    @property
    def git(self):
        return GitCmd(self)

    @property
    def config(self):
        return GitConfig(self)

    def tags(self, contains=None):
        """String list of repository's tag names

        Current tag order is committer date timestamp of tagged commit.
        No firm reason for that, and it could change in future version.

        """
        if contains:
            tags = self.git.tag(contains=contains).split("\n")
        else:
            tags = self.git.tag().split("\n")
        ## Should we use new version name sorting ?  refering to :
        ## ``git tags --sort -v:refname`` in git version >2.0.
        ## Sorting and reversing with command line is not available on
        ## git version <2.0
        return sorted(
            [self.commit(tag) for tag in tags if tag != ""],
            key=lambda x: int(x.committer_date_timestamp),
        )

    def log(
        self,
        includes=[
            "HEAD",
        ],
        excludes=[],
        include_merge=True,
        encoding=_preferred_encoding,
    ):
        """Reverse chronological list of git repository's commits

        Note: rev lists can be GitCommit instance list or identifier list.

        """

        refs = {"includes": includes, "excludes": excludes}
        for ref_type in ("includes", "excludes"):
            for idx, ref in enumerate(refs[ref_type]):
                if not isinstance(ref, GitCommit):
                    refs[ref_type][idx] = self.commit(ref)

        ## --topo-order: don't mix commits from separate branches.
        plog = Proc(
            "git log --stdin -z --topo-order --pretty=format:%s %s --"
            % (GIT_FULL_FORMAT_STRING, "--no-merges" if not include_merge else ""),
            encoding=encoding,
        )
        for ref in refs["includes"]:
            plog.stdin.write("%s\n" % ref.sha1)

        for ref in refs["excludes"]:
            plog.stdin.write("^%s\n" % ref.sha1)
        plog.stdin.close()

        def mk_commit(dct):
            """Creates an already set commit from a dct"""
            c = self.commit(dct["sha1"])
            for k, v in dct.items():
                setattr(c, k, v)
            return c

        values = plog.stdout.read("\x00")

        try:
            while True:  ## next(values) will eventualy raise a StopIteration
                yield mk_commit(dict([(key, next(values)) for key in GIT_FORMAT_KEYS]))
        except StopIteration:
            pass  ## since 3.7, we are not allowed anymore to trickle down
            ## StopIteration.
        finally:
            plog.stdout.close()
            plog.stderr.close()


def first_matching(section_regexps, string):
    for section, regexps in section_regexps:
        if regexps is None:
            return section
        for regexp in regexps:
            if re.search(regexp, string) is not None:
                return section


def ensure_template_file_exists(label, template_name):
    """Return template file path given a label hint and the template name

    Template name can be either a filename with full path,
    if this is the case, the label is of no use.

    If ``template_name`` does not refer to an existing file,
    then ``label`` is used to find a template file in the
    the bundled ones.

    """

    try:
        template_path = GitRepos(os.getcwd()).config.get("gitchangelog.template-path")
    except ShellError as e:
        stderr(
            "Error parsing git config: %s."
            " Won't be able to read 'template-path' if defined." % (str(e))
        )
        template_path = None

    if template_path:
        path_file = path_label = template_path
    else:
        path_file = os.getcwd()
        path_label = os.path.join(
            os.path.dirname(os.path.realpath(__file__)), "templates", label
        )

    for ftn in [
        os.path.join(path_file, template_name),
        os.path.join(path_label, "%s.tpl" % template_name),
    ]:
        if os.path.isfile(ftn):
            return ftn

    templates = glob.glob(os.path.join(path_label, "*.tpl"))
    if len(templates) > 0:
        msg = "These are the available %s templates:" % label
        msg += "\n - " + "\n - ".join(
            os.path.basename(f).split(".")[0] for f in templates
        )
        msg += "\nTemplates are located in %r" % path_label
    else:
        msg = "No available %s templates found in %r." % (label, path_label)
    die("Error: Invalid %s template name %r.\n" % (label, template_name) + "%s" % msg)


##
## Output Engines
##


@available_in_config
def rest_py(data, opts={}):
    """Returns ReStructured Text changelog content from data"""

    def rest_title(label, char="="):
        return (label.strip() + "\n") + (char * len(label) + "\n\n")

    def render_version(version):
        title = (
            "%s (%s)" % (version["tag"], version["date"])
            if version["tag"]
            else opts["unreleased_version_label"]
        )
        s = rest_title(title, char="-")

        sections = version["sections"]
        nb_sections = len(sections)
        for section in sections:

            section_label = section["label"] if section.get("label", None) else "Other"

            if not (section_label == "Other" and nb_sections == 1):
                s += rest_title(section_label, "~")

            for commit in section["commits"]:
                s += render_commit(commit, opts)
        return s

    def render_commit(commit, opts=opts):
        subject = commit["subject"]

        if opts["include_commit_sha"]:
            subject += " ``%s``" % commit["commit"].sha1_short

        entry = (
            indent(
                "\n".join(textwrap.wrap(subject, break_on_hyphens=False)), first="- "
            ).strip()
            + "\n"
        )

        if commit["body"]:
            entry += "\n" + indent(commit["body"])
            entry += "\n"

        entry += "\n"

        return entry

    if data["title"]:
        yield rest_title(data["title"], char="=") + "\n"

    for version in data["versions"]:
        if len(version["sections"]) > 0:
            yield render_version(version) + "\n"


## formatter engines

if pystache:

    @available_in_config
    def mustache(template_name):
        """Return a callable that will render a changelog data structure

        returned callable must take 2 arguments ``data`` and ``opts``.

        """
        template_path = ensure_template_file_exists("mustache", template_name)

        template = file_get_contents(template_path)

        def stuffed_versions(versions, opts):
            for version in versions:
                title = (
                    "%s (%s)" % (version["tag"], version["date"])
                    if version["tag"]
                    else opts["unreleased_version_label"]
                )
                version["label"] = title
                version["label_chars"] = list(version["label"])
                for section in version["sections"]:
                    section["label_chars"] = list(section["label"])
                    section["display_label"] = not (
                        section["label"] == "Other" and len(version["sections"]) == 1
                    )
                    for commit in section["commits"]:
                        commit["author_names_joined"] = ", ".join(commit["authors"])
                        commit["body_indented"] = indent(commit["body"])
                yield version

        def renderer(data, opts):

            ## mustache is very simple so we need to add some intermediate
            ## values
            data["general_title"] = True if data["title"] else False
            data["title_chars"] = list(data["title"]) if data["title"] else []

            data["versions"] = stuffed_versions(data["versions"], opts)

            return pystache.render(template, data)

        return renderer

else:

    @available_in_config
    def mustache(template_name):  ## pylint: disable=unused-argument
        die("Required 'pystache' python module not found.")


if mako:

    import mako.template  ## pylint: disable=wrong-import-position

    mako_env = dict(
        (f.__name__, f) for f in (ucfirst, indent, textwrap, paragraph_wrap)
    )

    @available_in_config
    def makotemplate(template_name):
        """Return a callable that will render a changelog data structure

        returned callable must take 2 arguments ``data`` and ``opts``.

        """
        template_path = ensure_template_file_exists("mako", template_name)

        template = mako.template.Template(filename=template_path)

        def renderer(data, opts):
            kwargs = mako_env.copy()
            kwargs.update({"data": data, "opts": opts})
            return template.render(**kwargs)

        return renderer

else:

    @available_in_config
    def makotemplate(template_name):  ## pylint: disable=unused-argument
        die("Required 'mako' python module not found.")


##
## Publish action
##


@available_in_config
def stdout(content):
    for chunk in content:
        safe_print(chunk)


@available_in_config
def FileInsertAtFirstRegexMatch(filename, pattern, flags=0, idx=lambda m: m.start()):

    def write_content(f, content):
        for content_line in content:
            f.write(content_line)

    def _wrapped(content):
        index = idx(_file_regex_match(filename, pattern, flags=flags))
        offset = 0
        new_offset = 0
        postfix = False

        with open(filename + "~", "w") as dst:
            with open(filename, "r") as src:
                for line in src:
                    if postfix:
                        dst.write(line)
                        continue
                    new_offset = offset + len(line)
                    if new_offset < index:
                        offset = new_offset
                        dst.write(line)
                        continue
                    dst.write(line[0 : index - offset])
                    write_content(dst, content)
                    dst.write(line[index - offset :])
                    postfix = True
            if not postfix:
                write_content(dst, content)
        if WIN32:
            os.remove(filename)
        os.rename(filename + "~", filename)

    return _wrapped


@available_in_config
def FileRegexSubst(filename, pattern, replace, flags=0):

    replace = re.sub(r"\\([0-9+])", r"\\g<\1>", replace)

    def _wrapped(content):
        src = file_get_contents(filename)
        ## Protect replacement pattern against the following expansion of '\o'
        src = re.sub(
            pattern,
            replace.replace(r"\o", "".join(content).replace("\\", "\\\\")),
            src,
            flags=flags,
        )
        if not PY3:
            src = src.encode(_preferred_encoding)
        file_put_contents(filename, src)

    return _wrapped


##
## Data Structure
##


def versions_data_iter(
    repository,
    revlist=None,
    ignore_regexps=[],
    section_regexps=[(None, "")],
    tag_filter_regexp=r"\d+\.\d+(\.\d+)?",
    include_merge=True,
    body_process=lambda x: x,
    subject_process=lambda x: x,
    log_encoding=DEFAULT_GIT_LOG_ENCODING,
    warn=warn,  ## Mostly used for test
):
    """Returns an iterator through versions data structures

    (see ``gitchangelog.rc.reference`` file for more info)

    :param repository: target ``GitRepos`` object
    :param revlist: list of strings that git log understands as revlist
    :param ignore_regexps: list of regexp identifying ignored commit messages
    :param section_regexps: regexps identifying sections
    :param tag_filter_regexp: regexp to match tags used as version
    :param include_merge: whether to include merge commits in the log or not
    :param body_process: text processing object to apply to body
    :param subject_process: text processing object to apply to subject
    :param log_encoding: the encoding used in git logs
    :param warn: callable to output warnings, mocked by tests

    :returns: iterator of versions data_structures

    """

    revlist = revlist or []

    ## Hash to speedup lookups
    versions_done = {}
    excludes = (
        [
            rev[1:]
            for rev in repository.git.rev_parse(
                [
                    "--rev-only",
                ]
                + revlist
                + [
                    "--",
                ]
            ).split("\n")
            if rev.startswith("^")
        ]
        if revlist
        else []
    )

    revs = repository.git.rev_list(*revlist).split("\n") if revlist else []
    revs = [rev for rev in revs if rev != ""]

    if revlist and not revs:
        die("No commits matching given revlist: %s" % (" ".join(revlist),))

    tags = [
        tag
        for tag in repository.tags(contains=revs[-1] if revs else None)
        if re.match(tag_filter_regexp, tag.identifier)
    ]

    tags.append(repository.commit("HEAD"))

    if revlist:
        max_rev = repository.commit(revs[0])
        new_tags = []
        for tag in tags:
            new_tags.append(tag)
            if max_rev <= tag:
                break
        tags = new_tags
    else:
        max_rev = tags[-1]

    section_order = [k for k, _v in section_regexps]

    tags = list(reversed(tags))

    ## Get the changes between tags (releases)
    for idx, tag in enumerate(tags):

        ## New version
        current_version = {
            "date": tag.tagger_date if tag.has_annotated_tag else tag.date,
            "commit_date": tag.date,
            "tagger_date": tag.tagger_date if tag.has_annotated_tag else None,
            "tag": tag.identifier if tag.identifier != "HEAD" else None,
            "commit": tag,
        }

        sections = collections.defaultdict(list)
        commits = repository.log(
            includes=[min(tag, max_rev)],
            excludes=tags[idx + 1 :] + excludes,
            include_merge=include_merge,
            encoding=log_encoding,
        )

        for commit in commits:
            if any(
                re.search(pattern, commit.subject) is not None
                for pattern in ignore_regexps
            ):
                continue

            body = body_process(commit.body)

            ## Extract gitlab issue number
            issue = None
            if match := re.search(r".*:gl:`#([0-9]+)`", body):
                issue = int(match.group(1))

            matched_section = first_matching(section_regexps, commit.subject)

            ## Finally storing the commit in the matching section

            sections[matched_section].append(
                {
                    "author": commit.author_name,
                    "authors": commit.author_names,
                    "subject": subject_process(commit.subject),
                    "body": body,
                    "commit": commit,
                    "issue": issue,
                }
            )

        ## Sort sections by issue number or title
        for section_key in sections.keys():
            sections[section_key].sort(
                key=lambda c: (
                    c["issue"] if c["issue"] is not None else sys.maxsize,
                    c["subject"],
                )
            )

        ## Flush current version
        current_version["sections"] = [
            {"label": k, "commits": sections[k]} for k in section_order if k in sections
        ]
        if len(current_version["sections"]) != 0:
            yield current_version
        versions_done[tag] = current_version


def changelog(
    output_engine=rest_py,
    unreleased_version_label="unreleased",
    include_commit_sha=False,
    warn=warn,  ## Mostly used for test
    **kwargs,
):
    """Returns a string containing the changelog of given repository

    This function returns a string corresponding to the template rendered with
    the changelog data tree.

    (see ``gitchangelog.rc.sample`` file for more info)

    For an exact list of arguments, see the arguments of
    ``versions_data_iter(..)``.

    :param unreleased_version_label: version label for untagged commits
    :param include_commit_sha: whether message should contain commit sha
    :param output_engine: callable to render the changelog data
    :param warn: callable to output warnings, mocked by tests

    :returns: content of changelog

    """

    opts = {
        "unreleased_version_label": unreleased_version_label,
        "include_commit_sha": include_commit_sha,
    }

    ## Setting main container of changelog elements
    title = None if kwargs.get("revlist") else "Changelog"
    data = {"title": title, "versions": []}

    versions = versions_data_iter(warn=warn, **kwargs)

    ## poke once in versions to know if there's at least one:
    try:
        first_version = next(versions)
    except StopIteration:
        die("Empty changelog. No commits were elected to be used as entry.")
    else:
        data["versions"] = itertools.chain([first_version], versions)

    return output_engine(data=data, opts=opts)


##
## Manage obsolete options
##

_obsolete_options_managers = []


def obsolete_option_manager(fun):
    _obsolete_options_managers.append(fun)


@obsolete_option_manager
def obsolete_replace_regexps(config):
    """This option was superseeded by the ``subject_process`` option.

    Each regex replacement you had could be translated in a
    ``ReSub(pattern, replace)`` in the ``subject_process`` pipeline.

    """
    if "replace_regexps" in config:
        for pattern, replace in config["replace_regexps"].items():
            config["subject_process"] = ReSub(pattern, replace) | config.get(
                "subject_process", ucfirst | final_dot
            )


@obsolete_option_manager
def obsolete_body_split_regexp(config):
    """This option was superseeded by the ``body_process`` option.

    The split regex can now be sent as a ``Wrap(regex)`` text process
    instruction in the ``body_process`` pipeline.

    """
    if "body_split_regex" in config:
        config["body_process"] = Wrap(config["body_split_regex"]) | config.get(
            "body_process", noop
        )


def manage_obsolete_options(config):
    for man in _obsolete_options_managers:
        man(config)


##
## Command line parsing
##


def parse_cmd_line(usage, description, epilog, exname, version):

    import argparse

    kwargs = dict(
        usage=usage,
        description=description,
        epilog="\n" + epilog,
        prog=exname,
        formatter_class=argparse.RawTextHelpFormatter,
    )

    try:
        parser = argparse.ArgumentParser(version=version, **kwargs)
    except TypeError:  ## compat with argparse from python 3.4
        parser = argparse.ArgumentParser(**kwargs)
        parser.add_argument(
            "-v",
            "--version",
            help="show program's version number and exit",
            action="version",
            version=version,
        )

    parser.add_argument(
        "-d",
        "--debug",
        help="Enable debug mode (show full tracebacks).",
        action="store_true",
        dest="debug",
    )
    parser.add_argument("revlist", nargs="*", action="store", default=[])

    ## Remove "show" as first argument for compatibility reason.

    argv = []
    for i, arg in enumerate(sys.argv[1:]):
        if arg.startswith("-"):
            argv.append(arg)
            continue
        if arg == "show":
            warn("'show' positional argument is deprecated.")
            argv += sys.argv[i + 2 :]
            break
        else:
            argv += sys.argv[i + 1 :]
            break

    return parser.parse_args(argv)


eval_if_callable = lambda v: v() if callable(v) else v


def get_revision(repository, config, opts):
    if opts.revlist:
        revs = opts.revlist
    else:
        revs = config.get("revs")
        if revs:
            revs = eval_if_callable(revs)
            if not isinstance(revs, list):
                die(
                    "Invalid type for 'revs' in config file. "
                    "A 'list' type is required, and a %r was given."
                    % type(revs).__name__
                )
            revs = [eval_if_callable(rev) for rev in revs]
        else:
            revs = []

    for rev in revs:
        if not isinstance(rev, basestring):
            die(
                "Invalid type for revision in revs list from config file. "
                "'str' type is required, and a %r was given." % type(rev).__name__
            )
        try:
            repository.git.rev_parse([rev, "--rev_only", "--"])
        except ShellError:
            if DEBUG:
                raise
            die("Revision %r is not valid." % rev)

    if revs == [
        "HEAD",
    ]:
        return []
    return revs


def get_log_encoding(repository, config):

    log_encoding = config.get("log_encoding", None)
    if log_encoding is None:
        try:
            log_encoding = repository.config.get("i18n.logOuputEncoding")
        except ShellError as e:
            warn(
                "Error parsing git config: %s."
                " Couldn't check if 'i18n.logOuputEncoding' was set." % (str(e))
            )

    ## Final defaults coming from git defaults
    return log_encoding or DEFAULT_GIT_LOG_ENCODING


##
## Config Manager
##


class Config(dict):

    def __getitem__(self, label):
        if label not in self.keys():
            die("Missing value in config file for key '%s'." % label)
        return super(Config, self).__getitem__(label)


##
## Safe print
##


def safe_print(content):
    if not PY3:
        if isinstance(content, unicode):
            content = content.encode(_preferred_encoding)

    try:
        print(content, end="")
        sys.stdout.flush()
    except UnicodeEncodeError:
        if DEBUG:
            raise
        ## XXXvlab: should use $COLUMNS in bash and for windows:
        ## http://stackoverflow.com/questions/14978548
        stderr(paragraph_wrap(textwrap.dedent("""\
            UnicodeEncodeError:
              There was a problem outputing the resulting changelog to
              your console.

              This probably means that the changelog contains characters
              that can't be translated to characters in your current charset
              (%s).
            """) % sys.stdout.encoding))
        if WIN32 and PY_VERSION < 3.6 and sys.stdout.encoding != "utf-8":
            ## As of PY 3.6, encoding is now ``utf-8`` regardless of
            ## PYTHONIOENCODING
            ## https://www.python.org/dev/peps/pep-0528/
            stderr(
                "  You might want to try to fix that by setting "
                "PYTHONIOENCODING to 'utf-8'."
            )
        exit(1)
    except IOError as e:
        if e.errno == 0 and not PY3 and WIN32:
            ## Yes, had a strange IOError Errno 0 after outputing string
            ## that contained UTF-8 chars on Windows and PY2.7
            pass  ## Ignoring exception
        elif (WIN32 and e.errno == 22) or (  ## Invalid argument
            not WIN32 and e.errno == errno.EPIPE
        ):  ## Broken Pipe
            ## Nobody is listening anymore to stdout it seems. Let's bailout.
            if PY3:
                try:
                    ## Called only to generate exception and have a chance at
                    ## ignoring it. Otherwise this happens upon exit, and gets
                    ## some error message printed on stderr.
                    sys.stdout.close()
                except BrokenPipeError:  ## expected outcome on linux
                    pass
                except OSError as e2:
                    if e2.errno != 22:  ## expected outcome on WIN32
                        raise
            ## Yay ! stdout is closed we can now exit safely.
            exit(0)
        else:
            raise


##
## Main
##


def main():

    global DEBUG
    ## Basic environment infos

    reference_config = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "gitchangelog.rc.reference"
    )

    basename = os.path.basename(sys.argv[0])
    if basename.endswith(".py"):
        basename = basename[:-3]

    debug_varname = "DEBUG_%s" % basename.upper()
    DEBUG = os.environ.get(debug_varname, False)

    i = lambda x: x % {"exname": basename}

    opts = parse_cmd_line(
        usage=i(usage_msg),
        description=i(description_msg),
        epilog=i(epilog_msg),
        exname=basename,
        version=__version__,
    )
    DEBUG = DEBUG or opts.debug

    try:
        repository = GitRepos(".")
    except EnvironmentError as e:
        if DEBUG:
            raise
        try:
            die(str(e))
        except Exception as e2:
            die(repr(e2))

    try:
        gc_rc = repository.config.get("gitchangelog.rc-path")
    except ShellError as e:
        stderr(
            "Error parsing git config: %s."
            " Won't be able to read 'rc-path' if defined." % (str(e))
        )
        gc_rc = None

    gc_rc = normpath(gc_rc, cwd=repository.toplevel) if gc_rc else None

    ## config file lookup resolution
    for enforce_file_existence, fun in [
        (True, lambda: os.environ.get("GITCHANGELOG_CONFIG_FILENAME")),
        (True, lambda: gc_rc),
        (
            False,
            lambda: (
                (os.path.join(repository.toplevel, ".%s.rc" % basename))
                if not repository.bare
                else None
            ),
        ),
    ]:
        changelogrc = fun()
        if changelogrc:
            if not os.path.exists(changelogrc):
                if enforce_file_existence:
                    die("File %r does not exists." % changelogrc)
                else:
                    continue  ## changelogrc valued, but file does not exists
            else:
                break

    ## config file may lookup for templates relative to the toplevel
    ## of git repository
    os.chdir(repository.toplevel)

    config = load_config_file(
        os.path.expanduser(changelogrc),
        default_filename=reference_config,
        fail_if_not_present=False,
    )

    config = Config(config)

    log_encoding = get_log_encoding(repository, config)
    revlist = get_revision(repository, config, opts)
    config["unreleased_version_label"] = eval_if_callable(
        config["unreleased_version_label"]
    )
    manage_obsolete_options(config)

    try:
        content = changelog(
            repository=repository,
            revlist=revlist,
            ignore_regexps=config["ignore_regexps"],
            section_regexps=config["section_regexps"],
            unreleased_version_label=config["unreleased_version_label"],
            include_commit_sha=config["include_commit_sha"],
            tag_filter_regexp=config["tag_filter_regexp"],
            output_engine=config.get("output_engine", rest_py),
            include_merge=config.get("include_merge", True),
            body_process=config.get("body_process", noop),
            subject_process=config.get("subject_process", noop),
            log_encoding=log_encoding,
        )

        if isinstance(content, basestring):
            content = content.splitlines(True)

        config.get("publish", stdout)(content)

    except KeyboardInterrupt:
        if DEBUG:
            err("Keyboard interrupt received while running '%s':" % (basename,))
            stderr(format_last_exception())
        else:
            err("Keyboard Interrupt. Bailing out.")
        exit(130)  ## Actual SIGINT as bash process convention.
    except Exception as e:  ## pylint: disable=broad-except
        if DEBUG:
            err("Exception while running '%s':" % (basename,))
            stderr(format_last_exception())
        else:
            message = "%s" % e
            err(message)
            stderr(
                "  (set %s environment variable, "
                "or use ``--debug`` to see full traceback)" % (debug_varname,)
            )
        exit(255)


##
## Launch program
##

if __name__ == "__main__":
    main()
