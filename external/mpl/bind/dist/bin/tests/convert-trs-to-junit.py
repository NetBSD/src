#!/usr/bin/env python3
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# Convert automake .trs files into JUnit format suitable for Gitlab

import argparse
import os
import sys
from xml.etree import ElementTree
from xml.etree.ElementTree import Element
from xml.etree.ElementTree import SubElement


# getting explicit encoding specification right for Python 2/3 would be messy,
# so let's hope for the best
def read_whole_text(filename):
    with open(filename) as inf:  # pylint: disable-msg=unspecified-encoding
        return inf.read().strip()


def read_trs_result(filename):
    result = None
    with open(filename, "r") as trs:  # pylint: disable-msg=unspecified-encoding
        for line in trs:
            items = line.split()
            if len(items) < 2:
                raise ValueError("unsupported line in trs file", filename, line)
            if items[0] != (":global-test-result:"):
                continue
            if result is not None:
                raise NotImplementedError("double :global-test-result:", filename)
            result = items[1].upper()

    if result is None:
        raise ValueError(":global-test-result: not found", filename)

    return result


def find_test_relative_path(source_dir, in_path):
    """Return {in_path}.c if it exists, with fallback to {in_path}"""
    candidates_relative = [in_path + ".c", in_path]
    for relative in candidates_relative:
        absolute = os.path.join(source_dir, relative)
        if os.path.exists(absolute):
            return relative
    raise KeyError


def err_out(exception):
    raise exception


def walk_trss(source_dir):
    for cur_dir, _dirs, files in os.walk(source_dir, onerror=err_out):
        for filename in files:
            if not filename.endswith(".trs"):
                continue

            filename_prefix = filename[: -len(".trs")]
            log_name = filename_prefix + ".log"
            full_trs_path = os.path.join(cur_dir, filename)
            full_log_path = os.path.join(cur_dir, log_name)
            sub_dir = os.path.relpath(cur_dir, source_dir)
            test_dir_path = os.path.join(sub_dir, filename_prefix)

            if sub_dir.startswith("bin/tests/system"):
                # Match the `pytest` style test names for system tests
                test_name = f"test_{filename_prefix}"
            else:
                test_name = test_dir_path

            t = {
                "name": test_name,
                "full_log_path": full_log_path,
                "rel_log_path": os.path.relpath(full_log_path, source_dir),
            }
            t["result"] = read_trs_result(full_trs_path)

            # try to find dir/file path for a clickable link
            try:
                t["rel_file_path"] = find_test_relative_path(source_dir, test_dir_path)
            except KeyError:
                pass  # no existing path found

            yield t


def append_testcase(testsuite, t):
    # attributes taken from
    # https://gitlab.com/gitlab-org/gitlab-foss/-/blob/master/lib/gitlab/ci/parsers/test/junit.rb
    attrs = {"name": t["name"]}
    if "rel_file_path" in t:
        attrs["file"] = t["rel_file_path"]

    testcase = SubElement(testsuite, "testcase", attrs)

    # Gitlab accepts only [[ATTACHMENT| links for system-out, not raw text
    s = SubElement(testcase, "system-out")
    s.text = "[[ATTACHMENT|" + t["rel_log_path"] + "]]"
    if t["result"].lower() == "pass":
        return

    # Gitlab shows output only for failed or skipped tests
    if t["result"].lower() == "skip":
        err = SubElement(testcase, "skipped")
    else:
        err = SubElement(testcase, "failure")
    err.text = read_whole_text(t["full_log_path"])


def gen_junit(results):
    testsuites = Element("testsuites")
    testsuite = SubElement(testsuites, "testsuite")
    for test in results:
        append_testcase(testsuite, test)
    return testsuites


def check_directory(path):
    try:
        os.listdir(path)
        return path
    except OSError as ex:
        msg = "Path {} cannot be listed as a directory: {}".format(path, ex)
        raise argparse.ArgumentTypeError(msg)


def main():
    parser = argparse.ArgumentParser(
        description="Recursively search for .trs + .log files and compile "
        "them into JUnit XML suitable for Gitlab. Paths in the "
        "XML are relative to the specified top directory."
    )
    parser.add_argument(
        "top_directory",
        type=check_directory,
        help="root directory where to start scanning for .trs files",
    )
    args = parser.parse_args()
    junit = gen_junit(walk_trss(args.top_directory))

    # encode results into file format, on Python 3 it produces bytes
    xml = ElementTree.tostring(junit, "utf-8")
    # use stdout as a binary file object, Python2/3 compatibility
    output = getattr(sys.stdout, "buffer", sys.stdout)
    output.write(xml)


if __name__ == "__main__":
    main()
