#!/usr/bin/env python3
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# Convert JUnit pytest output to automake .trs files

import argparse
import sys
from xml.etree import ElementTree


def junit_to_trs(junit_xml):
    root = ElementTree.fromstring(junit_xml)
    testcases = root.findall("./testsuite/testcase")

    if len(testcases) < 1:
        print(":test-result: ERROR convert-junit-to-trs.py")
        return 99

    has_fail = False
    has_error = False
    has_skipped = False
    for testcase in testcases:
        filename = f"{testcase.attrib['classname'].replace('.', '/')}.py"
        name = f"{filename}::{testcase.attrib['name']}"
        res = "PASS"
        for node in testcase:
            if node.tag == "failure":
                res = "FAIL"
                has_fail = True
            elif node.tag == "error":
                res = "ERROR"
                has_error = True
            elif node.tag == "skipped":
                if node.attrib.get("type") == "pytest.xfail":
                    res = "XFAIL"
                else:
                    res = "SKIP"
                    has_skipped = True
        print(f":test-result: {res} {name}")

    if has_error:
        return 99
    if has_fail:
        return 1
    if has_skipped:
        return 77
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Convert JUnit XML to Automake TRS and exit with "
        "the appropriate Automake-compatible exit code."
    )
    parser.add_argument(
        "junit_file",
        type=argparse.FileType("r", encoding="utf-8"),
        help="junit xml result file",
    )
    args = parser.parse_args()

    junit_xml = args.junit_file.read()
    sys.exit(junit_to_trs(junit_xml))


if __name__ == "__main__":
    main()
