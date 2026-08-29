<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->

# BIND9 System Test Framework

This directory holds test environments for running bind9 system tests involving
multiple name servers.

Each system test directory holds a set of test scripts and configuration files
to test different parts of BIND.  The directories are named for the aspect of
BIND they test, for example:

```
dnssec/       DNSSEC tests
forward/      Forwarding tests
glue/         Glue handling tests
```

etc.

A system test directory name must start with an alphabetic character and may
only contain alphanumeric characters and underscores.  Use underscore as the
word separator; hyphens are not allowed — they are reserved for the temporary
directories and symlinks the test runner creates.

Typically each set of tests sets up 2-5 name servers and then performs one or
more tests against them.  Within the test subdirectory, each name server has a
separate subdirectory containing its configuration data.  These subdirectories
are named "nsN" or "ansN" (where N is a number between 1 and 11, e.g. ns1,
ans2 etc.)

The tests are completely self-contained and do not require access to the real
DNS.  Generally, one of the test servers (usually ns1) is set up as a root
nameserver and is listed in the hints file of the others.

For task-oriented recipes (adding a new test directory, writing a regression
reproducer, mocking a misbehaving server, setting up zones), see
[the cookbook](COOKBOOK.md).


## Running the Tests

### Building BIND

The system tests run the binaries from the build tree, so BIND must be built
first.  The test-only binaries and plugins the tests need are built as part
of the regular build:

```sh
autoreconf -fi
./configure
make -j
```

### Prerequisites

To run system tests, make sure you have the following dependencies installed:

- python3 (3.10 and newer)
- pytest (7.0 and newer)
- pytest-xdist
- perl (still needed by the test runner internals; some legacy tests
  additionally need the Net::DNS module and are skipped when it is missing)

The full list of required and optional python packages can be found in
[requirements.txt](requirements.txt) (it can be installed with
`pip3 install -r requirements.txt`).

### Network Setup

To enable all servers to run on the same machine, they bind to separate virtual
IP addresses on the loopback interface.  ns1 runs on 10.53.0.1, ns2 on
10.53.0.2, etc.  Before running any tests, you must set up these addresses by
running the command

```sh
sh ifconfig.sh up
```

as root.  The interfaces can be removed by executing the command:

```sh
sh ifconfig.sh down
```

... also as root.

The servers use unprivileged ports (above 1024) instead of the usual port 53,
so they can be run without root privileges once the interfaces have been set
up.

**Note for MacOS Users**

If you wish to make the interfaces survive across reboots, copy
org.isc.bind.system and org.isc.bind.system.plist to /Library/LaunchDaemons
then run

```sh
launchctl load /Library/LaunchDaemons/org.isc.bind.system.plist
```

... as root.

### Running All the System Tests

Issue a plain `pytest` command in this directory to execute all tests
sequentially.  To execute them in parallel instead, run:

```sh
pytest -n <number-of-workers>
```

Parallel execution requires pytest-xdist; `-n auto` uses one worker per CPU.

Alternately, using the make command is also supported:

```sh
make [-j numproc] test
```

### Running a Single Test

To run all test modules in a single system test directory, pass the directory
name to pytest:

```sh
pytest dns64
```

The utility script `./run.sh dns64` does the same thing.

To narrow the run down further, prefer pytest node IDs over `-k` matching —
they are exact:

```sh
pytest dnssec_py/tests_mixed_ds.py
pytest doth/tests_sslyze.py::test_sslyze_dot
```

Parametrized tests have the parameter ID in brackets, so a single case of a
parametrized test can be selected as:

```sh
pytest "dnssec_py/tests_nsec3_answer.py::test_nodata[ns2]"
```

The `-k` option selects tests by pattern matching:

```sh
pytest -k <test-name-or-pattern>
```

Beware that a `-k` pattern might pick up more tests than intended.  Use the
`--collect-only` option to check the list of tests which match your `-k`
pattern.

### rr

When running system tests, named can be run under the rr tool. rr records a
trace to the $system_test/nsX/named-Y/ directory, which can be later used to
replay named. To enable this, run pytest with the USE_RR environment variable
set.

### Test Artifacts

Each test module is executed inside a unique temporary directory which contains
all the artifacts from the test run. If the tests succeed, they are deleted by
default. To override this behaviour, pass `--noclean` to pytest.

The directory name starts with the system test name, followed by `-tmp-XXXXXX`,
i.e. `dns64-tmp-r07vei9s` for `dns64` test run. Since this name changes each
run, a convenience symlink that has a stable name is also created. It points to
the latest test artifacts directory and has a form of `dns64-sh_dns64`
(depending on the particular test module).

To clean up the temporary directories and symlinks, run `make clean-local` in
the system test directory.

The following test artifacts are typically available:

- pytest.log.txt: main log file with test output
- files generated by the test itself, e.g. output from "dig" and "rndc"
- files produced by named, other tools or helper scripts


## Writing System Tests

### File Overview

Tests are organized into system test directories which may hold one or more
test modules (python files). Each module may have multiple test cases. The
system test directories may contain the following standard files:

- `tests_*.py`: These python files are picked up by pytest as modules. If they
  contain any test functions, they're added to the test suite.

- `*.j2`: Jinja2 templates, rendered automatically during test setup (see
  [Templates](#templates) below).

- `ns<N>`: These subdirectories contain test name servers that can be queried
  or can interact with each other. The value of N indicates the address the
  server listens on: for example, ns2 listens on 10.53.0.2, and ns4 on
  10.53.0.4. All test servers use an unprivileged port, so they don't need to
  run as root. These servers log at the highest debug level and the log is
  captured in the file "named.run".

- `ans<N>`: Like ns<N>, but these are mock name servers implemented in python
  (`ans.py`), usually with the `isctest.asyncserver` module.  They are
  generally programmed to misbehave in ways named would not, so as to exercise
  named's ability to interoperate with badly behaved name servers.  A few
  legacy mock servers are still implemented in perl (`ans.pl`); don't write
  new ones.

The following files appear in test directories that have not yet been fully
ported to python; do not add them to new tests:

- `tests.sh`: Legacy shell-based tests, run via a `tests_sh_*.py` glue module.

- `setup.sh`: Legacy shell test setup.  New tests use templates and a
  `bootstrap()` function instead.

### Module Scope

A module is a python file which contains test functions. Every system
test directory may contain multiple modules (i.e. tests_*.py files).

The server setup/teardown is performed for each module. Bundling test cases
together inside a single module may save some resources. However, test cases
inside a single module can't be executed in parallel.

It is possible to execute different modules defined within a single system test
directory in parallel. This is possible thanks to executing the tests inside a
temporary directory and proper port assignment to ensure there won't be any
conflicts.

### Port Usage

In order for the tests to run in parallel, each test requires a unique set of
ports. This is ensured by the pytest runner, which assigns a unique set of
ports to each test module.

Inside the python tests, it is possible to use fixtures like `named_port` to
get the assigned port numbers. They're also set as environment variables.
These include:

- `PORT`: used as the basic dns port
- `TLSPORT`: used as the port for DNS-over-TLS
- `HTTPPORT`, `HTTPSPORT`: used as the ports for DNS-over-HTTP(S)
- `CONTROLPORT`: used as the RNDC control port
- `EXTRAPORT1` through `EXTRAPORT8`: additional ports that can be used as needed

### Templates

Configuration files which need values that are only known at test run time —
ports, default crypto algorithms, conditional sections — are written as jinja2
templates with a `.j2` extension.  During test setup, the pytest runner
renders every `*.j2` file in the test directory and strips the extension:
`ns1/named.conf.j2` becomes `ns1/named.conf`.

Inside a template, all the runner's environment variables are available with
`@...@` delimiters, e.g.:

```jinja
options {
    port @PORT@;
    listen-on { 10.53.0.1; };
};
key rndc_key {
    secret "1234abcd8765";
    algorithm @DEFAULT_HMAC@;
};
controls {
    inet 10.53.0.1 port @CONTROLPORT@ allow { any; } keys { rndc_key; };
};
```

Standard jinja2 block syntax (`{% if %}`, `{% for %}`, …) can be used for
conditional or repeated sections.

Custom template variables come from an optional module-level `bootstrap()`
function.  When a test module defines one, the runner calls it before
rendering the templates and passes the returned dict to the template engine:

```python
def bootstrap():
    return {"valid": True}
```

Templates using custom variables must always provide defaults, so that the
file also renders when no value is supplied (e.g. when another module in the
same directory has no `bootstrap()`):

```jinja
{% set valid = valid | default(False) %}
```

`bootstrap()` is also the place where a module generates test data that has
to exist before the servers start — typically zone files and DNSSEC keys (see
[the cookbook](COOKBOOK.md) for a complete example).

Templates can also be re-rendered while the test is running, using the
`templates` fixture, e.g. to change a server's config before reloading it:

```python
def test_reload(ns1, templates):
    templates.render("ns1/named.conf", {"valid": True})
    ns1.reconfigure()
```

If you don't need the file to be auto-templated during test setup, use the
extension `.j2.manual` instead; such templates are only rendered when the test
calls `templates.render()` explicitly, and no defaults are needed.

### Fixtures and Helpers

Fixtures defined in `conftest.py` provide the test context:

- `servers` is a dictionary of all started `isctest.instance.NamedInstance`
  servers, keyed by directory name; the shortcut fixtures `ns1` through `ns11`
  return the corresponding instance directly. A `NamedInstance` is the
  interface for driving a server: `ns1.rndc("...")`, `ns1.reconfigure()`,
  `ns1.nsupdate(...)`, `ns1.watch_log_from_here()`, `ns1.ip`, ...
- `templates` renders jinja2 templates at runtime (see above).
- `named_port`, `named_tlsport`, `control_port`, ... return the assigned port
  numbers.
- `system_test_dir` is the temporary directory the module runs in.

The `isctest` package provides the helper library; the modules most tests
need are `isctest.query` (send DNS queries), `isctest.check` (assert on
responses), `isctest.zone` (zone and key setup), `isctest.kasp` (DNSSEC
key state checks), `isctest.asyncserver` (mock servers) and `isctest.log`
(logging and log watchers).

Pytest marks control test collection and setup:

- `@pytest.mark.extra_artifacts([...])` declares the files (globs) the test is
  expected to leave behind in addition to the common ones; undeclared
  leftovers fail the run.
- `@pytest.mark.requires_zones_loaded("ns1", ...)` delays the test until the
  listed servers have loaded all zones.
- `isctest.mark` has skip-unless conditions for environment prerequisites,
  e.g. `isctest.mark.with_dnstap`, `isctest.mark.with_lmdb`,
  `isctest.mark.live_internet_test`.

### Parametrization

Use `pytest.mark.parametrize` to run one test function over several inputs
instead of copy-pasting test cases or looping inside one test function — each
case is reported (and can be re-run) individually:

```python
@pytest.mark.parametrize(
    "qname,rdtype",
    [
        ("exists.example.", "A"),
        ("exists.example.", "TXT"),
        ("other.example.", "A"),
    ],
)
def test_answers(qname, rdtype, ns1):
    msg = isctest.query.create(qname, rdtype)
    response = isctest.query.udp(msg, ns1.ip)
    isctest.check.noerror(response)
```

This creates the test cases `test_answers[exists.example.-A]` etc., which can
be passed to pytest as node IDs.

### Logging

Each module has a separate log which will be saved as pytest.log.txt in the
temporary directory in which the test is executed. This log includes messages
for this module setup/teardown as well as any logging from the tests. Logging
level DEBUG and above will be present in this log.

Use `isctest.log` for test output (`isctest.log.info("...")` etc.); in
general, any log messages using INFO or above will also be printed out during
pytest execution. In CI, the pytest output is also saved to pytest.out.txt in
the bin/tests/system directory.

### Adding a Test to the System Test Suite

Once a test has been created it will be automatically picked up by the pytest
runner if it upholds the convention expected by pytest (especially when it
comes to naming files and test functions).  A new system test directory also
needs to be added to `TESTS` in `Makefile.am`, in order to be included in
`make check` runs.


### Writing a regression reproducer

The goal: turn "issue #NNNN" into a failing test with minimal ceremony.

1. **Decide the server topology.** Most reproducers need one of:
   - a single authoritative `named` (answer content bugs) — the skeleton
     recipe above;
   - a resolver plus a mock server that misbehaves (resolver bugs) — see
     the mock server recipe below;
   - signed zones and a validating resolver (DNSSEC bugs) — see the zone
     setup recipe below.

2. **Find the closest existing test and copy its shape.**  Good exemplars:
   `dispatch` (resolver + python mock server), `dnssec_py` (signed zones,
   validator, multiple modules sharing one server set), `nsec3` (multi-module
   family), `kasp`/`rollover_*` (key management state machines).

3. **Decide where the test lives.**  If an existing directory already has the
   server set you need, add a new `tests_*.py` module there; otherwise create
   a new directory.  Each module gets its own temporary directory, port
   range, and parallel slot, so you are not entangled with the other modules.
   Test functions *within* a module, however, run in file order against the
   same live servers: a new test inherits whatever state the tests above it
   left behind (cache contents, dynamic updates) and can disturb the tests
   below it.

4. **Write the test to fail first.**  Run it against an unfixed build and
   make sure it fails for the reason the issue describes — `ns*/named.run`
   in the kept temporary directory is the place to verify that.  Then apply
   the fix and watch it pass.


### Mock a misbehaving server

When a test needs a server that answers in ways named never would (bogus
glue, truncation, dropped queries, malformed records), add an `ansN`
subdirectory containing an `ans.py` script based on `isctest.asyncserver`.
The runner starts it automatically on 10.53.0.N, logging to `ans.run`.

Implementing a custom `ansN` server happens in two phases:

  - define all static DNS data that the server needs to serve (if any) in `*.db`
    files, like you would for a regular `named` instance,

  - implement any non-standard behavior (modifying zone-based responses or
    generating responses from scratch) by defining a response handler class,
    scoping it to the QNAMEs/QTYPEs/domains it owns, and installing it into an
    `AsyncDnsServer`.

Most importantly, avoid the temptation to define all DNS responses that a given
`ansN` server needs to serve using just dnspython APIs; zone files are much
easier to follow for static DNS data.  Splitting up static DNS data and custom
behavior also makes it easier to follow the idea behind each test.

The most commonly subclassed handler classes are (ordered by descending
specificity):

  - `QnameQtypeHandler`
  - `QnameHandler`
  - `DomainHandler`

These handler classes require certain properties (e.g. `qnames`, `qtypes`,
`domains`) to be defined by their subclasses.  These properties define the set
of queries that a given handler should be used for.  Please see
`isctest/asyncserver.py` for up-to-date information on available handler classes
and existing `ans.py` files for how they can be used in practice.  Consult the
log files (`ans.run`) in case a query is not matched by its intended handler.

**NOTE:** For readability (of both code and logs), defining separate handler
classes for distinct queries is strongly preferred over using a single handler
containing an `if`/`elif`/`else` chain.

**NOTE:** If you find yourself implementing an `__init__()` method in your
handler subclass, it often indicates that you're approaching the problem at hand
from the wrong side; contact QA for guidance in such a case.

When a query is matched to a handler, the latter is expected to yield a response
action through its `get_responses()` method, an async generator that inspects
the query context and decides how the server should react:

```python
from collections.abc import AsyncGenerator

import dns.flags

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)


class TruncateHandler(DomainHandler):
    """Answer everything under broken.example. with TC=1."""

    domains = ["broken.example."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        qctx.response.flags |= dns.flags.TC
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(TruncateHandler())
    server.run()


if __name__ == "__main__":
    main()
```

The available response actions are `DnsResponseSend` (optionally with a
`delay`), `ResponseDrop` (don't answer at all), `BytesResponseSend` (raw
bytes, for malformed packets) and `CloseConnection` (TCP).  Queries that no
handler matches are answered from zone data — `AsyncDnsServer` loads every
`*.db` zone file found in the `ansN` directory at startup — or with the
server's default rcode (REFUSED unless configured otherwise).

**NOTE:** For returning static responses, subclassing `StaticResponseHandler` is
strongly recommended instead of implementing the `get_responses()` generator
manually; see `resolver/ans3/ans.py` for practical examples.

**NOTE:** Calling `yield` does **NOT** make `get_responses()` return!  This is
by design: `get_responses()` can yield multiple DNS messages in response to a
single query, so that it can also handle AXFR/IXFR queries, among others.  Be
careful not to unintentionally cause multiple DNS messages to be returned for a
single query.  If your handler's `get_responses()` method contains multiple
`yield` statements, it might be a sign that it needs to be refactored into
multiple separate handlers.

If multiple `ansN` instances used in a given system test need to share common
logic, extract that logic into a `<test-name>_ans.py` module in the system test
directory.  See the `qmin` system test for a practical example.

If multiple system tests would benefit from sharing some common logic, consider
submitting a merge request adding that logic to `isctest/asyncserver.py` itself.

To the extent possible, try to keep each `ans.py` file limited in length and
scope.  Look at existing `ans.py` files to see what is meant by that.  If the
response generation logic required for reproducing a given bug is particularly
complex, consider dedicating the entire `ans.py` file just to that logic instead
of appending it to an existing one; `ansN` instances are cheap to spawn and run
compared to regular `named` instances.  If the number of `ansN` instances used
in a given system test is becoming unwieldy, it usually indicates the need to
start adding/moving code to a new system test directory.

In some rare cases, it may be useful to reuse a common set of `nsN` server
instances to reproduce a whole class of related issues, triggering which relies
on some non-standard behavior and therefore needs a custom `ansN` server to be
implemented.  If the logic necessary for reproducing each of these issues is
complex and the amount of those issues makes it impractical to add a separate
`ansN` server for each issue (as recommended in the previous paragraph), it is
acceptable to split up the test logic for each issue into separate `ans_*.py`
modules inside a single `ansN` directory and reduce `ans.py` itself to a loader
that imports and installs handlers defined in those separate modules:

```python
from mytest.ans1 import ans_some_bug, ans_some_other_bug
from isctest.asyncserver import AsyncDnsServer


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(ans_some_bug.SomeBugHandler())
    server.install_response_handler(ans_some_other_bug.SomeOtherBugHandler())
    server.run()


if __name__ == "__main__":
    main()
```

However, in such a case it is particularly important to ensure consistency
between the names of all the Python files related to a given issue - otherwise,
chaos ensues.  Furthermore, avoid using cryptic file names (e.g. numeric bug
identifiers).  The recommended naming scheme is:

```
mytest/
├── ans1
│   ├── ans.py
│   ├── ans_some_bug.py
│   └── ans_some_other_bug.py
├── ns2
│   └── ...
├── tests_some_bug.py
└── tests_some_other_bug.py
```

To point a resolver at the mock, delegate to it from the test's root zone
(served by ns1) or list it as a forwarder; `dispatch` shows the
delegation pattern end to end.

The existing mock servers are the best reference.  To find them, grep for
what you're about to use:
`git grep -l isctest.asyncserver -- '*/ans*/ans.py'` lists every python
mock, and a grep for the base class
(`DomainHandler`, `QnameHandler`, `ConnectionHandler`) or the response
action (`ResponseDrop`, `BytesResponseSend`, ...) you need usually turns
up a test already doing something similar.  The full toolbox lives in
`isctest/asyncserver.py` (query matching, TCP connection handling, TSIG
keyrings).


## Nameservers

As noted earlier, a system test will involve a number of nameservers.  These
will be either instances of named, or mock servers, typically written in
Python.

For the former, the version of "named" being run is the one from the build
tree set up by `./configure` (i.e. if the tests are run immediately after
`make`, the version of "named" used is the one just built).  The
configuration files, zone files etc. for these servers are located in
subdirectories of the test directory named "nsN", where N is a small integer.
The latter are special nameservers, mostly used for generating deliberately bad
responses, located in subdirectories named "ansN" (again, N is an integer).
In addition to configuration files, these directories should hold the
appropriate script files as well.

Note that the "N" for a particular test forms a single number space, e.g. if
there is an "ns2" directory, there cannot be an "ans2" directory as well.
Ideally, the directory numbers should start at 1 and work upwards.

When tests are executed, pytest takes care of the test setup and teardown. It
looks for any `nsN` and `ansN` directories in the system test directory and
starts those servers.

### `named` Command-Line Options

By default, `named` server is started with the following options:

```
-c named.conf   Specifies the configuration file to use (so by implication,
                each "nsN" nameserver's configuration file must be called
                named.conf).

-d 99           Sets the maximum debugging level.

-D <name>       The "-D" option sets a string used to identify the
                nameserver in a process listing.  In this case, the string
                is the name of the subdirectory.

-g              Runs the server in the foreground and logs everything to
                stderr.

-m record
                Turns on these memory usage debugging flags.
```

All output is sent to a file called `named.run` in the nameserver directory.

The options used to start named can be altered. There are a couple ways of
doing this. The runner checks the methods in a specific order: if a check
succeeds, the options are set and any other specification is ignored.  In
order, these are:

1. Including a file called "named.args" in the "nsN" directory.  If present,
the contents of the first non-commented, non-blank line of the file are used as
the named command-line arguments.  The rest of the file is ignored.

2. Tweaking the default command line arguments with "-T" options.  This flag is
used to alter the behavior of BIND for testing and is not documented in the
ARM.  The presence of a file called `named.<flag>` in the "nsN" directory adds
`-T <flag>` to the default command line (the content of the file is irrelevant
- it is only the presence that counts).  The recognized flags are:

```
dropedns         Recognise EDNS options in messages, but drop messages
                 containing them.

ednsformerr, ednsnotimp, ednsrefused
                 Answer EDNS queries with the given rcode, pretending to
                 be an old server that doesn't understand EDNS.

cookiealwaysvalid
                 Accept any DNS cookie presented by a client.

noaa             Never set the AA bit in an answer.

noedns           Disable recognition of EDNS options in messages.

nonearest        Omit the closest-encloser NSEC3 proof from negative
                 responses (except for DS queries).

nosoa            Disable the addition of SOA records to negative
                 responses (or to the additional section if the response
                 is triggered by RPZ rewriting).

maxudp512, maxudp1460
                 Set the maximum UDP size handled by named to 512/1460.

tat=1, tat=3     Send trust-anchor-telemetry queries every N seconds.

notcp            Disable TCP in "named".  Unlike the other flags, this
                 one is also applied when "named.args" is used.
```

### Running Nameservers Interactively

In order to debug the nameservers, you can let pytest perform the nameserver
setup and interact with the servers before the test starts, or even at specific
points during the test, using the `--trace` option to drop you into pdb debugger
which pauses the execution of the tests, while keeping the server state intact:

```sh
pytest -k dns64 --trace
```


## Developer Notes

### Test discovery and collection

There are two distinct types of system tests. The first is a legacy shell
script tests.sh containing individual test cases executed sequentially and the
success/failure is determined by return code. The second type is a regular
pytest file which contains test functions.

Dealing with the regular pytest files doesn't require any special consideration
as long as the naming conventions are met. Discovering the tests.sh tests is
more complicated.

The chosen solution is to add a bit of glue for each system test. For every
tests.sh, there is an accompanying tests_sh_*.py file that contains a test
function which utilizes a custom run_tests_sh fixture to call the tests.sh
script. Other solutions were tried and eventually rejected. While this
introduces a bit of extra glue, it is the most portable, compatible and least
complex solution.

### Compatibility with older pytest version

The minimum supported versions of python and the required python packages are
declared in [requirements.txt](requirements.txt) and in the
`pytest_configure()` check in `conftest.py`.  When implementing new runner
features, check feature support in the pytest and pytest-xdist versions
available in the oldest distributions covered by CI first; we may need to add
compat code to handle breaking upstream changes in either direction.

### Format of Shell Test Output

Legacy shell-based tests have the following format of output:

```
<letter>:<test-name>:<message> [(<number>)]
```

e.g.

```
I:catz:checking that dom1.example is not served by primary (1)
```

The meanings of the fields are as follows:

<letter>
This indicates the type of message.  This is one of:

```
S   Start of the test
A   Start of test (retained for backwards compatibility)
T   Start of test (retained for backwards compatibility)
E   End of the test
I   Information.  A test will typically output many of these messages
    during its run, indicating test progress.  Note that such a message may
    be of the form "I:testname:failed", indicating that a sub-test has
    failed.
R   Result.  Each test will result in one such message, which is of the
    form:

            R:<test-tmpdir>:<result>

    where <result> is one of:

        PASS        The test passed
        FAIL        The test failed
        SKIPPED     The test was not run, usually because some
                    prerequisites required to run the test are missing.
```

<test-tmpdir>
This is the name of the temporary test directory from which the message
emanated, which is also the name of the subdirectory holding the test files.

<message>
This is text output by the test during its execution.

(<number>)
If present, this will correlate with a file created by the test.  The tests
execute commands and route the output of each command to a file.  The name of
this file depends on the command and the test, but will usually be of the form:

```
<command>.out.<suffix><number>
```

e.g. nsupdate.out.test28, dig.out.q3.  This aids diagnosis of problems by
allowing the output that caused the problem message to be identified.
