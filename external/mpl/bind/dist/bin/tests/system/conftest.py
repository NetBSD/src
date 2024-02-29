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

from functools import partial
import logging
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from typing import Any, Dict, List, Optional

import pytest


pytest.register_assert_rewrite("isctest")


# Silence warnings caused by passing a pytest fixture to another fixture.
# pylint: disable=redefined-outer-name


# ----------------- Older pytest / xdist compatibility -------------------
# As of 2023-01-11, the minimal supported pytest / xdist versions are
# determined by what is available in EL8/EPEL8:
# - pytest 3.4.2
# - pytest-xdist 1.24.1
_pytest_ver = pytest.__version__.split(".")
_pytest_major_ver = int(_pytest_ver[0])
if _pytest_major_ver < 7:
    # pytest.Stash/pytest.StashKey mechanism has been added in 7.0.0
    # for older versions, use regular dictionary with string keys instead
    FIXTURE_OK = "fixture_ok"  # type: Any
else:
    FIXTURE_OK = pytest.StashKey[bool]()  # pylint: disable=no-member

# ----------------------- Globals definition -----------------------------

LOG_FORMAT = "%(asctime)s %(levelname)7s:%(name)s  %(message)s"
XDIST_WORKER = os.environ.get("PYTEST_XDIST_WORKER", "")
FILE_DIR = os.path.abspath(Path(__file__).parent)
ENV_RE = re.compile(b"([^=]+)=(.*)")
PORT_MIN = 5001
PORT_MAX = 32767
PORTS_PER_TEST = 20
PRIORITY_TESTS = [
    # Tests that are scheduled first. Speeds up parallel execution.
    "dupsigs/",
    "rpz/",
    "rpzrecurse/",
    "serve-stale/",
    "timeouts/",
    "upforwd/",
]
PRIORITY_TESTS_RE = re.compile("|".join(PRIORITY_TESTS))
CONFTEST_LOGGER = logging.getLogger("conftest")
SYSTEM_TEST_DIR_GIT_PATH = "bin/tests/system"
SYSTEM_TEST_NAME_RE = re.compile(f"{SYSTEM_TEST_DIR_GIT_PATH}" + r"/([^/]+)")
SYMLINK_REPLACEMENT_RE = re.compile(r"/tests(_.*)\.py")

# ---------------------- Module initialization ---------------------------


def init_pytest_conftest_logger(conftest_logger):
    """
    This initializes the conftest logger which is used for pytest setup
    and configuration before tests are executed -- aka any logging in this
    file that is _not_ module-specific.
    """
    conftest_logger.setLevel(logging.DEBUG)
    file_handler = logging.FileHandler("pytest.conftest.log.txt")
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    conftest_logger.addHandler(file_handler)


init_pytest_conftest_logger(CONFTEST_LOGGER)


def avoid_duplicated_logs():
    """
    Remove direct root logger output to file descriptors.
    This default is causing duplicates because all our messages go through
    regular logging as well and are thus displayed twice.
    """
    todel = []
    for handler in logging.root.handlers:
        if handler.__class__ == logging.StreamHandler:
            # Beware: As for pytest 7.2.2, LiveLogging and LogCapture
            # handlers inherit from logging.StreamHandler
            todel.append(handler)
    for handler in todel:
        logging.root.handlers.remove(handler)


def parse_env(env_bytes):
    """Parse the POSIX env format into Python dictionary."""
    out = {}
    for line in env_bytes.splitlines():
        match = ENV_RE.match(line)
        if match:
            # EL8+ workaround for https://access.redhat.com/solutions/6994985
            # FUTURE: can be removed when we no longer need to parse env vars
            if match.groups()[0] in [b"which_declare", b"BASH_FUNC_which%%"]:
                continue
            out[match.groups()[0]] = match.groups()[1]
    return out


def get_env_bytes(cmd):
    try:
        proc = subprocess.run(
            [cmd],
            shell=True,
            check=True,
            cwd=FILE_DIR,
            stdout=subprocess.PIPE,
        )
    except subprocess.CalledProcessError as exc:
        CONFTEST_LOGGER.error("failed to get shell env: %s", exc)
        raise exc
    env_bytes = proc.stdout
    return parse_env(env_bytes)


# Read common environment variables for running tests from conf.sh.
# FUTURE: Remove conf.sh entirely and define all variables in pytest only.
CONF_ENV = get_env_bytes(". ./conf.sh && env")
os.environb.update(CONF_ENV)
CONFTEST_LOGGER.debug("variables in env: %s", ", ".join([str(key) for key in CONF_ENV]))

# --------------------------- pytest hooks -------------------------------


def pytest_addoption(parser):
    parser.addoption(
        "--noclean",
        action="store_true",
        default=False,
        help="don't remove the temporary test directories with artifacts",
    )


def pytest_configure(config):
    # Ensure this hook only runs on the main pytest instance if xdist is
    # used to spawn other workers.
    if not XDIST_WORKER:
        if config.pluginmanager.has_plugin("xdist") and config.option.numprocesses:
            # system tests depend on module scope for setup & teardown
            # enforce use "loadscope" scheduler or disable paralelism
            try:
                import xdist.scheduler.loadscope  # pylint: disable=unused-import
            except ImportError:
                CONFTEST_LOGGER.debug(
                    "xdist is too old and does not have "
                    "scheduler.loadscope, disabling parallelism"
                )
                config.option.dist = "no"
            else:
                config.option.dist = "loadscope"


def pytest_ignore_collect(path):
    # System tests are executed in temporary directories inside
    # bin/tests/system. These temporary directories contain all files
    # needed for the system tests - including tests_*.py files. Make sure to
    # ignore these during test collection phase. Otherwise, test artifacts
    # from previous runs could mess with the runner. Also ignore the
    # convenience symlinks to those test directories. In both of those
    # cases, the system test name (directory) contains an underscore, which
    # is otherwise and invalid character for a system test name.
    match = SYSTEM_TEST_NAME_RE.search(str(path))
    if match is None:
        CONFTEST_LOGGER.warning("unexpected test path: %s (ignored)", path)
        return True
    system_test_name = match.groups()[0]
    return "_" in system_test_name


def pytest_collection_modifyitems(items):
    """Schedule long-running tests first to get more benefit from parallelism."""
    priority = []
    other = []
    for item in items:
        if PRIORITY_TESTS_RE.search(item.nodeid):
            priority.append(item)
        else:
            other.append(item)
    items[:] = priority + other


class NodeResult:
    def __init__(self, report=None):
        self.outcome = None
        self.messages = []
        if report is not None:
            self.update(report)

    def update(self, report):
        if self.outcome is None or report.outcome != "passed":
            self.outcome = report.outcome
        if report.longreprtext:
            self.messages.append(report.longreprtext)


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item):
    """Hook that is used to expose test results to session (for use in fixtures)."""
    # execute all other hooks to obtain the report object
    outcome = yield
    report = outcome.get_result()

    # Set the test outcome in session, so we can access it from module-level
    # fixture using nodeid. Note that this hook is called three times: for
    # setup, call and teardown. We only care about the overall result so we
    # merge the results together and preserve the information whether a test
    # passed.
    test_results = {}
    try:
        test_results = getattr(item.session, "test_results")
    except AttributeError:
        setattr(item.session, "test_results", test_results)
    node_result = test_results.setdefault(item.nodeid, NodeResult())
    node_result.update(report)


# --------------------------- Fixtures -----------------------------------


@pytest.fixture(scope="session")
def modules():
    """
    Sorted list of ALL modules.

    The list includes even test modules that are not tested in the current
    session. It is used to determine port distribution. Using a complete
    list of all possible test modules allows independent concurrent pytest
    invocations.
    """
    mods = []
    for dirpath, _dirs, files in os.walk(FILE_DIR):
        for file in files:
            if file.startswith("tests_") and file.endswith(".py"):
                mod = f"{dirpath}/{file}"
                if not pytest_ignore_collect(mod):
                    mods.append(mod)
    return sorted(mods)


@pytest.fixture(scope="session")
def module_base_ports(modules):
    """
    Dictionary containing assigned base port for every module.

    The port numbers are deterministically assigned before any testing
    starts. This fixture MUST return the same value when called again
    during the same test session. When running tests in parallel, this is
    exactly what happens - every worker thread will call this fixture to
    determine test ports.
    """
    port_min = PORT_MIN
    port_max = PORT_MAX - len(modules) * PORTS_PER_TEST
    if port_max < port_min:
        raise RuntimeError("not enough ports to assign unique port set to each module")

    # Rotate the base port value over time to detect possible test issues
    # with using random ports. This introduces a very slight race condition
    # risk. If this value changes between pytest invocation and spawning
    # worker threads, multiple tests may have same port values assigned. If
    # these tests are then executed simultaneously, the test results will
    # be misleading.
    base_port = int(time.time() // 3600) % (port_max - port_min) + port_min

    return {mod: base_port + i * PORTS_PER_TEST for i, mod in enumerate(modules)}


@pytest.fixture(scope="module")
def base_port(request, module_base_ports):
    """Start of the port range assigned to a particular test module."""
    port = module_base_ports[request.fspath]
    return port


@pytest.fixture(scope="module")
def ports(base_port):
    """Dictionary containing port names and their assigned values."""
    return {
        "PORT": base_port,
        "TLSPORT": base_port + 1,
        "HTTPPORT": base_port + 2,
        "HTTPSPORT": base_port + 3,
        "EXTRAPORT1": base_port + 4,
        "EXTRAPORT2": base_port + 5,
        "EXTRAPORT3": base_port + 6,
        "EXTRAPORT4": base_port + 7,
        "EXTRAPORT5": base_port + 8,
        "EXTRAPORT6": base_port + 9,
        "EXTRAPORT7": base_port + 10,
        "EXTRAPORT8": base_port + 11,
        "CONTROLPORT": base_port + 12,
    }


@pytest.fixture(scope="module")
def named_port(ports):
    return ports["PORT"]


@pytest.fixture(scope="module")
def named_tlsport(ports):
    return ports["TLSPORT"]


@pytest.fixture(scope="module")
def named_httpsport(ports):
    return ports["HTTPSPORT"]


@pytest.fixture(scope="module")
def control_port(ports):
    return ports["CONTROLPORT"]


@pytest.fixture(scope="module")
def env(ports):
    """Dictionary containing environment variables for the test."""
    env = os.environ.copy()
    for portname, portnum in ports.items():
        env[portname] = str(portnum)
    env["builddir"] = f"{env['TOP_BUILDDIR']}/{SYSTEM_TEST_DIR_GIT_PATH}"
    env["srcdir"] = f"{env['TOP_SRCDIR']}/{SYSTEM_TEST_DIR_GIT_PATH}"
    return env


@pytest.fixture(scope="module")
def system_test_name(request):
    """Name of the system test directory."""
    path = Path(request.fspath)
    return path.parent.name


@pytest.fixture(scope="module")
def mlogger(system_test_name):
    """Logging facility specific to this test module."""
    avoid_duplicated_logs()
    return logging.getLogger(system_test_name)


@pytest.fixture
def logger(request, system_test_name):
    """Logging facility specific to a particular test."""
    return logging.getLogger(f"{system_test_name}.{request.node.name}")


@pytest.fixture(scope="module")
def system_test_dir(
    request, env, system_test_name, mlogger
):  # pylint: disable=too-many-statements,too-many-locals
    """
    Temporary directory for executing the test.

    This fixture is responsible for creating (and potentially removing) a
    copy of the system test directory which is used as a temporary
    directory for the test execution.

    FUTURE: This removes the need to have clean.sh scripts.
    """

    def get_test_result():
        """Aggregate test results from all individual tests from this module
        into a single result: failed > skipped > passed."""
        try:
            all_test_results = request.session.test_results
        except AttributeError:
            # This may happen if pytest execution is interrupted and
            # pytest_runtest_makereport() is never called.
            mlogger.debug("can't obtain test results, test run was interrupted")
            return "error"
        test_results = {
            node.nodeid: all_test_results[node.nodeid]
            for node in request.node.collect()
            if node.nodeid in all_test_results
        }
        assert len(test_results)
        messages = []
        for node, result in test_results.items():
            mlogger.debug("%s %s", result.outcome.upper(), node)
            messages.extend(result.messages)
        for message in messages:
            mlogger.debug("\n" + message)
        failed = any(res.outcome == "failed" for res in test_results.values())
        skipped = any(res.outcome == "skipped" for res in test_results.values())
        if failed:
            return "failed"
        if skipped:
            return "skipped"
        assert all(res.outcome == "passed" for res in test_results.values())
        return "passed"

    def unlink(path):
        try:
            path.unlink()  # missing_ok=True isn't available on Python 3.6
        except FileNotFoundError:
            pass

    # Create a temporary directory with a copy of the original system test dir contents
    system_test_root = Path(f"{env['TOP_BUILDDIR']}/{SYSTEM_TEST_DIR_GIT_PATH}")
    testdir = Path(
        tempfile.mkdtemp(prefix=f"{system_test_name}_tmp_", dir=system_test_root)
    )
    shutil.rmtree(testdir)
    shutil.copytree(system_test_root / system_test_name, testdir)

    # Create a convenience symlink with a stable and predictable name
    module_name = SYMLINK_REPLACEMENT_RE.sub(r"\1", request.node.name)
    symlink_dst = system_test_root / module_name
    unlink(symlink_dst)
    symlink_dst.symlink_to(os.path.relpath(testdir, start=system_test_root))

    # Configure logger to write to a file inside the temporary test directory
    mlogger.handlers.clear()
    mlogger.setLevel(logging.DEBUG)
    handler = logging.FileHandler(testdir / "pytest.log.txt", mode="w")
    formatter = logging.Formatter(LOG_FORMAT)
    handler.setFormatter(formatter)
    mlogger.addHandler(handler)

    # System tests are meant to be executed from their directory - switch to it.
    old_cwd = os.getcwd()
    os.chdir(testdir)
    mlogger.debug("switching to tmpdir: %s", testdir)
    try:
        yield testdir  # other fixtures / tests will execute here
    finally:
        os.chdir(old_cwd)
        mlogger.debug("changed workdir to: %s", old_cwd)

        result = get_test_result()

        # Clean temporary dir unless it should be kept
        keep = False
        if request.config.getoption("--noclean"):
            mlogger.debug(
                "--noclean requested, keeping temporary directory %s", testdir
            )
            keep = True
        elif result == "failed":
            mlogger.debug(
                "test failure detected, keeping temporary directory %s", testdir
            )
            keep = True
        elif not request.node.stash[FIXTURE_OK]:
            mlogger.debug(
                "test setup/teardown issue detected, keeping temporary directory %s",
                testdir,
            )
            keep = True

        if keep:
            mlogger.info(
                "test artifacts in: %s", symlink_dst.relative_to(system_test_root)
            )
        else:
            mlogger.debug("deleting temporary directory")
            handler.flush()
            handler.close()
            shutil.rmtree(testdir)
            unlink(symlink_dst)


def _run_script(  # pylint: disable=too-many-arguments
    env,
    mlogger,
    system_test_dir: Path,
    interpreter: str,
    script: str,
    args: Optional[List[str]] = None,
):
    """Helper function for the shell / perl script invocations (through fixtures below)."""
    if args is None:
        args = []
    path = Path(script)
    if not path.is_absolute():
        # make sure relative paths are always relative to system_dir
        path = system_test_dir.parent / path
    script = str(path)
    cwd = os.getcwd()
    if not path.exists():
        raise FileNotFoundError(f"script {script} not found in {cwd}")
    mlogger.debug("running script: %s %s %s", interpreter, script, " ".join(args))
    mlogger.debug("  workdir: %s", cwd)
    returncode = 1

    cmd = [interpreter, script] + args
    with subprocess.Popen(
        cmd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
        errors="backslashreplace",
    ) as proc:
        if proc.stdout:
            for line in proc.stdout:
                mlogger.info("    %s", line.rstrip("\n"))
        proc.communicate()
        returncode = proc.returncode
        if returncode:
            raise subprocess.CalledProcessError(returncode, cmd)
        mlogger.debug("  exited with %d", returncode)


@pytest.fixture(scope="module")
def shell(env, system_test_dir, mlogger):
    """Function to call a shell script with arguments."""
    return partial(_run_script, env, mlogger, system_test_dir, env["SHELL"])


@pytest.fixture(scope="module")
def perl(env, system_test_dir, mlogger):
    """Function to call a perl script with arguments."""
    return partial(_run_script, env, mlogger, system_test_dir, env["PERL"])


@pytest.fixture(scope="module")
def run_tests_sh(system_test_dir, shell):
    """Utility function to execute tests.sh as a python test."""

    def run_tests():
        shell(f"{system_test_dir}/tests.sh")

    return run_tests


@pytest.fixture(scope="module", autouse=True)
def system_test(  # pylint: disable=too-many-arguments,too-many-statements
    request,
    env: Dict[str, str],
    mlogger,
    system_test_dir,
    shell,
    perl,
):
    """
    Driver of the test setup/teardown process. Used automatically for every test module.

    This is the most important one-fixture-to-rule-them-all. Note the
    autouse=True which causes this fixture to be loaded by every test
    module without the need to explicitly specify it.

    When this fixture is used, it utilizes other fixtures, such as
    system_test_dir, which handles the creation of the temporary test
    directory.

    Afterwards, it checks the test environment and takes care of starting
    the servers. When everything is ready, that's when the actual tests are
    executed. Once that is done, this fixture stops the servers and checks
    for any artifacts indicating an issue (e.g. coredumps).

    Finally, when this fixture reaches an end (or encounters an exception,
    which may be caused by fail/skip invocations), any fixtures which is
    used by this one are finalized - e.g. system_test_dir performs final
    checks and cleans up the temporary test directory.
    """

    def check_net_interfaces():
        try:
            perl("testsock.pl", ["-p", env["PORT"]])
        except subprocess.CalledProcessError as exc:
            mlogger.error("testsock.pl: exited with code %d", exc.returncode)
            pytest.skip("Network interface aliases not set up.")

    def check_prerequisites():
        try:
            shell(f"{system_test_dir}/prereq.sh")
        except FileNotFoundError:
            pass  # prereq.sh is optional
        except subprocess.CalledProcessError:
            pytest.skip("Prerequisites missing.")

    def setup_test():
        try:
            shell(f"{system_test_dir}/setup.sh")
        except FileNotFoundError:
            pass  # setup.sh is optional
        except subprocess.CalledProcessError as exc:
            mlogger.error("Failed to run test setup")
            pytest.fail(f"setup.sh exited with {exc.returncode}")

    def start_servers():
        try:
            perl("start.pl", ["--port", env["PORT"], system_test_dir.name])
        except subprocess.CalledProcessError as exc:
            mlogger.error("Failed to start servers")
            pytest.fail(f"start.pl exited with {exc.returncode}")

    def stop_servers():
        try:
            perl("stop.pl", [system_test_dir.name])
        except subprocess.CalledProcessError as exc:
            mlogger.error("Failed to stop servers")
            get_core_dumps()
            pytest.fail(f"stop.pl exited with {exc.returncode}")

    def get_core_dumps():
        try:
            shell("get_core_dumps.sh", [system_test_dir.name])
        except subprocess.CalledProcessError as exc:
            mlogger.error("Found core dumps or sanitizer reports")
            pytest.fail(f"get_core_dumps.sh exited with {exc.returncode}")

    os.environ.update(env)  # Ensure pytests have the same env vars as shell tests.
    mlogger.info(f"test started: {request.node.name}")
    port = int(env["PORT"])
    mlogger.info("using port range: <%d, %d>", port, port + PORTS_PER_TEST - 1)

    if not hasattr(request.node, "stash"):  # compatibility with pytest<7.0.0
        request.node.stash = {}  # use regular dict instead of pytest.Stash
    request.node.stash[FIXTURE_OK] = True

    # Perform checks which may skip this test.
    check_net_interfaces()
    check_prerequisites()

    # Store the fact that this fixture hasn't successfully finished yet.
    # This is checked before temporary directory teardown to decide whether
    # it's okay to remove the directory.
    request.node.stash[FIXTURE_OK] = False

    setup_test()
    try:
        start_servers()
        mlogger.debug("executing test(s)")
        yield
    finally:
        mlogger.debug("test(s) finished")
        stop_servers()
        get_core_dumps()
        request.node.stash[FIXTURE_OK] = True
