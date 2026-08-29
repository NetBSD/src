#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from dnssec_nsec3.ans1 import common, f025, f055
from isctest.asyncserver import AsyncDnsServer


def main() -> None:
    keys = common.load_keys()
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handler(f025.F025Handler(keys))
    server.install_response_handler(f055.F055Handler(keys))
    server.run()


if __name__ == "__main__":
    main()
