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

from .. import log

PORT_MIN = 5001
PORT_MAX = 32767
PORTS_PER_TEST = 20

PORT_VARS = {
    "PORT": "5300",
    "TLSPORT": "5301",
    "HTTPPORT": "5302",
    "HTTPSPORT": "5303",
    "EXTRAPORT1": "5304",
    "EXTRAPORT2": "5305",
    "EXTRAPORT3": "5306",
    "EXTRAPORT4": "5307",
    "EXTRAPORT5": "5308",
    "EXTRAPORT6": "5309",
    "EXTRAPORT7": "5310",
    "EXTRAPORT8": "5311",
    "CONTROLPORT": "5312",
}


def set_base_port(base_port: int):
    log.debug(f"setting base port {base_port}")
    assert base_port >= PORT_MIN
    assert base_port <= PORT_MAX
    PORT_VARS["PORT"] = str(base_port)
    PORT_VARS["TLSPORT"] = str(base_port + 1)
    PORT_VARS["HTTPPORT"] = str(base_port + 2)
    PORT_VARS["HTTPSPORT"] = str(base_port + 3)
    PORT_VARS["EXTRAPORT1"] = str(base_port + 4)
    PORT_VARS["EXTRAPORT2"] = str(base_port + 5)
    PORT_VARS["EXTRAPORT3"] = str(base_port + 6)
    PORT_VARS["EXTRAPORT4"] = str(base_port + 7)
    PORT_VARS["EXTRAPORT5"] = str(base_port + 8)
    PORT_VARS["EXTRAPORT6"] = str(base_port + 9)
    PORT_VARS["EXTRAPORT7"] = str(base_port + 10)
    PORT_VARS["EXTRAPORT8"] = str(base_port + 11)
    PORT_VARS["CONTROLPORT"] = str(base_port + 12)
    os.environ.update(PORT_VARS)
