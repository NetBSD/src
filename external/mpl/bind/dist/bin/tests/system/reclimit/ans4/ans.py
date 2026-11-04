"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

import dns.rcode

from isctest.asyncserver import ControllableAsyncDnsServer

from ..reclimit_ans import (
    DirectExampleHandler,
    FallbackNxdomainHandler,
    IndirectExampleOrgHandler,
    LimitControlCommand,
    Ns1ExampleOrgHandler,
    ReclimitStateHandler,
)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_response_handlers(
        state_handler := ReclimitStateHandler(),
        DirectExampleHandler(state_handler, 4),
        IndirectExampleOrgHandler(state_handler, 4),
        Ns1ExampleOrgHandler(state_handler),
        FallbackNxdomainHandler(state_handler),
    )
    server.install_control_command(LimitControlCommand(state_handler))
    server.run()


if __name__ == "__main__":
    main()
