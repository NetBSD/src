#as: -mbig-obj
#objdump: -h
#name: PE x64 big obj

.*: *file format pe-bigobj-.*

Sections:
#...
5000. \.data\$a49999  .*
                  CONTENTS, ALLOC, LOAD, DATA

