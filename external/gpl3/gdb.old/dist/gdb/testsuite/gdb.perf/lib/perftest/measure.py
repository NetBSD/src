# Copyright (C) 2013-2014 Free Software Foundation, Inc.

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

import time
import os
import gc

class Measure(object):
    """A class that measure and collect the interesting data for a given testcase.

    An instance of Measure has a collection of measurements, and each
    of them is to measure a given aspect, such as time and memory.
    """

    def __init__(self, measurements):
        """Constructor of measure.

        measurements is a collection of Measurement objects.
        """

        self.measurements = measurements

    def measure(self, func, id):
        """Measure the operations done by func with a collection of measurements."""
        # Enable GC, force GC and disable GC before running test in order to reduce
        # the interference from GC.
        gc.enable()
        gc.collect()
        gc.disable()

        for m in self.measurements:
            m.start(id)

        func()

        for m in self.measurements:
            m.stop(id)

        gc.enable()

    def report(self, reporter, name):
        """Report the measured results."""
        for m in self.measurements:
            m.report(reporter, name)

class Measurement(object):
    """A measurement for a certain aspect."""

    def __init__(self, name, result):
        """Constructor of Measurement.

        Attribute result is the TestResult associated with measurement.
        """
        self.name = name;
        self.result = result

    def start(self, id):
        """Abstract method to start the measurement."""
        raise NotImplementedError("Abstract Method:start")

    def stop(self, id):
        """Abstract method to stop the measurement.

        When the measurement is stopped, we've got something, and
        record them in result.
        """
        raise NotImplementedError("Abstract Method:stop.")

    def report(self, reporter, name):
        """Report the measured data by argument reporter."""
        self.result.report(reporter, name + " " + self.name)

class MeasurementCpuTime(Measurement):
    """Measurement on CPU time."""
    # On UNIX, time.clock() measures the amount of CPU time that has
    # been used by the current process.  On Windows it will measure
    # wall-clock seconds elapsed since the first call to the function.
    # Something other than time.clock() should be used to measure CPU
    # time on Windows.

    def __init__(self, result):
        super(MeasurementCpuTime, self).__init__("cpu_time", result)
        self.start_time = 0

    def start(self, id):
        self.start_time = time.clock()

    def stop(self, id):
        if os.name == 'nt':
            cpu_time = 0
        else:
            cpu_time = time.clock() - self.start_time
        self.result.record (id, cpu_time)

class MeasurementWallTime(Measurement):
    """Measurement on Wall time."""

    def __init__(self, result):
        super(MeasurementWallTime, self).__init__("wall_time", result)
        self.start_time = 0

    def start(self, id):
        self.start_time = time.time()

    def stop(self, id):
        wall_time = time.time() - self.start_time
        self.result.record (id, wall_time)

class MeasurementVmSize(Measurement):
    """Measurement on memory usage represented by VmSize."""

    def __init__(self, result):
        super(MeasurementVmSize, self).__init__("vmsize", result)

    def _compute_process_memory_usage(self, key):
        file_path = "/proc/%d/status" % os.getpid()
        try:
            t = open(file_path)
            v = t.read()
            t.close()
        except:
            return 0
        i = v.index(key)
        v = v[i:].split(None, 3)
        if len(v) < 3:
            return 0
        return int(v[1])

    def start(self, id):
        pass

    def stop(self, id):
        memory_used = self._compute_process_memory_usage("VmSize:")
        self.result.record (id, memory_used)
