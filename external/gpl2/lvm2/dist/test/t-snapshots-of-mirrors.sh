. ./test-utils.sh

prepare_vg 4

# Create snapshot of a mirror origin
lvcreate -m 1 -L 10M -n lv $vg
lvcreate -s $vg/lv -L 10M -n snap

# Down-convert (mirror -> linear) under a snapshot
lvconvert -m0 $vg/lv

# Up-convert (linear -> mirror)
lvconvert -m2 $vg/lv

# Down-convert (mirror -> mirror)
lvconvert -m1 $vg/lv

# Up-convert (mirror -> mirror) -- Not supported!
not lvconvert -m2 $vg/lv

# Log conversion (disk -> core)
lvconvert --mirrorlog core $vg/lv

# Log conversion (core -> redundant) -- Not available yet!
not lvconvert --mirrorlog redundant $vg/lv

# Log conversion (redundant -> core) -- Not available yet!
# Note: Uncomment this command when the above works
# not lvconvert --mirrorlog core $vg/lv

# Log conversion (core -> disk)
lvconvert --mirrorlog disk $vg/lv

# Clean-up
lvremove -ff $vg
