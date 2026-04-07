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

from isctest.run import EnvCmd

import isctest


def bootstrap():
    templates = isctest.template.TemplateEngine(".")
    keygen = EnvCmd("KEYGEN", "-a ECDSA256")
    signer = EnvCmd("SIGNER")

    isctest.log.info("setup iter-too-many.")
    zonename = "iter-too-many."
    ksk_name = keygen(f"-f KSK {zonename}", cwd="ns2").out.strip()
    zsk_name = keygen(f"{zonename}", cwd="ns2").out.strip()
    ksk = isctest.kasp.Key(ksk_name, keydir="ns2")
    zsk = isctest.kasp.Key(zsk_name, keydir="ns2")
    dnskeys = [ksk.dnskey, zsk.dnskey]

    tdata = {
        "dnskeys": dnskeys,
    }
    templates.render(f"ns2/{zonename}db", tdata, template=f"ns2/{zonename}db.j2.manual")
    signer(
        f"-P -o {zonename} -f {zonename}signed.db -3 A1B2C3D4 -H too-many -H 51 -S {zonename}db",
        cwd="ns2",
    )

    return {
        "trust_anchors": [
            ksk.into_ta("static-key"),
        ],
    }


def test_excessive_nsec3_iterations_delegation(ns3):
    # reproducer for CVE-2026-1519 [GL#5708]
    zone = "example.sub.iter-too-many"
    msg = isctest.query.create(zone, "A")
    res = isctest.query.tcp(msg, ns3.ip)

    # an insecure response is expected regardless of the NSEC3 iteration limit,
    # because the sub.iter-too-many. zone is unsigned. the real difference is
    # in the CPU usage required for generating such response, but that can't be
    # easily and reliably tested in an automated fashion
    isctest.check.noerror(res)

    with ns3.watch_log_from_start() as watcher:
        watcher.wait_for_line(
            f"validating {zone}/A: validator_callback_ds: too many iterations"
        )
