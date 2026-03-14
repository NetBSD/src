# Copyright 2022-2025 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gdb

from .server import request
from .startup import in_gdb_thread


@in_gdb_thread
def _thread_name(thr):
    if thr.name is not None:
        return thr.name
    if thr.details is not None:
        return thr.details
    # Always return a name, as the protocol doesn't allow for nameless
    # threads.  Use the local thread number here... it doesn't matter
    # without multi-inferior but in that case it might make more
    # sense.
    return f"Thread #{thr.num}"


@request("threads", expect_stopped=False)
def threads(**args):
    result = []
    for thr in gdb.selected_inferior().threads():
        result.append(
            {
                "id": thr.global_num,
                "name": _thread_name(thr),
            }
        )
    return {
        "threads": result,
    }
