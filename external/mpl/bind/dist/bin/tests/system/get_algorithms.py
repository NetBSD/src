#!/usr/bin/python3

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

# This script is a 'port' broker.  It keeps track of ports given to the
# individual system subtests, so every test is given a unique port range.

import logging
import os
from pathlib import Path
import platform
import random
import subprocess
import time
from typing import Dict, List, NamedTuple, Union

# Uncomment to enable DEBUG logging
# logging.basicConfig(
#     format="get_algorithms.py %(levelname)s %(message)s", level=logging.DEBUG
# )

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


RSASHA1 = Algorithm("RSASHA1", 5, 1280)
RSASHA256 = Algorithm("RSASHA256", 8, 1280)
RSASHA512 = Algorithm("RSASHA512", 10, 1280)
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

TESTCRYPTO = Path(__file__).resolve().parent / "testcrypto.sh"

KEYGEN = os.getenv("KEYGEN", "")
if not KEYGEN:
    raise RuntimeError("KEYGEN environment variable has to be set")

ALGORITHM_SET = os.getenv("ALGORITHM_SET", "stable")
assert ALGORITHM_SET in ALGORITHM_SETS, f'ALGORITHM_SET "{ALGORITHM_SET}" unknown'
logging.debug('choosing from ALGORITHM_SET "%s"', ALGORITHM_SET)


def is_supported(alg: Algorithm) -> bool:
    """Test whether a given algorithm is supported on the current platform."""
    try:
        subprocess.run(
            f"{TESTCRYPTO} -q {alg.name}",
            shell=True,
            check=True,
            env={"KEYGEN": KEYGEN},
            stdout=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError as exc:
        logging.debug(exc)
        logging.info("algorithm %s not supported", alg.name)
        return False
    return True


def filter_supported(algs: AlgorithmSet) -> AlgorithmSet:
    """Select supported algorithms from the set."""
    filtered = {}
    for alg_type in algs._fields:
        candidates = getattr(algs, alg_type)
        if isinstance(candidates, Algorithm):
            candidates = [candidates]
        supported = list(filter(is_supported, candidates))
        if len(supported) == 1:
            supported = supported.pop()
        elif not supported:
            raise RuntimeError(
                f'no {alg_type.upper()} algorithm from "{ALGORITHM_SET}" set '
                "supported on this platform"
            )
        filtered[alg_type] = supported
    return AlgorithmSet(**filtered)


def select_random(algs: AlgorithmSet, stable_period=STABLE_PERIOD) -> AlgorithmSet:
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


def algorithms_env(algs: AlgorithmSet) -> Dict[str, str]:
    """Return environment variables with selected algorithms as a dict."""
    algs_env: Dict[str, str] = {}

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

    logging.info("selected algorithms: %s", algs_env)
    return algs_env


def main():
    disable_checking = int(os.getenv("DISABLE_ALGORITHM_SUPPORT_CHECKING", "0"))
    try:
        algs = ALGORITHM_SETS[ALGORITHM_SET]
        if not disable_checking:
            algs = filter_supported(algs)
        algs = select_random(algs)
        algs_env = algorithms_env(algs)
    except Exception:
        # if anything goes wrong, the conf.sh ignores error codes, so make sure
        # we set an environment variable to an error value that can be checked
        # later by run.sh
        print("export ALGORITHM_SET=error")
        raise
    else:
        for name, value in algs_env.items():
            print(f"export {name}={value}")


if __name__ == "__main__":
    main()
