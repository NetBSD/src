#!/usr/bin/env python3
# pppd must actually execute the /etc/ppp hook scripts.
#
# Regression test for the run_program() child dereferencing the pppdb
# pointer that ppp_safe_fork() had already tdb_close()d (and freed): the
# child died of SIGSEGV before reaching execve, so ip-up/ip-down silently
# never ran while the parent daemon carried on looking healthy. Only
# reachable when pppd is built with TDB (--enable-multilink) and a hook
# script actually exists -- run_program() returns before forking if the
# script is missing, which is why no other test in this suite reaches it.
#
# Also checks PPP_SCRIPT_INSTANCE names the script actually being run
# (link_down() used to run auth-down labelled as "auth-up").
#
# Builds without TDB still run the test: the scripts must run and be
# labelled correctly there too, only the pppdb crash can't happen.

import os
import shlex
import time

from pppfns import (
    IS_LINUX, PPPD, SCRATCHDIR, PppPair, pppd_confdir,
    require_link_env, test_fail, test_skipped,
)

require_link_env()

CONFDIR = pppd_confdir(PPPD)

# The crash only happens in the TDB code path, so without it a pass says
# nothing about the pppdb fix; say so. PPP_PATH_PPPDB is only compiled in
# with PPP_WITH_TDB.
with open(PPPD, 'rb') as f:
    if b'/pppd2.tdb' not in f.read():
        print(f'note: {PPPD} built without TDB (--enable-multilink); '
              'pppdb regression not exercised')

HOOK = """#!/bin/sh
echo "hello from {name} instance=${{PPP_SCRIPT_INSTANCE:-unset}} args=$*" >> {marker}
"""


def make_scripts(names, marker):
    # The script child runs with umask 077, so a marker file created by the
    # (root) hook would be unreadable when the suite runs via sudo. Create
    # it as the invoking user first; >> keeps the ownership.
    marker.touch()
    return {name: HOOK.format(name=name, marker=shlex.quote(str(marker)))
            for name in names}


if not IS_LINUX:
    # Without a mount namespace the hooks go into the real confdir, and
    # launch.sh refuses to clobber existing ones (most hosts ship ip-up).
    for name in ('ip-pre-up', 'ip-up', 'ip-down', 'auth-up', 'auth-down'):
        if os.path.exists(f'{CONFDIR}/{name}'):
            test_skipped(f'{CONFDIR}/{name} already exists')

# Exit status run_program()'s child uses when the exec itself fails.
HOOK_EXEC_FAILED = 'status = 0x63'


def wait_for_hook(peer, marker, name, timeout=30):
    """Wait for the named hook script to append its line to the marker file."""
    prefix = f'hello from {name} '
    deadline = time.time() + timeout
    while True:
        for line in marker.read_text().splitlines():
            if line.startswith(prefix):
                return line
        if time.time() >= deadline:
            break
        time.sleep(0.1)

    # Didn't run. The parent logs a warning when a script child dies on a
    # signal, so surface that here -- it distinguishes "pppd never forked"
    # from "the forked child crashed before execve".
    path = f'{CONFDIR}/{name}'
    detail = ''
    for line in peer.log_text().splitlines():
        if 'terminated with signal' in line:
            detail = f'\npppd reported a dying script child: {line.strip()}'
            break
        if f'Script {path} finished' in line and HOOK_EXEC_FAILED in line:
            detail = f'\npppd could not exec the script: {line.strip()}'
    test_fail(f'{path} did not run within {timeout}s{detail}')


def check_hook(peer, marker, name):
    line = wait_for_hook(peer, marker, name)
    print(line)
    # PPP_SCRIPT_INSTANCE is new in 2.5.4; only assert it when present so
    # that --pppd-bin2 runs against an older binary still work.
    got = line.split('instance=', 1)[1].split(' ', 1)[0]
    if got == 'unset':
        print('  PPP_SCRIPT_INSTANCE not set (pre-2.5.4 pppd?)')
    elif got != name:
        test_fail(f'{name}: PPP_SCRIPT_INSTANCE is {got!r}, expected {name!r}')


def check_no_crashed_children(pair):
    for peer in (pair.a, pair.b):
        for line in peer.log_text().splitlines():
            if 'terminated with signal' in line:
                test_fail(f'pppd {peer.name}: script child died: {line.strip()}')


# Hooks go on side 'a', which always runs the binary under test.

print('ip-pre-up / ip-up / ip-down:')
marker = SCRATCHDIR / 'ip.out'
scripts = make_scripts(('ip-pre-up', 'ip-up', 'ip-down'), marker)
with PppPair(a_kwargs=dict(scripts=scripts), name='ip') as pair:
    pair.up()
    # ip-pre-up runs synchronously (run_program(..., wait=1)), the others
    # asynchronously -- different paths through the parent side of the fork.
    check_hook(pair.a, marker, 'ip-pre-up')
    check_hook(pair.a, marker, 'ip-up')
    # Dropping the peer makes 'a' tear the link down, which must run ip-down
    # before 'a' exits.
    pair.b.stop()
    check_hook(pair.a, marker, 'ip-down')
    pair.a.stop()
    check_no_crashed_children(pair)

# auth-up/auth-down only run when 'a' authenticated its peer, so make 'a'
# the PAP server (see auth-pap_test.py).
print('auth-up / auth-down:')
marker = SCRATCHDIR / 'auth.out'
scripts = make_scripts(('auth-up', 'auth-down'), marker)
with PppPair(a_options=['auth', 'require-pap', 'name', 'srv'],
             a_kwargs=dict(noauth=False, scripts=scripts,
                           pap_secrets='cli\tsrv\t"s3cret"\t*\n'),
             b_options=['user', 'cli', 'remotename', 'srv',
                        'password', 's3cret'],
             name='auth') as pair:
    pair.up()
    check_hook(pair.a, marker, 'auth-up')
    pair.b.stop()
    check_hook(pair.a, marker, 'auth-down')
    pair.a.stop()
    check_no_crashed_children(pair)
