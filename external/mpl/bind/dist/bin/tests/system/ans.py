"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.

This is a bare-bones DNS server that only serves data from zone files.  It is
meant to be used as a replacement for full-blown named instances in system
tests when a given server is only required to return zone-based data.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 BEWARE!  THIS SERVER DOES NOT NECESSARILY RETURN PROTOCOL-COMPLIANT ANSWERS!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

See AsyncDnsServer._abort_if_*() methods in isctests/asyncserver.py for more
details.  Use a regular named instance for anything non-trivial.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 DO NOT ADD CUSTOM LOGIC TO THIS FILE.  IT IS ONLY MEANT TO BE SYMLINKED INTO
ansX/ SUBDIRECTORIES IN SYSTEM TESTS TO REDUCE THE AMOUNT OF BOILERPLATE CODE.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

If you need to customize server behavior, implement it in a dedicated ans.py
server in the system test at hand.  If an extension you are working on can be
useful in other system tests, please consider opening a merge request extending
isctest/asyncserver.py.
"""

from isctest.asyncserver import AsyncDnsServer


def main() -> None:
    server = AsyncDnsServer()
    server.run()


if __name__ == "__main__":
    main()
