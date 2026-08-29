<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->

# BIND9 System Test Cookbook

Task-oriented recipes for the system test framework.  Each recipe is
self-contained and shows working code that can be copied and adapted.
For concepts and reference documentation (build setup, fixtures, runner
internals), see [README.md](README.md).

Prerequisite for all recipes: a built tree and the test network interfaces
configured — see "Running the Tests" in the README.


## Iterate on a single test

The basic loop while developing a test:

```sh
cd bin/tests/system
pytest mytest                                     # whole directory
pytest mytest/tests_mytestmod.py::test_one_case   # one test function
```

When a test fails, its temporary directory is kept and a stable symlink to it
is created (e.g. `mytest-mytestmod` for `mytest/tests_mytestmod.py`).  Look
there for `pytest.log.txt` (the test's own log) and `ns*/named.run` (each
server's debug-level log).  Pass `--noclean` to keep the directory even on
success.

To poke at the servers interactively, pause the test at any point by
inserting a breakpoint, or start pdb right away with `--trace`; to get a pdb
prompt automatically when a test fails, use `--pdb`.  In all cases the
servers keep running while the test is paused, so their state can be
inspected:

```sh
pytest mytest --trace   # pdb at the start of each test
pytest mytest --pdb     # pdb when a test fails
```

Timing-sensitive bugs hide in system tests: before declaring a test done, run
it several times (and ideally once under parallel load, `pytest -n auto`).
A test that flakes is treated as a bug in the test.


## Add a new system test directory

This skeleton sets up one authoritative server and queries it.  Pick a name
that starts with a letter and uses underscores as word separators (hyphens
are not allowed), and create:

```
demo/
├── ns1/
│   ├── named.conf.j2
│   └── example.db
└── tests_demo.py
```

`demo/ns1/named.conf.j2` — the config template; the runner renders it to
`named.conf` at setup time, filling in the assigned ports.  Templates inside
an `nsN`/`ansN` subdirectory also get an `ns` variable describing that
server, so the config doesn't hardcode its own address (`@ns.ip@` renders to
10.53.0.1 in ns1, 10.53.0.2 in ns2, ...):

```jinja
options {
    query-source address @ns.ip@;
    notify-source @ns.ip@;
    transfer-source @ns.ip@;
    port @PORT@;
    pid-file "named.pid";
    listen-on { @ns.ip@; };
    listen-on-v6 { none; };
    recursion no;
    dnssec-validation no;
};

{% include "_common/controls.conf.j2" %}

zone "example" {
    type primary;
    file "example.db";
};
```

The `_common/controls.conf.j2` include sets up the rndc control channel, so
the test (and the runner's shutdown sequence) can use `rndc`.

`demo/ns1/example.db` — a plain zone file:

```
$TTL 300
example.        IN SOA  ns1.example. hostmaster.example. 1 600 600 1200 600
example.        NS      ns1.example.
ns1.example.    A       10.53.0.1
a.example.      A       10.0.0.1
```

`demo/tests_demo.py` — the test module:

```python
import isctest


def test_a_record(ns1):
    msg = isctest.query.create("a.example.", "A")
    response = isctest.query.udp(msg, ns1.ip)
    isctest.check.noerror(response)
    assert str(response.answer[0][0]) == "10.0.0.1"
```

That's it — run it with `pytest demo`.  The pytest runner discovers new
directories automatically; to also include the test in `make check` runs,
add it to `TESTS` in `Makefile.am`.

Notes:

- The `ns1` fixture is the started server (an
  `isctest.instance.NamedInstance`); `ns1.ip` is 10.53.0.1.  Servers in
  `nsN`/`ansN` subdirectories are started automatically.
- If your test writes files beyond the usual ones (e.g. `dig.out.*`,
  `dsset-*`), declare them as a module-level marker —
  `pytestmark = pytest.mark.extra_artifacts(["dig.out.*"])` — or the
  artifact check at teardown will fail.  Most real test modules carry one.


## Set up zones in bootstrap()

A module-level `bootstrap()` function runs before the config templates are
rendered and before the servers start.  This is where zone files and DNSSEC
keys can be generated; whatever dict it returns becomes template data.

The simplest case is a plain unsigned zone — use this instead of writing
the same boilerplate zone file by hand in every `nsN` directory:

```python
from isctest.template import NS1, zones
from isctest.zone import Zone


def bootstrap():
    zone = Zone("example", NS1)    # signed=False is the default
    zone.configure()               # render the zone file
    return {"zones": zones([zone])}
```

`Zone.configure()` renders the zone file from
`_common/zones/template.db.j2.manual`, a generic zone with SOA, NS, and a
few test records.  To provide your own content, drop a template named after
the zone file — `ns1/zones/<name>.db.j2.manual` — and `configure()` picks
it up automatically instead of the generic one.  (The `.j2.manual` suffix
keeps the file out of the runner's automatic template pass, which renders
every plain `*.j2` at setup time; here `bootstrap()` renders it instead.)

For DNSSEC-signed zones, pass `signed=True` and make sure to configure the
`trust_anchors`:

```python
from isctest.template import NS1, NS2, zones
from isctest.zone import Zone, configure_root


def bootstrap():
    zone = Zone("signed-example", NS2, signed=True)
    zone.configure()           # keys, zone file, dnssec-signzone

    # root zone on ns1, delegating to (and providing DS records for)
    # the zone above
    root = configure_root([zone])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, zone]),
    }
```

With `signed=True`, `configure()` also generates a KSK+ZSK with
`dnssec-keygen` and signs the rendered zone with `dnssec-signzone`.  To
sign a hand-written zone file checked into git (no rendering at all), put
it at `ns2/zones/<name>.db` and drive the steps yourself:

```python
def bootstrap():
    zone = Zone("signed-example", NS2, signed=True)
    zone.add_keys()
    zone.sign("-3 ABCD")       # extra dnssec-signzone args, e.g. NSEC3

    root = configure_root([zone])
    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, zone]),
    }
```

On the template side, the `_common` includes consume the returned data: the
authoritative server's `named.conf.j2` ends with

```jinja
{% include "_common/controls.conf.j2" %}
{% include "_common/zones.conf.j2" %}
```

(`zones.conf.j2` emits a `zone` block for every zone whose nameserver
matches the rendering server) and the validating resolver's with

```jinja
{% include "_common/controls.conf.j2" %}
{% include "_common/trusted.conf.j2" %}
{% include "_common/root.hint.conf" %}
```

(`trusted.conf.j2` emits `trust-anchors` from the `trust_anchors` data, and
`root.hint.conf` points the resolver at ns1 for the root zone).

The `dnssec_py` directory is the canonical example of this pattern,
including several modules with different zone setups sharing one directory.
Declare the generated files as artifacts:

```python
pytestmark = pytest.mark.extra_artifacts(
    [
        "ns*/dsset-*",
        "ns*/trusted.conf",
        "ns*/zones/*.db",
        "ns*/zones/*.db.signed",
    ]
)
```


## Drive named and watch its logs

The `NamedInstance` fixtures (`ns1` ... `ns11`) control the running servers.
The cardinal rule: never `time.sleep()` waiting for the server — watch the
log for the line that proves the event happened.

Wait for a log line caused by an action — enter the watcher *before*
triggering the action, so the line cannot slip past unobserved:

```python
def test_cache_flush(ns4):
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("flush")
        watcher.wait_for_line("flushing caches in all views succeeded")
```

`wait_for_line()` accepts a string, a compiled regex, or a list of either
(any match wins) and returns the `re.Match`.  `wait_for_all([...])` waits
until every pattern has appeared, `wait_for_sequence([...])` requires them
in order.  To match lines that may already have been logged (e.g. during
startup), use `watch_log_from_start()` instead.

Reconfigure a server mid-test by re-rendering its config template with new
data, then reloading.  `reconfigure()` and `reload()` send the rndc command
and wait for the corresponding completion line in the log:

```python
def test_with_new_config(ns1, templates):
    templates.render("ns1/named.conf", {"flag": True})
    ns1.reconfigure()
```

Dynamic updates go through `nsupdate`:

```python
import dns.update

def test_update(ns1):
    update = dns.update.UpdateMessage("example.")
    update.add("added.example.", 300, "A", "10.0.0.2")
    ns1.nsupdate(update)
```

Send queries and check the responses with `isctest.query` and
`isctest.check`:

```python
msg = isctest.query.create("a.example.", "A")
response = isctest.query.udp(msg, ns1.ip)     # or .tcp(), .tls()
isctest.check.noerror(response)               # rcode checks: nxdomain,
                                              #  servfail, refused, ...
isctest.check.adflag(response)                # flag checks: rdflag,
                                              #  noraflag, ...
isctest.check.same_answer(response, expected) # compare two responses
```

To wait until a zone transfer has happened, poll the SOA serial:

```python
isctest.query.wait_for_serial(ns2.ip, "example.", expected_serial)
```

For grepping a file the server produced (dumps, statistics), use
`isctest.text`:

```python
from re import compile as Re

ns4.rndc("dumpdb -all")
dump = isctest.text.TextFile("ns4/named_dump.db")
assert dump.grep(Re(r"^a\.example\..*10\.0\.0\.1"))
```
