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

# pylint: disable=unknown-option-value,re-compile-alias

import re

from astroid import nodes
from pylint.checkers import BaseRawFileChecker
from pylint.lint import PyLinter


class ReCompileChecker(BaseRawFileChecker):

    name = "custom_raw"
    msgs = {
        "R9901": (
            "Replace re.compile() with Re() using `from re import compile as Re`",
            "re-compile-alias",
            (
                "Use a Re() alias instead of re.compile() by importing the "
                "re.compile() function as Re()"
            ),
        ),
    }
    options = ()

    def process_module(self, node: nodes.Module) -> None:
        pattern = re.compile(r"re\.compile\(")
        import_pattern = re.compile(r"^\s*(import|from)\s+isctest\b")
        with node.stream() as stream:
            lines = [line.decode("utf-8", errors="replace") for line in stream]

        if not any(
            import_pattern.search(line) and not line.lstrip().startswith("#")
            for line in lines
        ):
            return

        for lineno, line in enumerate(lines):
            if pattern.search(line):
                self.add_message("re-compile-alias", line=lineno)


def register(linter: PyLinter) -> None:
    linter.register_checker(ReCompileChecker(linter))
