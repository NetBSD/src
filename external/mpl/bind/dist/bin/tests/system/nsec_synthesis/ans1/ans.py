#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from isctest.asyncserver import AsyncDnsServer
from nsec_synthesis.ans1 import common, f004, f023


def main() -> None:
    keys = common.load_keys()
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handler(f004.F004Handler(keys))
    server.install_response_handler(f023.F023Handler(keys))
    server.run()


if __name__ == "__main__":
    main()
