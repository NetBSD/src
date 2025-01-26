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

from pathlib import Path
from typing import Dict


def load_ac_vars_from_files() -> Dict[str, str]:
    ac_vars = {}
    ac_vars_dir = Path(__file__).resolve().parent / ".ac_vars"
    var_paths = [
        path
        for path in ac_vars_dir.iterdir()
        if path.is_file() and not path.name.endswith(".in")
    ]
    for var_path in var_paths:
        ac_vars[var_path.name] = var_path.read_text(encoding="utf-8").strip()
    return ac_vars


AC_VARS = load_ac_vars_from_files()
