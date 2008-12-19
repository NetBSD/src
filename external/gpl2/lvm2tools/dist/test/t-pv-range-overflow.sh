#!/bin/sh
# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# 'Ensure that pvmove diagnoses PE-range values 2^32 and larger.'

. ./test-utils.sh

aux prepare_vg 2

lvcreate -L4 -n"$lv" $vg

# Test for the bogus diagnostic reported in BZ 284771
# http://bugzilla.redhat.com/284771.
# 'run pvmove with an unrecognized LV name to show bad diagnostic'
not pvmove -v -nbogus $dev1 $dev2 2> err
grep "  Logical volume bogus not found." err

# With lvm-2.02.28 and earlier, on a system with 64-bit "long int",
# the PE range parsing code would accept values up to 2^64-1, but would
# silently truncate them to int32_t.  I.e., $dev1:$(echo 2^32|bc) would be
# treated just like $dev1:0.
# 'run the offending pvmove command'
not pvmove -v -n$lv $dev1:4294967296 $dev2

#!/bin/sh
# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# 'Ensure that pvmove diagnoses PE-range values 2^32 and larger.'

. ./test-utils.sh

aux prepare_vg 2

lvcreate -L4 -n"$lv" $vg

# Test for the bogus diagnostic reported in BZ 284771
# http://bugzilla.redhat.com/284771.
# 'run pvmove with an unrecognized LV name to show bad diagnostic'
not pvmove -v -nbogus $dev1 $dev2 2> err
grep "  Logical volume bogus not found." err

# With lvm-2.02.28 and earlier, on a system with 64-bit "long int",
# the PE range parsing code would accept values up to 2^64-1, but would
# silently truncate them to int32_t.  I.e., $dev1:$(echo 2^32|bc) would be
# treated just like $dev1:0.
# 'run the offending pvmove command'
not pvmove -v -n$lv $dev1:4294967296 $dev2

