"""Shared helpers for pppd's integration test scripts.

Each test builds one or more pairs of pppd processes joined back-to-back by
an AF_UNIX socketpair on stdin/stdout (pppd's "notty" mode). Every pppd runs
in its own network namespace, so the ONLY path between the two negotiated IP
addresses is the ppp link itself; a ping between them genuinely traverses
the link. A private mount namespace bind-mounts a per-peer etc.ppp directory
over the binary's compiled-in configuration directory, keeping the tests
hermetic (options, pap-secrets, chap-secrets) without touching the host's
/etc/ppp.

pppd must run as root, so when the suite is not run as root every privileged
command is prefixed with "sudo -n"; tests skip when neither applies.

Conventions matching the rsync-derived harness:
  * Exit codes (see the Exit enum): 0=pass, 1=fail, 2=error, 77=skip, 78=xfail.
  * The runner sets these environment variables before invoking each test:
      scratchdir       per-test scratch directory
      TOOLDIR          build directory
      PPPD             path to the pppd binary under test
      PPPD_PEER        pppd binary for the far side (version mixing)
      PPPD_CONFDIR(2)  compiled-in confdir overrides (default: auto-detect)
      TESTRUN_TIMEOUT  the runner's per-test timeout in seconds
"""

from __future__ import annotations

import atexit
import os
import re
import shlex
import shutil
import socket
import subprocess
import sys
import time
from pathlib import Path

from exitcodes import Exit   # re-exported: tests may `from pppfns import Exit`


# --- environment -----------------------------------------------------------

def _required(name: str) -> str:
    v = os.environ.get(name)
    if not v:
        sys.stderr.write(
            f"pppfns: required environment variable {name} is not set; "
            "run this test via runtests.py rather than directly.\n"
        )
        sys.exit(Exit.ERROR)
    return v


SCRATCHDIR = Path(_required('scratchdir'))
TOOLDIR = Path(_required('TOOLDIR'))
PPPD = _required('PPPD')

# The "peer" pppd binary -- used for the far side of each link. The runner
# sets PPPD_PEER to a second binary when invoked with --pppd-bin2, letting a
# run mix two pppd versions over the wire. When no second binary was
# selected, PPPD_PEER == PPPD and nothing changes.
PPPD_PEER = os.environ.get('PPPD_PEER', PPPD)

TESTRUN_TIMEOUT = int(os.environ.get('TESTRUN_TIMEOUT', '120'))

# pppd exit codes (pppd/pppd.h)
EXIT_OK = 0
EXIT_FATAL_ERROR = 1
EXIT_OPTION_ERROR = 2
EXIT_USER_REQUEST = 5
EXIT_NEGOTIATION_FAILED = 10
EXIT_PEER_AUTH_FAILED = 11
EXIT_PEER_DEAD = 15
EXIT_HANGUP = 16
EXIT_AUTH_TOPEER_FAILED = 19

# Linux runs each pppd in its own network namespace; elsewhere (Solaris)
# there are no namespaces, so both pppds share the host IP stack and only
# the protocol-level tests can run (see require_link_env).
IS_LINUX = sys.platform.startswith('linux')
IS_SUNOS = os.uname().sysname == 'SunOS'

# The kernel ppp device pppd opens differs per platform: /dev/ppp on Linux
# (the ppp_generic driver), /dev/sppp on Solaris/illumos (the sppp STREAMS
# driver -- see PPP_DRV_NAME in pppd/pppd-private.h).
PPP_DEV = '/dev/sppp' if IS_SUNOS else '/dev/ppp'

# The IP addresses used by every link test. With namespaces these never
# collide between tests, even with -j; without them, derive a per-process
# subnet (avoiding 10.0.2.x, which qemu user networking uses).
if IS_LINUX:
    IP_A = '10.0.0.1'
    IP_B = '10.0.0.2'
else:
    _n = os.getpid()
    IP_A = f'10.{100 + (_n >> 8) % 64}.{_n % 256}.1'
    IP_B = f'10.{100 + (_n >> 8) % 64}.{_n % 256}.2'


# --- result reporting ------------------------------------------------------

def test_fail(msg: str) -> 'None':
    sys.stderr.write(msg.rstrip() + '\n')
    sys.exit(Exit.FAIL)


def test_skipped(msg: str) -> 'None':
    sys.stderr.write(msg.rstrip() + '\n')
    (SCRATCHDIR / 'whyskipped').write_text(msg.rstrip() + '\n')
    sys.exit(Exit.SKIP)


def test_xfail(msg: str) -> 'None':
    sys.stderr.write(msg.rstrip() + '\n')
    sys.exit(Exit.XFAIL)


# --- privilege / environment gating ----------------------------------------

_sudo = None


def sudo_prefix() -> list:
    """Command prefix for privileged operations: [] when running as root,
    ['sudo', '-n'] when passwordless sudo works, otherwise skip the test."""
    global _sudo
    if _sudo is None:
        if os.geteuid() == 0:
            _sudo = []
        else:
            try:
                r = subprocess.run(['sudo', '-n', 'true'],
                                   capture_output=True, timeout=10)
                ok = r.returncode == 0
            except (FileNotFoundError, subprocess.TimeoutExpired):
                ok = False
            if not ok:
                test_skipped('pppd tests require root or passwordless sudo')
            _sudo = ['sudo', '-n']
    return _sudo


def require_link_env(routing: bool = False) -> 'None':
    """Skip unless this host can run a pppd pair: root/sudo, the namespace
    tools (Linux), and kernel ppp support.

    routing=True marks tests that send IP traffic across the link; those
    need per-pppd network namespaces (otherwise both negotiated addresses
    are local to one stack and traffic short-circuits via loopback), so
    they are Linux-only. Protocol-level tests run elsewhere too, but in a
    mode that writes secrets into the binary's real configuration
    directory -- only allowed when PPPD_TEST_GLOBAL_CONF=1 says the host
    is disposable (e.g. a CI VM)."""
    sudo = sudo_prefix()
    if IS_LINUX:
        tools = ('unshare', 'nsenter', 'ip', 'timeout', 'pkill')
    else:
        if routing:
            test_skipped('IP routing tests need Linux network namespaces')
        if os.environ.get('PPPD_TEST_GLOBAL_CONF') != '1':
            test_skipped('non-Linux link tests write to the real pppd '
                         'config directory; set PPPD_TEST_GLOBAL_CONF=1 '
                         'on a disposable host to allow')
        tools = ('pkill',)
    for tool in tools:
        if shutil.which(tool) is None:
            test_skipped(f'{tool} not found')
    if IS_LINUX and not os.path.exists(PPP_DEV):
        subprocess.run(sudo + ['modprobe', 'ppp_generic'], capture_output=True)
    if not os.path.exists(PPP_DEV):
        test_skipped(f'no kernel ppp support ({PPP_DEV} missing)')


def pppd_confdir(binary: str) -> str:
    """The configuration directory the binary was compiled with (the dir it
    reads options/pap-secrets/chap-secrets from). Honours the runner's
    --pppd-confdir overrides, else scans the binary for the compiled-in
    "<dir>/pap-secrets" path string."""
    if binary == PPPD and os.environ.get('PPPD_CONFDIR'):
        return os.environ['PPPD_CONFDIR']
    if binary == PPPD_PEER and os.environ.get('PPPD_CONFDIR2'):
        return os.environ['PPPD_CONFDIR2']
    with open(binary, 'rb') as f:
        data = f.read()
    m = re.search(rb'[!-~]+/pap-secrets', data)
    if m:
        return m.group(0)[:-len('/pap-secrets')].decode()
    return '/etc/ppp'


# --- pppd instances ---------------------------------------------------------

class PppPeer:
    """One pppd instance with its link on stdin/stdout (one end of a
    socketpair). On Linux it runs in its own network + mount namespace;
    elsewhere it shares the host stack and real configuration directory
    (see require_link_env)."""

    def __init__(self, name: str, binary: str, local_ip: str, remote_ip: str,
                 options=None, noauth: bool = True,
                 pap_secrets: str = None, chap_secrets: str = None,
                 scripts: dict = None):
        self.name = name
        self.binary = binary
        self.local_ip = local_ip
        self.remote_ip = remote_ip
        self.proc = None
        self.pid = None
        self.ifname = None
        self.exitcode = None
        self.netns = None
        self.conf_cleanup = []
        self.dir = SCRATCHDIR / name
        self.logfile = self.dir / 'pppd.log'
        self.pidfile = self.dir / 'pppd.pid'
        self.errfile = self.dir / 'spawn.err'

        confdir = pppd_confdir(binary)
        etc_ppp = self.dir / 'etc.ppp'
        etc_ppp.mkdir(parents=True)
        conf_files = []         # (name, mode) pairs staged by launch.sh below

        def stage(fname, text, mode):
            (self.dir / fname).write_text(text)
            if IS_LINUX:
                # pppd refuses a secrets file unless its resolved path
                # consists entirely of root-owned, non-group/other-
                # writable components, which a scratch dir under $HOME
                # can never satisfy. The check walks the realpath, so
                # symlink the file into a root-owned tmpfs dir that
                # launch.sh populates inside the mount namespace.
                (etc_ppp / fname).symlink_to(f'/run/ppp-conf/{fname}')
            else:
                # No mount namespace: launch.sh copies the file into the
                # real confdir (PPPD_TEST_GLOBAL_CONF gate); remove it
                # again in stop().
                self.conf_cleanup.append(f'{confdir}/{fname}')
            conf_files.append((fname, mode))

        for fname, text in (('pap-secrets', pap_secrets),
                            ('chap-secrets', chap_secrets)):
            if text is not None:
                stage(fname, text, '600')
        # Hook scripts pppd execs itself (ip-up, ip-down, ...). They must be
        # root-owned, non-group/other-writable and executable or
        # ppp_check_access() refuses to run them.
        for fname, text in (scripts or {}).items():
            stage(fname, text, '755')
        if IS_LINUX:
            # The bind-mounted confdir replaces the host's, so provide the
            # files pppd reads from it. An empty options file keeps the run
            # hermetic.
            (etc_ppp / 'options').write_text('')
        # Pre-create the log as the invoking user: pppd appends (O_APPEND),
        # so the file stays user-owned and scratch cleanup works unprivileged.
        self.logfile.touch()

        argv = [binary, 'notty', 'nodetach', 'local', 'debug',
                'logfile', str(self.logfile),
                # Belt-and-braces lifetime bounds: no persist, dead-peer
                # detection via LCP echos, and a cap on connected time
                # (maxconnect only arms once the link is up; the timeout
                # wrapper below is the wall-clock backstop).
                'maxconnect', str(2 * TESTRUN_TIMEOUT),
                'lcp-echo-interval', '1', 'lcp-echo-failure', '10']
        if noauth:
            argv.append('noauth')
        argv += list(options or [])
        argv.append(f'{local_ip}:{remote_ip}')
        # Root-side wall-clock watchdog: pppd dies even if it wedges before
        # the link comes up and the test runner was SIGKILLed (which can't
        # reap root processes). timeout(1) forwards SIGTERM and propagates
        # pppd's exit status, so clean-shutdown tests are unaffected.
        # Required on Linux; used elsewhere when available.
        self.has_watchdog = shutil.which('timeout') is not None
        if self.has_watchdog:
            argv = ['timeout', '--kill-after=10', str(2 * TESTRUN_TIMEOUT)] + argv

        q = shlex.quote
        script = ['#!/bin/sh', 'set -e']
        if IS_LINUX:
            # mkdir -p of a missing compiled-in confdir (e.g.
            # /usr/local/etc/ppp for a fresh build) is the one tolerated
            # host-side effect; the bind mount itself is private to this
            # mount namespace, as is the tmpfs hiding the pidfile/tdb
            # directory under /run.
            script += [
                # mode=755: the default tmpfs 1777 would itself trip pppd's
                # secrets-path safety check on /run.
                'mount -t tmpfs -o mode=755 tmpfs /run',
                '[ ! -d /var/run ] || [ -L /var/run ] || mount -t tmpfs -o mode=755 tmpfs /var/run',
                f"mkdir -p {q(confdir)}",
                f"mount --bind {q(str(etc_ppp))} {q(confdir)}",
                'mkdir -m 755 /run/ppp-conf']
            for fname, mode in conf_files:
                script += [f"cp {q(str(self.dir / fname))} /run/ppp-conf/{fname}",
                           f"chmod {mode} /run/ppp-conf/{fname}"]
            script += ['ip link set lo up']
        else:
            script += [f"mkdir -p {q(confdir)}"]
            for fname, mode in conf_files:
                dst = f'{confdir}/{fname}'
                # Never clobber a real secrets file or hook script, even on
                # an opted-in host.
                script += [f"if [ -e {q(dst)} ]; then echo {q(dst)} already exists >&2; exit 1; fi",
                           f"cp {q(str(self.dir / fname))} {q(dst)}",
                           f"chmod {mode} {q(dst)}"]
        script += [
            # $$ survives the exec, so this records the watchdog's (or
            # pppd's) pid; on Linux it also names the network namespace
            # via /proc/<pid>/ns/net.
            f"echo $$ > {q(str(self.pidfile))}",
            'exec ' + ' '.join(q(a) for a in argv)]
        launch = self.dir / 'launch.sh'
        launch.write_text('\n'.join(script) + '\n')
        self.launch = launch

    def start(self, sock) -> 'None':
        """Start pppd with its link on the given socket (one socketpair end)."""
        if IS_LINUX:
            wrapper = ['unshare', '--mount', '--net',
                       '--propagation', 'private']
        else:
            wrapper = []
        with open(self.errfile, 'w') as errfh:
            self.proc = subprocess.Popen(
                sudo_prefix() + wrapper + ['sh', str(self.launch)],
                stdin=sock, stdout=sock, stderr=errfh)
        atexit.register(self.stop)
        deadline = time.time() + 10
        while time.time() < deadline:
            # Check the pidfile before the process: a pppd that legitimately
            # ran and exited within one poll interval (e.g. a fast auth
            # reject) has still started -- tests observe it via its log and
            # exit code.
            try:
                self.pid = int(self.pidfile.read_text())
                break
            except (FileNotFoundError, ValueError):
                pass
            if self.proc.poll() is not None:
                # The pidfile may have appeared between the read above and
                # the exit; only a missing pidfile means launch.sh failed.
                try:
                    self.pid = int(self.pidfile.read_text())
                    break
                except (FileNotFoundError, ValueError):
                    test_fail(f'pppd {self.name} failed to start '
                              f'(exit {self.proc.returncode}):\n'
                              + self.errfile.read_text())
            time.sleep(0.05)
        if self.pid is None:
            test_fail(f'pppd {self.name} did not write its pid file')
        if IS_LINUX:
            self.netns = f'/proc/{self.pid}/ns/net'

    def log_text(self) -> str:
        try:
            return self.logfile.read_text()
        except OSError:
            return ''

    def wait_for_text(self, pattern: str, timeout: float = 30) -> 're.Match':
        """Wait until the pppd log matches the regex; fail with the log tail."""
        deadline = time.time() + timeout
        while True:
            m = re.search(pattern, self.log_text())
            if m:
                return m
            if time.time() >= deadline:
                tail = '\n'.join(self.log_text().splitlines()[-20:])
                test_fail(f"pppd {self.name}: no '{pattern}' in log after "
                          f"{timeout}s; log tail:\n{tail}")
            time.sleep(0.05)

    def wait_link_up(self, timeout: float = 30) -> 'None':
        """Wait for IPCP to come up with the expected addresses."""
        self.wait_for_text(rf'local +IP address {re.escape(self.local_ip)}', timeout)
        self.wait_for_text(rf'remote IP address {re.escape(self.remote_ip)}', timeout)
        m = self.wait_for_text(r'Using interface (\S+)', timeout)
        self.ifname = m.group(1)

    def _ns_argv(self, argv) -> list:
        if self.netns is None:
            return sudo_prefix() + list(argv)
        return sudo_prefix() + ['nsenter', f'--net={self.netns}', '--'] + list(argv)

    def run_in_ns(self, argv, check: bool = True, **kwargs) -> 'subprocess.CompletedProcess':
        """Run a command inside this pppd's network namespace (or on the
        host when there is no namespace)."""
        kwargs.setdefault('capture_output', True)
        kwargs.setdefault('text', True)
        r = subprocess.run(self._ns_argv(argv), **kwargs)
        if check and r.returncode != 0:
            test_fail(f'pppd {self.name}: command {argv} failed '
                      f'(exit {r.returncode}):\n{r.stdout}\n{r.stderr}')
        return r

    def popen_in_ns(self, argv, **kwargs) -> 'subprocess.Popen':
        """Start a background command inside this pppd's network namespace."""
        return subprocess.Popen(self._ns_argv(argv), **kwargs)

    def ping(self, target_ip: str, count: int = 3, size: int = None) -> int:
        """Ping target_ip from inside this peer's namespace; returns the
        number of replies received."""
        argv = ['ping', '-c', str(count), '-i', '0.2', '-W', '5']
        if size is not None:
            argv += ['-s', str(size)]
        r = self.run_in_ns(argv + [target_ip], check=False)
        m = re.search(r'(\d+) (?:packets )?received', r.stdout)
        return int(m.group(1)) if m else 0

    def ip_addr(self) -> str:
        if shutil.which('ip'):
            return self.run_in_ns(['ip', '-o', 'addr', 'show', 'dev', self.ifname]).stdout
        # Solaris: ifconfig prints "inet <local> --> <remote> ..."
        return self.run_in_ns(['ifconfig', self.ifname]).stdout

    def link_mtu(self) -> int:
        if shutil.which('ip'):
            out = self.run_in_ns(['ip', '-o', 'link', 'show', 'dev', self.ifname]).stdout
        else:
            out = self.run_in_ns(['ifconfig', self.ifname]).stdout
        m = re.search(r'mtu (\d+)', out)
        if not m:
            test_fail(f'pppd {self.name}: no mtu in: {out}')
        return int(m.group(1))

    def expect_exit(self, codes, timeout: float = 60) -> int:
        """Wait for pppd to exit of its own accord; fail unless its exit code
        is one of `codes`. sudo propagates pppd's exit status."""
        try:
            self.exitcode = self.proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self.stop()
            test_fail(f'pppd {self.name} still running after {timeout}s '
                      f'(expected exit in {sorted(codes)})')
        if self.exitcode not in codes:
            test_fail(f'pppd {self.name} exited {self.exitcode}, '
                      f'expected one of {sorted(codes)}; log tail:\n'
                      + '\n'.join(self.log_text().splitlines()[-20:]))
        return self.exitcode

    def stop(self, timeout: float = 10) -> int:
        """Terminate pppd (clean LCP shutdown via SIGTERM, then SIGKILL) and
        record its exit code."""
        if self.proc is None or self.exitcode is not None:
            return self.exitcode

        def signal_pppd(sig):
            # pppd runs as root; signal it via sudo when unprivileged. When
            # a timeout watchdog wraps pppd, the recorded pid is the
            # watchdog's, so signal its child (pppd itself, via pkill -P):
            # TERMing the wrapper would make it exit 124 and lose pppd's
            # exit status.
            if self.has_watchdog:
                subprocess.run(sudo_prefix() + ['pkill', f'-{sig}', '-P', str(self.pid)],
                               capture_output=True)
            else:
                subprocess.run(sudo_prefix() + ['kill', f'-{sig}', str(self.pid)],
                               capture_output=True)

        if self.proc.poll() is None and self.pid is not None:
            signal_pppd('TERM')
        try:
            self.exitcode = self.proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            if self.pid is not None:
                signal_pppd('KILL')
                subprocess.run(sudo_prefix() + ['kill', '-KILL', str(self.pid)],
                               capture_output=True)
            try:
                self.exitcode = self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                # Can't signal a root process whose pid we never learned;
                # the root-side timeout watchdog bounds its lifetime, so
                # don't hang the test waiting for it.
                sys.stderr.write(f'pppd {self.name}: could not reap '
                                 f'(left to its watchdog)\n')
                self.exitcode = -1
        for path in self.conf_cleanup:
            subprocess.run(sudo_prefix() + ['rm', '-f', path],
                           capture_output=True)
        self.conf_cleanup = []
        return self.exitcode


class PppPair:
    """Two pppd instances ('a' using PPPD, 'b' using PPPD_PEER) joined by a
    socketpair. Use as a context manager so both sides are torn down on any
    test failure."""

    def __init__(self, a_options=None, b_options=None,
                 a_kwargs=None, b_kwargs=None, name: str = ''):
        prefix = name + '-' if name else ''
        self.a = PppPeer(prefix + 'a', PPPD, IP_A, IP_B,
                         options=a_options, **(a_kwargs or {}))
        self.b = PppPeer(prefix + 'b', PPPD_PEER, IP_B, IP_A,
                         options=b_options, **(b_kwargs or {}))

    def start(self) -> 'PppPair':
        sock_a, sock_b = socket.socketpair(socket.AF_UNIX, socket.SOCK_STREAM)
        self.a.start(sock_a)
        self.b.start(sock_b)
        # The charshunt children hold the link now; drop our copies so the
        # pair fully closes when both pppds exit.
        sock_a.close()
        sock_b.close()
        return self

    def up(self, timeout: float = 30) -> 'PppPair':
        self.start()
        self.a.wait_link_up(timeout)
        self.b.wait_link_up(timeout)
        return self

    def down(self) -> 'None':
        self.b.stop()
        self.a.stop()

    def __enter__(self) -> 'PppPair':
        return self

    def __exit__(self, *exc) -> bool:
        self.down()
        return False
