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

"""Custom authoritative server (ans4) for the dnssec_py suite.

Per-domain response handlers live one-per-module in sibling *_ans.py files
(e.g. rrsig_labels_signer_ans.py); this loader installs each into a single
AsyncDnsServer.  Keeping each domain's crafted-response logic in its own
file bounds its scope as the server accrues unrelated domains.
"""

from dnssec_py.ans4 import noqname_mismatch, rrsig_labels_signer_ans, sibling_ds_ans
from isctest.asyncserver import AsyncDnsServer


def main() -> None:
    server = AsyncDnsServer()

    server.install_response_handler(sibling_ds_ans.SiblingDsInjectionHandler())
    if noqname_mismatch.PEM_PATH.exists():
        server.install_response_handlers(noqname_mismatch.RuntimeCheckHandler())
    if rrsig_labels_signer_ans.PEM_PATH.exists():
        server.install_response_handler(rrsig_labels_signer_ans.AttackerZoneHandler())

    server.run()


if __name__ == "__main__":
    main()
