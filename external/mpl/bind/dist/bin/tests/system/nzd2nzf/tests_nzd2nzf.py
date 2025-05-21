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
import pytest

import isctest
import isctest.mark
import isctest.run

import dns.message

pytestmark = [
    isctest.mark.with_lmdb,
    pytest.mark.extra_artifacts(
        ["ns1/_default.nzd", "ns1/_default.nzf~"],
    ),
]


def test_nzd2nzf(servers):
    zone_data = '"added.example" { type primary; file "added.db"; };'
    msg = dns.message.make_query("a.added.example.", "A")

    # query for non-existing zone data
    res = isctest.query.tcp(msg, servers["ns1"].ip)
    isctest.check.refused(res)

    # add new zone into the default NZD using "rndc addzone"
    servers["ns1"].rndc(f"addzone {zone_data}", log=False)

    # query for existing zone data
    res = isctest.query.tcp(msg, servers["ns1"].ip)
    isctest.check.noerror(res)

    servers["ns1"].stop()

    # dump "_default.nzd" to "_default.nzf" and check that it contains the expected content
    cfg_dir = "ns1"
    stdout = isctest.run.cmd(
        [os.environ["NZD2NZF"], "_default.nzd"], cwd=cfg_dir
    ).stdout.decode("utf-8")
    assert f"zone {zone_data}" in stdout
    nzf_filename = os.path.join(cfg_dir, "_default.nzf")
    with open(nzf_filename, "w", encoding="utf-8") as nzf_file:
        nzf_file.write(stdout)

    # delete "_default.nzd" database
    nzd_filename = os.path.join(cfg_dir, "_default.nzd")
    os.remove(nzd_filename)

    # start ns1 again, it should migrate "_default.nzf" to "_default.nzd"
    servers["ns1"].start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    # query for zone data from the migrated zone config
    res = isctest.query.tcp(msg, servers["ns1"].ip)
    isctest.check.noerror(res)
