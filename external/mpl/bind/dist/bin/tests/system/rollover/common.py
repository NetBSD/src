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

from datetime import timedelta
import os

import pytest

from isctest.kasp import Ipub, IpubC, Iret
from isctest.vars.algorithms import Algorithm

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.axfr*",
        "dig.out*",
        "K*.key*",
        "K*.private*",
        "ns*/*.db",
        "ns*/*.db.infile",
        "ns*/*.db.jnl",
        "ns*/*.db.jbk",
        "ns*/*.db.signed",
        "ns*/*.db.signed.jnl",
        "ns*/*.conf",
        "ns*/dsset-*",
        "ns*/K*.key",
        "ns*/K*.private",
        "ns*/K*.state",
        "ns*/keygen.out.*",
        "ns*/managed-keys.**",
        "ns*/settime.out.*",
        "ns*/signer.out.*",
        "ns*/zones",
        "ns1/root.db.in",
    ]
)


TIMEDELTA = {
    0: timedelta(seconds=0),
    "PT5M": timedelta(minutes=5),
    "PT20M": timedelta(minutes=20),
    "PT1H": timedelta(hours=1),
    "PT2H": timedelta(hours=2),
    "PT6H": timedelta(hours=6),
    "PT12H": timedelta(hours=12),
    "P1D": timedelta(days=1),
    "P2D": timedelta(days=2),
    "P5D": timedelta(days=5),
    "P7D": timedelta(days=7),
    "P10D": timedelta(days=10),
    "P14D": timedelta(days=14),
    "P30D": timedelta(days=30),
    "P60D": timedelta(days=60),
    "P90D": timedelta(days=90),
    "P6M": timedelta(days=31 * 6),
    "P1Y": timedelta(days=365),
}
DURATION = {isoname: int(delta.total_seconds()) for isoname, delta in TIMEDELTA.items()}
CDSS = ["CDNSKEY", "CDS (SHA-256)"]
DEFAULT_CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT1H"],
    "ds-ttl": TIMEDELTA["P1D"],
    "max-zone-ttl": TIMEDELTA["P1D"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["PT1H"],
    "purge-keys": TIMEDELTA["P90D"],
    "retire-safety": TIMEDELTA["PT1H"],
    "signatures-refresh": TIMEDELTA["P5D"],
    "signatures-validity": TIMEDELTA["P14D"],
    "zone-propagation-delay": TIMEDELTA["PT5M"],
}
UNSIGNING_CONFIG = DEFAULT_CONFIG.copy()
UNSIGNING_CONFIG["dnskey-ttl"] = TIMEDELTA["PT2H"]
ALGOROLL_CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT1H"],
    "ds-ttl": TIMEDELTA["PT2H"],
    "max-zone-ttl": TIMEDELTA["PT6H"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["PT1H"],
    "purge-keys": TIMEDELTA["P90D"],
    "retire-safety": TIMEDELTA["PT2H"],
    "signatures-refresh": TIMEDELTA["P5D"],
    "signatures-validity": TIMEDELTA["P30D"],
    "zone-propagation-delay": TIMEDELTA["PT1H"],
}
ALGOROLL_IPUB = Ipub(ALGOROLL_CONFIG)
ALGOROLL_IPUBC = IpubC(ALGOROLL_CONFIG, rollover=False)
ALGOROLL_IRET = Iret(ALGOROLL_CONFIG, rollover=False)
ALGOROLL_IRETKSK = Iret(ALGOROLL_CONFIG, zsk=False, ksk=True, rollover=False)
ALGOROLL_KEYTTLPROP = (
    ALGOROLL_CONFIG["dnskey-ttl"] + ALGOROLL_CONFIG["zone-propagation-delay"]
)
ALGOROLL_OFFSETS = {}
ALGOROLL_OFFSETS["step2"] = -int(ALGOROLL_IPUB.total_seconds())
ALGOROLL_OFFSETS["step3"] = -int(ALGOROLL_IRET.total_seconds())
ALGOROLL_OFFSETS["step4"] = ALGOROLL_OFFSETS["step3"] - int(
    ALGOROLL_IRETKSK.total_seconds()
)
ALGOROLL_OFFSETS["step5"] = ALGOROLL_OFFSETS["step4"] - int(
    ALGOROLL_KEYTTLPROP.total_seconds()
)
ALGOROLL_OFFSETS["step6"] = ALGOROLL_OFFSETS["step5"] - int(
    ALGOROLL_IRET.total_seconds()
)
ALGOROLL_OFFVAL = -DURATION["P7D"]
KSK_CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT2H"],
    "ds-ttl": TIMEDELTA["PT1H"],
    "max-zone-ttl": TIMEDELTA["P1D"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["P1D"],
    "purge-keys": TIMEDELTA["PT1H"],
    "retire-safety": TIMEDELTA["P2D"],
    "signatures-refresh": TIMEDELTA["P7D"],
    "signatures-validity": TIMEDELTA["P14D"],
    "zone-propagation-delay": TIMEDELTA["PT1H"],
}
KSK_LIFETIME = TIMEDELTA["P60D"]
KSK_LIFETIME_POLICY = int(KSK_LIFETIME.total_seconds())
KSK_IPUB = Ipub(KSK_CONFIG)
KSK_IPUBC = IpubC(KSK_CONFIG)
KSK_IRET = Iret(KSK_CONFIG, zsk=False, ksk=True)
KSK_KEYTTLPROP = KSK_CONFIG["dnskey-ttl"] + KSK_CONFIG["zone-propagation-delay"]


@pytest.fixture
def alg():
    return os.environ["DEFAULT_ALGORITHM_NUMBER"]


@pytest.fixture
def size():
    return os.environ["DEFAULT_BITS"]


def default_algorithm():
    return Algorithm(
        os.environ["DEFAULT_ALGORITHM"],
        int(os.environ["DEFAULT_ALGORITHM_NUMBER"]),
        int(os.environ["DEFAULT_BITS"]),
    )
