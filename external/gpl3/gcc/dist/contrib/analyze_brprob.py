#!/usr/bin/env python3
#
# Script to analyze results of our branch prediction heuristics
#
# This file is part of GCC.
#
# GCC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.
#
# GCC is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.  */
#
#
#
# This script is used to calculate two basic properties of the branch prediction
# heuristics - coverage and hitrate.  Coverage is number of executions
# of a given branch matched by the heuristics and hitrate is probability
# that once branch is predicted as taken it is really taken.
#
# These values are useful to determine the quality of given heuristics.
# Hitrate may be directly used in predict.def.
#
# Usage:
#  Step 1: Compile and profile your program.  You need to use -fprofile-generate
#    flag to get the profiles.
#  Step 2: Make a reference run of the intrumented application.
#  Step 3: Compile the program with collected profile and dump IPA profiles
#          (-fprofile-use -fdump-ipa-profile-details)
#  Step 4: Collect all generated dump files:
#          find . -name '*.profile' | xargs cat > dump_file
#  Step 5: Run the script:
#          ./analyze_brprob.py dump_file
#          and read results.  Basically the following table is printed:
#
# HEURISTICS                           BRANCHES  (REL)  HITRATE                COVERAGE  (REL)
# early return (on trees)                     3   0.2%  35.83% /  93.64%          66360   0.0%
# guess loop iv compare                       8   0.6%  53.35% /  53.73%       11183344   0.0%
# call                                       18   1.4%  31.95% /  69.95%       51880179   0.2%
# loop guard                                 23   1.8%  84.13% /  84.85%    13749065956  42.2%
# opcode values positive (on trees)          42   3.3%  15.71% /  84.81%     6771097902  20.8%
# opcode values nonequal (on trees)         226  17.6%  72.48% /  72.84%      844753864   2.6%
# loop exit                                 231  18.0%  86.97% /  86.98%     8952666897  27.5%
# loop iterations                           239  18.6%  91.10% /  91.10%     3062707264   9.4%
# DS theory                                 281  21.9%  82.08% /  83.39%     7787264075  23.9%
# no prediction                             293  22.9%  46.92% /  70.70%     2293267840   7.0%
# guessed loop iterations                   313  24.4%  76.41% /  76.41%    10782750177  33.1%
# first match                               708  55.2%  82.30% /  82.31%    22489588691  69.0%
# combined                                 1282 100.0%  79.76% /  81.75%    32570120606 100.0%
#
#
#  The heuristics called "first match" is a heuristics used by GCC branch
#  prediction pass and it predicts 55.2% branches correctly. As you can,
#  the heuristics has very good covertage (69.05%).  On the other hand,
#  "opcode values nonequal (on trees)" heuristics has good hirate, but poor
#  coverage.

import sys
import os
import re
import argparse

from math import *

counter_aggregates = set(['combined', 'first match', 'DS theory',
    'no prediction'])

def percentage(a, b):
    return 100.0 * a / b

def average(values):
    return 1.0 * sum(values) / len(values)

def average_cutoff(values, cut):
    l = len(values)
    skip = floor(l * cut / 2)
    if skip > 0:
        values.sort()
        values = values[skip:-skip]
    return average(values)

def median(values):
    values.sort()
    return values[int(len(values) / 2)]

class Summary:
    def __init__(self, name):
        self.name = name
        self.branches = 0
        self.successfull_branches = 0
        self.count = 0
        self.hits = 0
        self.fits = 0

    def get_hitrate(self):
        return 100.0 * self.hits / self.count

    def get_branch_hitrate(self):
        return 100.0 * self.successfull_branches / self.branches

    def count_formatted(self):
        v = self.count
        for unit in ['','K','M','G','T','P','E','Z']:
            if v < 1000:
                return "%3.2f%s" % (v, unit)
            v /= 1000.0
        return "%.1f%s" % (v, 'Y')

    def print(self, branches_max, count_max):
        print('%-40s %8i %5.1f%% %11.2f%% %7.2f%% / %6.2f%% %14i %8s %5.1f%%' %
            (self.name, self.branches,
                percentage(self.branches, branches_max),
                self.get_branch_hitrate(),
                self.get_hitrate(),
                percentage(self.fits, self.count),
                self.count, self.count_formatted(),
                percentage(self.count, count_max)))

class Profile:
    def __init__(self, filename):
        self.filename = filename
        self.heuristics = {}
        self.niter_vector = []

    def add(self, name, prediction, count, hits):
        if not name in self.heuristics:
            self.heuristics[name] = Summary(name)

        s = self.heuristics[name]
        s.branches += 1

        s.count += count
        if prediction < 50:
            hits = count - hits
        remaining = count - hits
        if hits >= remaining:
            s.successfull_branches += 1

        s.hits += hits
        s.fits += max(hits, remaining)

    def add_loop_niter(self, niter):
        if niter > 0:
            self.niter_vector.append(niter)

    def branches_max(self):
        return max([v.branches for k, v in self.heuristics.items()])

    def count_max(self):
        return max([v.count for k, v in self.heuristics.items()])

    def print_group(self, sorting, group_name, heuristics):
        count_max = self.count_max()
        branches_max = self.branches_max()

        sorter = lambda x: x.branches
        if sorting == 'branch-hitrate':
            sorter = lambda x: x.get_branch_hitrate()
        elif sorting == 'hitrate':
            sorter = lambda x: x.get_hitrate()
        elif sorting == 'coverage':
            sorter = lambda x: x.count
        elif sorting == 'name':
            sorter = lambda x: x.name.lower()

        print('%-40s %8s %6s %12s %18s %14s %8s %6s' %
            ('HEURISTICS', 'BRANCHES', '(REL)',
            'BR. HITRATE', 'HITRATE', 'COVERAGE', 'COVERAGE', '(REL)'))
        for h in sorted(heuristics, key = sorter):
            h.print(branches_max, count_max)

    def dump(self, sorting):
        heuristics = self.heuristics.values()
        if len(heuristics) == 0:
            print('No heuristics available')
            return

        special = list(filter(lambda x: x.name in counter_aggregates,
            heuristics))
        normal = list(filter(lambda x: x.name not in counter_aggregates,
            heuristics))

        self.print_group(sorting, 'HEURISTICS', normal)
        print()
        self.print_group(sorting, 'HEURISTIC AGGREGATES', special)

        if len(self.niter_vector) > 0:
            print ('\nLoop count: %d' % len(self.niter_vector)),
            print('  avg. # of iter: %.2f' % average(self.niter_vector))
            print('  median # of iter: %.2f' % median(self.niter_vector))
            for v in [1, 5, 10, 20, 30]:
                cut = 0.01 * v
                print('  avg. (%d%% cutoff) # of iter: %.2f'
                    % (v, average_cutoff(self.niter_vector, cut)))

parser = argparse.ArgumentParser()
parser.add_argument('dump_file', metavar = 'dump_file',
    help = 'IPA profile dump file')
parser.add_argument('-s', '--sorting', dest = 'sorting',
    choices = ['branches', 'branch-hitrate', 'hitrate', 'coverage', 'name'],
    default = 'branches')

args = parser.parse_args()

profile = Profile(sys.argv[1])
r = re.compile('  (.*) heuristics( of edge [0-9]*->[0-9]*)?( \\(.*\\))?: (.*)%.*exec ([0-9]*) hit ([0-9]*)')
loop_niter_str = ';;  profile-based iteration count: '
for l in open(args.dump_file).readlines():
    m = r.match(l)
    if m != None and m.group(3) == None:
        name = m.group(1)
        prediction = float(m.group(4))
        count = int(m.group(5))
        hits = int(m.group(6))

        profile.add(name, prediction, count, hits)
    elif l.startswith(loop_niter_str):
        v = int(l[len(loop_niter_str):])
        profile.add_loop_niter(v)

profile.dump(args.sorting)
