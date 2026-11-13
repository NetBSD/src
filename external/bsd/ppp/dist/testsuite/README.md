# pppd integration testsuite

End-to-end tests that bring up pairs of pppd processes back-to-back and
verify link negotiation, IP routing, data integrity, authentication and
option negotiation. Run them from the build directory with:

```
./runtests.py                  # all tests
./runtests.py link-up ping     # specific tests
./runtests.py 'auth*'          # glob
./runtests.py -j 4             # in parallel
```

Each test is a `testsuite/NAME_test.py` script run by `runtests.py`; shared
helpers live in `testsuite/pppfns.py`. Per-test exit codes follow the
autotools convention: 0=pass, 1=fail, 2=error, 77=skipped, 78=xfail.

## How a link is built

Each test creates one or more `PppPair`s: two pppd processes joined by an
AF_UNIX socketpair on stdin/stdout (pppd's `notty` mode). Every pppd runs
in its own network namespace, so the only path between the two negotiated
addresses is the ppp link itself — a ping between them genuinely traverses
the link. A private mount namespace bind-mounts a per-peer configuration
directory over the binary's compiled-in one (auto-detected from the binary,
or set with `--pppd-confdir`), so the tests never read or modify the host's
/etc/ppp. The namespaces are anonymous and disappear with the processes.

The one tolerated host side effect: if the compiled-in configuration
directory does not exist (e.g. /usr/local/etc/ppp for a fresh build), it is
created empty so it can serve as a bind-mount point.

## Privileges

pppd must run as root. When the suite is not run as root, every privileged
command is prefixed with `sudo -n`; if neither root nor passwordless sudo
is available (or the kernel lacks ppp support), the link tests report SKIP.

## Non-Linux hosts (Solaris)

Network namespaces are Linux-only, so on other platforms the IP routing
tests (ping, data-transfer) skip: without namespaces both negotiated
addresses are local to one stack and traffic would short-circuit via
loopback. The protocol-level tests (link-up, auth, MRU negotiation) still
run, but in a mode that temporarily writes the test secrets into the
binary's real configuration directory; because that touches host state,
it must be enabled explicitly with PPPD_TEST_GLOBAL_CONF=1 on a disposable
host (the Solaris CI VM does this). Existing secrets files are never
overwritten. This mode is not parallel-safe; don't combine it with -j.

## Testing two pppd versions against each other

```
./runtests.py --pppd-bin2 /usr/sbin/pppd
```

runs the far side of every link with the second binary, exercising on-wire
interoperability between the build under test and another version. An
expected-outcome manifest for such runs can be supplied with
`--expect-result FILE` (one `<testname> <pass|skip|fail|xfail>` per line).

## Debugging a failure

The pppd debug logs (`<scratch>/<test>/<peer>/pppd.log`) are printed
automatically when a test fails. Use `--preserve-scratch` to keep the
scratch directories (default `./testtmp/<test>/`) and `--always-log` to see
the logs of passing tests too.
