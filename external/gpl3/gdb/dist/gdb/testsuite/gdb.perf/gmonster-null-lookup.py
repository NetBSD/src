# Copyright (C) 2015-2023 Free Software Foundation, Inc.

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

# Test handling of lookup of a symbol that doesn't exist.
# Efficient handling of this case is important, and not just for typos.
# Sometimes the debug info for the needed object isn't present.

from perftest import perftest
from perftest import measure
from perftest import utils


class NullLookup(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, name, run_names, binfile):
        # We want to measure time in this test.
        super(NullLookup, self).__init__(name)
        self.run_names = run_names
        self.binfile = binfile

    def warm_up(self):
        pass

    def execute_test(self):
        for run in self.run_names:
            this_run_binfile = "%s-%s" % (self.binfile, utils.convert_spaces(run))
            utils.select_file(this_run_binfile)
            utils.runto_main()
            utils.safe_execute("mt expand-symtabs")
            iteration = 5
            while iteration > 0:
                utils.safe_execute("mt flush symbol-cache")
                func = lambda: utils.safe_execute("p symbol_not_found")
                self.measure.measure(func, run)
                iteration -= 1
