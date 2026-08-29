#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from dnssec_wildcard.ans1 import common, f043, f045
from isctest.asyncserver import AsyncDnsServer


def main() -> None:
    keys = common.load_keys()
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handler(f043.F043Handler(keys))
    server.install_response_handler(f045.F045Handler(keys))
    server.run()


if __name__ == "__main__":
    main()
