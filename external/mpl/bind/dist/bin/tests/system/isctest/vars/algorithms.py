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

import os
import platform
import random
import subprocess
import tempfile
import time
from typing import Dict, List, NamedTuple, Optional, Union

from .basic import BASIC_VARS
from .. import log

# Algorithms are selected randomly at runtime from a list of supported
# algorithms. The randomization is deterministic and remains stable for a
# period of time for a given platform.
ALG_VARS = {
    # There are multiple algoritms sets to choose from (see ALGORITHM_SETS). To
    # override the default choice, set the ALGORITHM_SET env var prior to
    # loading this module or call set_algorithm_set().
    "ALGORITHM_SET": "none",
    "DEFAULT_ALGORITHM": "",
    "DEFAULT_ALGORITHM_NUMBER": "",
    "DEFAULT_BITS": "",
    # Alternative algorithm for test cases that require more than one algorithm
    # (for example algorithm rollover). Must be different from
    # DEFAULT_ALGORITHM.
    "ALTERNATIVE_ALGORITHM": "",
    "ALTERNATIVE_ALGORITHM_NUMBER": "",
    "ALTERNATIVE_BITS": "",
    # Algorithm that is used for tests against the "disable-algorithms"
    # configuration option. Must be different from above algorithms.
    "DISABLED_ALGORITHM": "",
    "DISABLED_ALGORITHM_NUMBER": "",
    "DISABLED_BITS": "",
    # Default HMAC algorithm. Must match the rndc configuration in
    # bin/tests/system/_common (rndc.conf, rndc.key)
    "DEFAULT_HMAC": "hmac-sha256",
}

STABLE_PERIOD = 3600 * 3
"""number of secs during which algorithm selection remains stable"""


class Algorithm(NamedTuple):
    name: str
    number: int
    bits: int


class AlgorithmSet(NamedTuple):
    """Collection of DEFAULT, ALTERNATIVE and DISABLED algorithms"""

    default: Union[Algorithm, List[Algorithm]]
    """DEFAULT is the algorithm for testing."""

    alternative: Union[Algorithm, List[Algorithm]]
    """ALTERNATIVE is an alternative algorithm for test cases that require more
    than one algorithm (for example algorithm rollover)."""

    disabled: Union[Algorithm, List[Algorithm]]
    """DISABLED is an algorithm that is used for tests against the
    "disable-algorithms" configuration option."""


RSASHA1 = Algorithm("RSASHA1", 5, 2048)
RSASHA256 = Algorithm("RSASHA256", 8, 2048)
RSASHA512 = Algorithm("RSASHA512", 10, 2048)
ECDSAP256SHA256 = Algorithm("ECDSAP256SHA256", 13, 256)
ECDSAP384SHA384 = Algorithm("ECDSAP384SHA384", 14, 384)
ED25519 = Algorithm("ED25519", 15, 256)
ED448 = Algorithm("ED448", 16, 456)

ALL_ALGORITHMS = [
    RSASHA1,
    RSASHA256,
    RSASHA512,
    ECDSAP256SHA256,
    ECDSAP384SHA384,
    ED25519,
    ED448,
]

ALL_ALGORITHMS_BY_NUM = {alg.number: alg for alg in ALL_ALGORITHMS}

ALGORITHM_SETS = {
    "stable": AlgorithmSet(
        default=ECDSAP256SHA256, alternative=RSASHA256, disabled=ECDSAP384SHA384
    ),
    "ecc_default": AlgorithmSet(
        default=[
            ECDSAP256SHA256,
            ECDSAP384SHA384,
            ED25519,
            ED448,
        ],
        alternative=RSASHA256,
        disabled=RSASHA512,
    ),
    # FUTURE The system tests needs more work before they're ready for this.
    # "random": AlgorithmSet(
    #     default=ALL_ALGORITHMS,
    #     alternative=ALL_ALGORITHMS,
    #     disabled=ALL_ALGORITHMS,
    # ),
}


def is_crypto_supported(alg: Algorithm) -> bool:
    """Test whether a given algorithm is supported on the current platform."""
    assert alg in ALL_ALGORITHMS, f"unknown algorithm: {alg}"
    with tempfile.TemporaryDirectory() as tmpdir:
        proc = subprocess.run(
            [
                BASIC_VARS["KEYGEN"],
                "-a",
                alg.name,
                "-b",
                str(alg.bits),
                "foo",
            ],
            cwd=tmpdir,
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
        if proc.returncode == 0:
            return True
        log.debug(f"dnssec-keygen stderr: {proc.stderr.decode('utf-8')}")
        log.info("algorithm %s not supported", alg.name)
        return False


# Indicate algorithm support on the current platform.
CRYPTO_SUPPORTED_VARS = {
    "RSASHA1_SUPPORTED": "0",
    "RSASHA256_SUPPORTED": "0",
    "RSASHA512_SUPPORTED": "0",
    "ECDSAP256SHA256_SUPPORTED": "0",
    "ECDSAP384SHA384_SUPPORTED": "0",
    "ED25519_SUPPORTED": "0",
    "ED448_SUPPORTED": "0",
}

SUPPORTED_ALGORITHMS: List[Algorithm] = []


def init_crypto_supported():
    """Initialize the environment variables indicating cryptography support."""
    for alg in ALL_ALGORITHMS:
        supported = is_crypto_supported(alg)
        if supported:
            SUPPORTED_ALGORITHMS.append(alg)
        envvar = f"{alg.name}_SUPPORTED"
        val = "1" if supported else "0"
        CRYPTO_SUPPORTED_VARS[envvar] = val
        os.environ[envvar] = val


def _filter_supported(algs: AlgorithmSet) -> AlgorithmSet:
    """Select supported algorithms from the set."""
    filtered = {}
    for alg_type in algs._fields:
        candidates = getattr(algs, alg_type)
        if isinstance(candidates, Algorithm):
            candidates = [candidates]
        supported = [alg for alg in candidates if alg in SUPPORTED_ALGORITHMS]
        if len(supported) == 1:
            supported = supported.pop()
        elif not supported:
            raise RuntimeError(
                f"no {alg_type.upper()} algorithm " "supported on this platform"
            )
        filtered[alg_type] = supported
    return AlgorithmSet(**filtered)


def _select_random(algs: AlgorithmSet, stable_period=STABLE_PERIOD) -> AlgorithmSet:
    """Select random DEFAULT, ALTERNATIVE and DISABLED algorithms from the set.

    The algorithm selection is deterministic for a given time period and
    platform. This should make potential issues more reproducible.

    To increase the likelyhood of detecting an issue with a given algorithm in
    CI, the current platform is used as a randomness source. When testing on
    multiple platforms at the same time, this ensures more algorithm variance
    while keeping reproducibility for a single platform.

    The function also ensures that DEFAULT, ALTERNATIVE and DISABLED algorithms
    are all different.
    """
    # FUTURE Random selection of ALTERNATIVE and DISABLED algorithms needs to
    # be implemented.
    alternative = algs.alternative
    disabled = algs.disabled
    assert isinstance(
        alternative, Algorithm
    ), "ALTERNATIVE algorithm randomization not supported yet"
    assert isinstance(
        disabled, Algorithm
    ), "DISABLED algorithm randomization not supported yet"

    # initialize randomness
    now = time.time()
    time_seed = int(now - now % stable_period)
    seed = f"{platform.platform()}_{time_seed}"
    random.seed(seed)

    # DEFAULT selection
    if isinstance(algs.default, Algorithm):
        default = algs.default
    else:
        candidates = algs.default
        for taken in [alternative, disabled]:
            try:
                candidates.remove(taken)
            except ValueError:
                pass
        assert len(candidates), "no possible choice for DEFAULT algorithm"
        random.shuffle(candidates)
        default = candidates[0]

    # Ensure only single algorithm is present for each option
    assert isinstance(default, Algorithm)
    assert isinstance(alternative, Algorithm)
    assert isinstance(disabled, Algorithm)

    assert default != alternative, "DEFAULT and ALTERNATIVE algorithms are the same"
    assert default != disabled, "DEFAULT and DISABLED algorithms are the same"
    assert alternative != disabled, "ALTERNATIVE and DISABLED algorithms are the same"

    return AlgorithmSet(default, alternative, disabled)


def _algorithms_env(algs: AlgorithmSet, name: str) -> Dict[str, str]:
    """Return environment variables with selected algorithms as a dict."""
    algs_env = {
        "ALGORITHM_SET": name,
    }

    def set_alg_env(alg: Algorithm, prefix):
        algs_env[f"{prefix}_ALGORITHM"] = alg.name
        algs_env[f"{prefix}_ALGORITHM_NUMBER"] = str(alg.number)
        algs_env[f"{prefix}_BITS"] = str(alg.bits)

    assert isinstance(algs.default, Algorithm)
    assert isinstance(algs.alternative, Algorithm)
    assert isinstance(algs.disabled, Algorithm)

    set_alg_env(algs.default, "DEFAULT")
    set_alg_env(algs.alternative, "ALTERNATIVE")
    set_alg_env(algs.disabled, "DISABLED")

    log.info("selected algorithms: %s", algs_env)
    return algs_env


def set_algorithm_set(name: Optional[str]):
    if name is None:
        name = "stable"
    assert name in ALGORITHM_SETS, f'ALGORITHM_SET "{name}" unknown'
    if name == ALG_VARS["ALGORITHM_SET"]:
        log.debug('algorithm set already configured: "%s"', name)
        return
    log.debug('choosing from ALGORITHM_SET "%s"', name)

    algs = ALGORITHM_SETS[name]
    algs = _filter_supported(algs)
    algs = _select_random(algs)
    algs_env = _algorithms_env(algs, name)

    ALG_VARS.update(algs_env)
    os.environ.update(algs_env)
