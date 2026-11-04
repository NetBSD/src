############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

"""
Sphinx domain "namedconf". See iscconf.py for details.

"""

from docutils import nodes

import iscconf
import mergegrammar


class ToBeReplacedStatementList(nodes.General, nodes.Element):
    """
    Placeholder, does nothing, but must be picklable
    (= cannot be in generated class).
    """


def setup(app):
    grammar = mergegrammar.combine()
    return iscconf.setup(
        app, "namedconf", "named.conf", ToBeReplacedStatementList, grammar
    )
