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

import glob
import struct


class RawFormatHeader(dict):
    """
    A dictionary of raw-format header fields read from a zone file.
    """

    fields = [
        "format",
        "version",
        "dumptime",
        "flags",
        "sourceserial",
        "lastxfrin",
    ]

    def __init__(self, file_name):
        header = struct.Struct(">IIIIII")
        with open(file_name, "rb") as data:
            header_data = data.read(header.size)
        super().__init__(zip(self.fields, header.unpack_from(header_data)))


def test_unsigned_serial_number():
    """
    Check whether all signed zone files in the "ns8" subdirectory contain the
    serial number of the unsigned version of the zone in the raw-format header.
    The test assumes that all "*.signed" files in the "ns8" subdirectory are in
    raw format.

    Notes:

      - The actual zone signing and dumping happens while the tests.sh phase of
        the "inline" system test is set up and run.  This check only verifies
        the outcome of those events; it does not initiate any signing or
        dumping itself.

      - example[0-9][0-9].com.db.signed files are initially signed by
        dnssec-signzone while the others - by named.
    """

    zones_with_unsigned_serial_missing = []

    for signed_zone in sorted(glob.glob("ns8/*.signed")):
        raw_header = RawFormatHeader(signed_zone)
        # Ensure the unsigned serial number is placed where it is expected.
        assert raw_header["format"] == 2
        assert raw_header["version"] == 1
        # Check whether the header flags indicate that the unsigned serial
        # number is set and that the latter is indeed set.
        if raw_header["flags"] & 0x02 == 0 or raw_header["sourceserial"] == 0:
            zones_with_unsigned_serial_missing.append(signed_zone)

    assert not zones_with_unsigned_serial_missing
