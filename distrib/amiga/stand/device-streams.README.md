# device-streams
Amiga tool for copying between block devices and streams/files, similar to UNIX `dd`.
Originally released by Christian E. Hopps in 1993, updated to be useful today.

Requires AmigaOS 2.04+.

https://github.com/rvalles/device-streams

## Usage
Run the command with `-h` parameter to display usage information.

### Examples
#### Print RDB device and partition information.
```
5.Ram Disk:> devstreams/rdbinfo
Device: "scsi.device"  Unit: 0  Capacity: 114473.4 Megs
DiskVendor: FUJITSU  DiskProduct MHV2120AH        DiskRevision: 0000
Cylinders: 232581  Heads: 16  Blks-p-Trk: 63 [Blks-p-Cyl: 1008]
Total Blocks: 234441648  Block Size 512
64bit

--| Partition: "DH0" Capacity: 1023.7 Megs
--| Start Block: 3024  End Block: 2099663 Total Blocks: 2096640
--| Block Size: 512

--| Partition: "DH1" Capacity: 10239.4 Megs
--| Start Block: 2099664  End Block: 23070095 Total Blocks: 20970432
--| Block Size: 512
--| 64bit
###
```
#### Dump a partition into a file.
```
5.Ram Disk:> devstreams/devtostream --output=DH1:dump.dd --rdb-name=DH0 --verbose
found new device "scsi.device"
found drive FUJITSU  MHV2120AH        0000 [capacity:114473M]
 at unit 0 on device "scsi.device"
| partition: "DH0" sb: 3024 eb: 2099663 totb: 2096640
|            Block Size: 512 Capacity: 1023.7
| partition: "DH1" sb: 2099664 eb: 23070095 totb: 20970432
|            Block Size: 512 Capacity: 10239.4
found partition: "DH0" capacity: 1023.7 Megs
start block: 3024  end block: 2099663 total blocks: 2096640
block Size: 512
dumping: start block: 3024 to end block: 2099663 [size: 1048319K]

write from partition "DH0" to file "DH1:dump.dd"? [Ny]:y
writing: 0x00200950 -> 0x002009cf  [100%]
```
## Improvements
* Cleaned up code.
* Removed SAS/C-isms and updated to build in modern toolchains.
* Use of 64bit offsets throughout.
* Safeguards added against 32bit overflow wraparound.
* Use dos.library for file I/O.
* Trackdisk 64 support.
* NSD support.

## Binaries built
* rdbinfo: Examines RDB partition tables and lists the partitions.
* streamtodev: Writes data from a stream into a device.
* devtostream: Reads data from a device into a stream.
* xstreamtodev: As streamtodev but with extra options to specify block range.
* xdevtostream: As devtostream but with extra options to specify block range.

## Building
The new build process uses GNU Make. Simply review the Makefile and run `make`.

Alternatively,
* `make clean` will delete all artifacts.
* `make lint` will format the code to standards.
* `make dist` will prepare a lha archive with the binaries.

Development is done using the `bebbo/amiga-gcc` crossdev toolchain.
https://github.com/bebbo/amiga-gcc

With some care so that building with `vbcc` is also supported.
http://sun.hasenbraten.de/vbcc/

## Authors
* Roc Vallès Domènech (2022)
* Christian E. Hopps (1993)
