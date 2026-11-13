#!/usr/bin/env python3

# Copyright (C) 2026 Andrew Tridgell
#
# Modelled on rsync's runtests.py test harness.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version
# 2 as published by the Free Software Foundation.

"""pppd integration test runner.

Invokes test scripts from testsuite/ and reports results.

Usage:
    ./runtests.py [options] [TEST ...]

Each TEST is a test name (e.g. 'link-up') or glob pattern (e.g. 'auth*').
If no tests are specified, all tests are run.

The link tests start pairs of pppd instances in private network namespaces,
which requires root or passwordless sudo; without that they report SKIP.

Two pppd versions can be tested against each other with --pppd-bin and
--pppd-bin2 (e.g. the build tree's pppd against a distro /usr/sbin/pppd).
"""

import argparse
import concurrent.futures
import fnmatch
import glob
import os
import signal
import subprocess
import sys
import threading

# Share the test exit-code enum with the test helpers. exitcodes.py lives in
# testsuite/ (next to this script); it has no import-time side effects.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'testsuite'))
from exitcodes import Exit


def parse_args():
    p = argparse.ArgumentParser(description='Run pppd integration test suite')
    p.add_argument('tests', nargs='*', metavar='TEST',
                   help='Test names or patterns to run (default: all)')
    p.add_argument('--exclude', default=None, metavar='LIST',
                   help='Comma-separated test names/globs to skip entirely: '
                        'they are not run and not reported as skipped. '
                        'Falls back to the PPPD_TEST_EXCLUDE environment '
                        'variable.')
    p.add_argument('-j', '--parallel', type=int, default=1, metavar='N',
                   help='Run up to N tests in parallel (default: 1)')
    p.add_argument('--preserve-scratch', action='store_true',
                   help='Keep scratch directories after tests complete')
    p.add_argument('--always-log', action='store_true',
                   help='Show test logs even for passing tests')
    p.add_argument('--stop-on-fail', action='store_true',
                   help='Stop after first test failure')
    p.add_argument('--fail-on-skip', action='store_true',
                   help='Treat skipped tests as failures. For CI: without '
                        'this, losing a prerequisite (sudo, /dev/ppp, ...) '
                        'would skip every link test and still exit 0.')
    p.add_argument('--timeout', type=int, default=120, metavar='SECS',
                   help='Per-test timeout in seconds (default: 120)')
    p.add_argument('--pppd-bin', default=None, metavar='PATH',
                   help='Path to pppd binary (default: ./pppd/pppd)')
    p.add_argument('--pppd-bin2', default=None, metavar='PATH',
                   help='Path to a second ("peer") pppd binary used for the '
                        'far side of each link. Lets the suite mix two pppd '
                        'versions over the wire. Default: same as --pppd-bin '
                        '(no version mixing).')
    p.add_argument('--pppd-confdir', default=None, metavar='DIR',
                   help='Configuration directory the pppd binary was compiled '
                        'with (the dir holding its options/pap-secrets files). '
                        'Default: auto-detected from the binary.')
    p.add_argument('--pppd-confdir2', default=None, metavar='DIR',
                   help='Configuration directory for the --pppd-bin2 binary. '
                        'Default: auto-detected from the binary.')
    p.add_argument('--tooldir', default=None, metavar='DIR',
                   help='Build directory (default: cwd)')
    p.add_argument('--srcdir', default=None, metavar='DIR',
                   help='Source directory (default: script directory)')
    p.add_argument('--expect-result', default=None, metavar='FILE',
                   help='Path to an expected-outcome manifest (one '
                        '"<testname> <pass|skip|fail|xfail>" per line). When '
                        'set, ONLY the tests listed in FILE are run, and each '
                        "test's actual outcome is compared against its "
                        'expected one; any mismatch (including an unexpected '
                        'pass) fails the run. Used for version-mixing CI.')
    return p.parse_args()


def prep_scratch(scratchdir):
    """Prepare a fresh scratch directory for a test."""
    if os.path.isdir(scratchdir):
        subprocess.run(['chmod', '-R', 'u+rwX', scratchdir], capture_output=True)
        subprocess.run(['rm', '-rf', scratchdir], capture_output=True)
    os.makedirs(scratchdir, exist_ok=True)


# Tests are identified by a positive "_test.py" suffix so that helper
# modules (e.g. pppfns.py) sit in testsuite/ without being mistaken for
# tests.
_PY_TEST_SUFFIX = '_test.py'


def _is_test_path(path):
    return os.path.basename(path).endswith(_PY_TEST_SUFFIX)


def _testbase(path):
    """Strip the test extension to get the canonical test name."""
    base = os.path.basename(path)
    if base.endswith(_PY_TEST_SUFFIX):
        return base[:-len(_PY_TEST_SUFFIX)]
    return base


def collect_tests(suitedir, patterns):
    """Collect test scripts (_test.py) matching the given patterns."""
    if not patterns:
        candidates = glob.glob(os.path.join(suitedir, '*' + _PY_TEST_SUFFIX))
        tests = sorted(p for p in candidates if _is_test_path(p))
    else:
        seen = set()
        tests = []
        for pat in patterns:
            # Accept either bare name ("link-up"), explicit extension, or glob.
            if pat.endswith('.py'):
                pats = [pat]
            else:
                pats = [pat + _PY_TEST_SUFFIX]
            for p in pats:
                for m in sorted(glob.glob(os.path.join(suitedir, p))):
                    if _is_test_path(m) and m not in seen:
                        seen.add(m)
                        tests.append(m)
    return tests


_VALID_OUTCOMES = ('pass', 'skip', 'fail', 'xfail')


def parse_expect_result(path):
    """Parse an expected-outcome manifest into {testbase: outcome}.

    One "<testname> <outcome>" entry per line; '#' comments and blank lines
    are ignored. outcome is one of pass|skip|fail|xfail. The set of listed
    tests doubles as the run set (see main()). Exits 2 on a malformed file.
    """
    expect = {}
    with open(path) as f:
        for lineno, raw in enumerate(f, 1):
            line = raw.split('#', 1)[0].strip()
            if not line:
                continue
            fields = line.split()
            if len(fields) != 2 or fields[1] not in _VALID_OUTCOMES:
                sys.stderr.write(
                    f"{path}:{lineno}: expected '<testname> "
                    f"<{'|'.join(_VALID_OUTCOMES)}>', got: {raw.rstrip()}\n"
                )
                sys.exit(Exit.ERROR)
            expect[fields[0]] = fields[1]
    return expect


def outcome_of(result):
    """Map a per-test exit code to an outcome string."""
    if result == Exit.PASS:
        return 'pass'
    if result == Exit.SKIP:
        return 'skip'
    if result == Exit.XFAIL:
        return 'xfail'
    return 'fail'


class TestResult:
    """Result of a single test execution."""
    __slots__ = ('testbase', 'result', 'output', 'skipped_reason')

    def __init__(self, testbase, result, output='', skipped_reason=''):
        self.testbase = testbase
        self.result = result
        self.output = output
        self.skipped_reason = skipped_reason


def run_one_test(testscript, testbase, scratchdir, base_env, timeout, always_log):
    """Run a single test. Returns a TestResult.

    This function is safe to call from multiple threads — it uses only
    per-test state (unique scratchdir, copy of env).
    """
    prep_scratch(scratchdir)

    env = base_env.copy()
    env['scratchdir'] = scratchdir

    cmd = [sys.executable, testscript]

    logfile = os.path.join(scratchdir, 'test.log')
    with open(logfile, 'w') as log:
        # start_new_session: run the test driver as its own session/group
        # leader so a timeout can killpg the whole tree. Note that pppd
        # instances run as root and survive the killpg when the runner is
        # unprivileged; the helpers bound their lifetime with maxconnect.
        proc = subprocess.Popen(
            cmd,
            stdout=log, stderr=subprocess.STDOUT,
            env=env, cwd=env.get('TOOLDIR', '.'),
            start_new_session=True,
        )
        try:
            result = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            # Reap the whole session group, but only if the driver really is
            # its own group leader and that group isn't ours -- killpg of our
            # own group would take down the runner.
            try:
                pgid = os.getpgid(proc.pid)
            except OSError:
                pgid = -1
            if pgid == proc.pid and pgid != os.getpgrp():
                try:
                    os.killpg(pgid, signal.SIGKILL)
                except OSError:
                    proc.kill()
            else:
                proc.kill()
            proc.wait()
            result = 1
            log.write(f"\nTIMEOUT: test took over {timeout} seconds\n")

    # Build output text
    output_parts = []

    show_log = always_log or (result not in (Exit.PASS, Exit.SKIP, Exit.XFAIL))
    if show_log:
        output_parts.append(f'----- {testbase} log follows')
        try:
            with open(logfile) as f:
                output_parts.append(f.read().rstrip())
        except FileNotFoundError:
            pass
        output_parts.append(f'----- {testbase} log ends')
        # Dump the per-peer pppd logs and spawn errors too: with notty the
        # pppd output only goes to its logfile, so this is the main evidence
        # of what went wrong on the wire.
        for extra in sorted(glob.glob(os.path.join(scratchdir, '*', 'pppd.log')) +
                            glob.glob(os.path.join(scratchdir, '*', 'spawn.err'))):
            relname = os.path.relpath(extra, scratchdir)
            try:
                with open(extra) as f:
                    content = f.read().rstrip()
            except OSError:
                continue
            if not content:
                continue
            output_parts.append(f'----- {testbase} {relname} follows')
            output_parts.append(content)
            output_parts.append(f'----- {testbase} {relname} ends')

    skipped_reason = ''
    if result == Exit.PASS:
        output_parts.append(f'PASS    {testbase}')
    elif result == Exit.SKIP:
        whyfile = os.path.join(scratchdir, 'whyskipped')
        try:
            with open(whyfile) as f:
                skipped_reason = f.read().strip()
        except FileNotFoundError:
            pass
        output_parts.append(f'SKIP    {testbase} ({skipped_reason})')
    elif result == Exit.XFAIL:
        output_parts.append(f'XFAIL   {testbase}')
    else:
        output_parts.append(f'FAIL    {testbase}')

    return TestResult(testbase, result, '\n'.join(output_parts), skipped_reason)


# Lock for serializing output in parallel mode
_print_lock = threading.Lock()


def _resolve_bin(value, default):
    """Absolutize a binary path so tests (which run with cwd=TOOLDIR) and
    relative --pppd-bin=../foo/pppd forms both resolve as the operator
    intended."""
    path = value or default
    if path and not os.path.isabs(path):
        path = os.path.abspath(path)
    return path


def main():
    args = parse_args()

    if args.exclude is None:
        args.exclude = os.environ.get('PPPD_TEST_EXCLUDE', '')

    # Determine directories
    tooldir = args.tooldir or os.environ.get('TOOLDIR') or os.getcwd()
    script_path = os.path.dirname(os.path.abspath(__file__))
    srcdir = args.srcdir or script_path

    pppd_bin = _resolve_bin(args.pppd_bin or os.environ.get('PPPD'),
                            os.path.join(tooldir, 'pppd', 'pppd'))
    # Optional second ("peer") binary for the far side of each link, so a
    # run can mix two pppd versions. Defaults to pppd_bin -> no mixing.
    pppd_bin2 = _resolve_bin(args.pppd_bin2 or os.environ.get('PPPD_PEER'),
                             pppd_bin)

    suitedir = os.path.join(srcdir, 'testsuite')
    scratchbase = os.path.join(os.environ.get('scratchbase', tooldir), 'testtmp')
    os.makedirs(scratchbase, exist_ok=True)

    if not os.path.isfile(pppd_bin):
        sys.stderr.write(f"pppd binary {pppd_bin} is not a file "
                         f"(build it first, or use --pppd-bin)\n")
        sys.exit(Exit.ERROR)
    if not os.path.isfile(pppd_bin2):
        sys.stderr.write(f"pppd binary {pppd_bin2} is not a file\n")
        sys.exit(Exit.ERROR)
    if not os.path.isdir(suitedir):
        sys.stderr.write(f"testsuite dir {suitedir} is not a directory\n")
        sys.exit(Exit.ERROR)

    # Print header
    print('=' * 60)
    print(f'{sys.argv[0]} running in {tooldir}')
    print(f'    pppd_bin={pppd_bin}')
    if pppd_bin2 != pppd_bin:
        print(f'    pppd_peer={pppd_bin2}')
    print(f'    srcdir={srcdir}')
    print(f'    os={subprocess.check_output(["uname", "-a"], text=True).strip()}')
    print(f'    preserve_scratch={"yes" if args.preserve_scratch else "no"}')
    if args.parallel > 1:
        print(f'    parallel={args.parallel}')
    print(f'    scratchbase={scratchbase}')

    # Make the testsuite/ directory importable so tests can `import pppfns`.
    pythonpath = suitedir
    if os.environ.get('PYTHONPATH'):
        pythonpath = suitedir + os.pathsep + os.environ['PYTHONPATH']

    base_env = os.environ.copy()
    base_env.update({
        'TOOLDIR': tooldir,
        'srcdir': srcdir,
        'suitedir': suitedir,
        'PPPD': pppd_bin,
        'PPPD_PEER': pppd_bin2,
        'PPPD_CONFDIR': args.pppd_confdir or os.environ.get('PPPD_CONFDIR', ''),
        'PPPD_CONFDIR2': args.pppd_confdir2 or os.environ.get('PPPD_CONFDIR2', ''),
        'scratchbase': scratchbase,
        'TESTRUN_TIMEOUT': str(args.timeout),
        'PYTHONPATH': pythonpath,
        # Keep __pycache__ out of the (possibly read-only) source tree.
        'PYTHONDONTWRITEBYTECODE': '1',
    })

    # Collect tests
    tests = collect_tests(suitedir, args.tests)

    # Drop excluded tests entirely (matched by basename against name/glob).
    excl = [e.strip() for e in args.exclude.split(',') if e.strip()]
    if excl:
        before = len(tests)
        tests = [t for t in tests
                 if not any(fnmatch.fnmatch(_testbase(t), pat) for pat in excl)]
        if before != len(tests):
            print(f"Excluding {before - len(tests)} test(s) matching: "
                  f"{', '.join(excl)}")

    # An expected-result manifest defines BOTH the run set (its keys) and the
    # expected per-test outcome (its values). Used for version-mixing runs.
    expect = parse_expect_result(args.expect_result) if args.expect_result else None
    if expect is not None:
        have = {_testbase(t) for t in tests}
        unknown = sorted(k for k in expect if k not in have)
        if unknown:
            sys.stderr.write(
                "runtests.py: --expect-result lists test(s) with no matching "
                f"test file (ignored): {', '.join(unknown)}\n"
            )
        tests = [t for t in tests if _testbase(t) in expect]

    def _cls(outcome):
        """Equivalence class for outcome comparison: fail and xfail both just
        mean 'broke', so a manifest 'fail' matches an actual fail OR xfail."""
        return 'broken' if outcome in ('fail', 'xfail') else outcome

    def mismatch(testbase, actual):
        """True if actual outcome disagrees with the manifest expectation."""
        return expect is not None and _cls(expect[testbase]) != _cls(actual)

    # Record test order for consistent output
    test_order = {_testbase(t): i for i, t in enumerate(tests)}

    passed = 0
    failed = 0
    skipped = 0
    xfailed = 0
    outcomes = {}  # testbase -> actual outcome string

    def process_result(tr):
        """Process a TestResult and update counters. Returns True if the test
        should count as a failure for --stop-on-fail purposes."""
        nonlocal passed, failed, skipped, xfailed
        with _print_lock:
            if tr.output:
                print(tr.output, flush=True)
        scratchdir = os.path.join(scratchbase, tr.testbase)
        oc = outcome_of(tr.result)
        outcomes[tr.testbase] = oc
        if tr.result == Exit.PASS:
            passed += 1
        elif tr.result == Exit.SKIP:
            skipped += 1
        elif tr.result == Exit.XFAIL:
            # XFAIL: an expected failure (a known, documented residual the
            # test asserts against). Reported distinctly but does NOT fail
            # the suite; when the underlying issue is fixed the test returns
            # 0 instead.
            xfailed += 1
        else:
            failed += 1
        if tr.result in (Exit.PASS, Exit.SKIP, Exit.XFAIL) and not args.preserve_scratch \
                and os.path.isdir(scratchdir):
            subprocess.run(['rm', '-rf', scratchdir], capture_output=True)
        # With a manifest, only a mismatch is a "failure" (an expected fail
        # is fine); without one, any non-pass/skip/xfail result is a failure.
        if expect is not None:
            return mismatch(tr.testbase, oc)
        return tr.result not in (Exit.PASS, Exit.SKIP, Exit.XFAIL)

    if args.parallel > 1:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel) as executor:
            futures = {}
            for testscript in tests:
                testbase = _testbase(testscript)
                scratchdir = os.path.join(scratchbase, testbase)
                f = executor.submit(
                    run_one_test, testscript, testbase, scratchdir,
                    base_env, args.timeout, args.always_log
                )
                futures[f] = testbase

            for f in concurrent.futures.as_completed(futures):
                tr = f.result()
                is_fail = process_result(tr)
                if is_fail and args.stop_on_fail:
                    for pending in futures:
                        pending.cancel()
                    break
    else:
        for testscript in tests:
            testbase = _testbase(testscript)
            scratchdir = os.path.join(scratchbase, testbase)
            tr = run_one_test(
                testscript, testbase, scratchdir,
                base_env, args.timeout, args.always_log
            )
            is_fail = process_result(tr)
            if is_fail and args.stop_on_fail:
                break

    # Summary
    print('-' * 60)
    print('----- overall results:')
    print(f'      {passed} passed')
    if failed > 0:
        print(f'      {failed} failed')
    if xfailed > 0:
        print(f'      {xfailed} xfailed (expected)')
    if skipped > 0:
        print(f'      {skipped} skipped')

    if expect is not None:
        # Version-mixing mode: the run is judged purely on whether each
        # test's actual outcome matched its manifest expectation. An expected
        # 'fail' is fine; an UNEXPECTED pass (xpass) or any other divergence
        # is not.
        mismatches = []
        for tb in sorted(expect, key=lambda x: test_order.get(x, 1 << 30)):
            actual = outcomes.get(tb, 'notrun')
            if actual == 'notrun' or mismatch(tb, actual):
                mismatches.append((tb, expect[tb], actual))
        if mismatches:
            print('----- expected-result mismatches:')
            for tb, want, got in mismatches:
                tag = ' (xpass)' if _cls(want) == 'broken' and got == 'pass' else ''
                print(f'      {tb}: expected {want}, got {got}{tag}')
        print('-' * 60)
        exit_code = len(mismatches)
        print(f'overall result is {exit_code}')
        sys.exit(exit_code)

    exit_code = failed
    if args.fail_on_skip and skipped > 0:
        print(f'      (--fail-on-skip: counting {skipped} skipped as failed)')
        exit_code += skipped
    print('-' * 60)
    print(f'overall result is {exit_code}')
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
