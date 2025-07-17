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

import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "axfr.out",
        "dig.out.*",
        "stats.*",
        "wait_for_message.*",
        "ans*/ans.run",
        "ns1/dot-fallback.db",
        "ns1/edns-expire.db",
        "ns1/ixfr-too-big.db",
        "ns1/ixfr-too-big.db.jnl",
        "ns1/ixfr-too-many-diffs.db.jnl",
        "ns1/sec.db",
        "ns2/dot-fallback.db",
        "ns2/example.db",
        "ns2/example.db.jnl",
        "ns2/mapped.db",
        "ns2/sec.db",
        "ns2/tsigzone.db",
        "ns3/example.bk",
        "ns3/example.bk.jnl",
        "ns3/mapped.bk",
        "ns3/primary.bk",
        "ns3/primary.bk.jnl",
        "ns3/tsigzone.bk",
        "ns3/xfer-stats.bk",
        "ns4/nil.db",
        "ns4/root.db",
        "ns6/axfr-max-idle-time.bk",
        "ns6/axfr-max-transfer-time.bk",
        "ns6/axfr-min-transfer-rate.bk",
        "ns6/axfr-rndc-retransfer-force.bk",
        "ns6/edns-expire.bk",
        "ns6/ixfr-too-big.bk",
        "ns6/ixfr-too-big.bk.jnl",
        "ns6/ixfr-too-many-diffs.bk",
        "ns6/primary.db",
        "ns6/primary.db.jnl",
        "ns6/sec.bk",
        "ns6/xot-primary-try-next.bk",
        "ns6/xfr-and-reconfig.bk",
        "ns7/edns-expire.bk",
        "ns7/primary2.db",
        "ns7/sec.bk",
        "ns7/sec.bk.jnl",
        "ns8/large.db",
        "ns8/small.db",
    ]
)


def test_xfer(run_tests_sh):
    run_tests_sh()
