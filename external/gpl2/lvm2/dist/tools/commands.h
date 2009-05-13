/*	$NetBSD: commands.h,v 1.1.1.1.2.1 2009/05/13 18:52:47 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/***********  Replace with script?
xx(e2fsadm,
   "Resize logical volume and ext2 filesystem",
   "e2fsadm "
   "[-d|--debug] " "[-h|--help] " "[-n|--nofsck]" "\n"
   "\t{[-l|--extents] [+|-]LogicalExtentsNumber |" "\n"
   "\t [-L|--size] [+|-]LogicalVolumeSize[kKmMgGtTpPeE]}" "\n"
   "\t[-t|--test] "  "\n"
   "\t[-v|--verbose] "  "\n"
   "\t[--version] " "\n"
   "\tLogicalVolumePath" "\n",

    extents_ARG, size_ARG, nofsck_ARG, test_ARG)
*********/

xx(dumpconfig,
   "Dump active configuration",
   0,
   "dumpconfig "
   "\t[-f|--file filename] " "\n"
   "[ConfigurationVariable...]\n",
   file_ARG)

xx(formats,
   "List available metadata formats",
   0,
   "formats\n")

xx(help,
   "Display help for commands",
   0,
   "help <command>" "\n")

/*********
xx(lvactivate,
   "Activate logical volume on given partition(s)",
   "lvactivate "
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-v|--verbose]\n"
   "Logical Volume(s)\n")
***********/

xx(lvchange,
   "Change the attributes of logical volume(s)",
   CACHE_VGMETADATA,
   "lvchange\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-a|--available [e|l]y|n]\n"
   "\t[--addtag Tag]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-C|--contiguous y|n]\n"
   "\t[-d|--debug]\n"
   "\t[--deltag Tag]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--ignoremonitoring]\n"
   "\t[--monitor {y|n}]\n"
   "\t[-M|--persistent y|n] [--major major] [--minor minor]\n"
   "\t[-P|--partial] " "\n"
   "\t[-p|--permission r|rw]\n"
   "\t[-r|--readahead ReadAheadSectors|auto|none]\n"
   "\t[--refresh]\n"
   "\t[--resync]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[-y|--yes]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path] [LogicalVolume[Path]...]\n",

   alloc_ARG, autobackup_ARG, available_ARG, contiguous_ARG, force_ARG,
   ignorelockingfailure_ARG, ignoremonitoring_ARG, major_ARG, minor_ARG,
   monitor_ARG, partial_ARG, permission_ARG, persistent_ARG, readahead_ARG,
   resync_ARG, refresh_ARG, addtag_ARG, deltag_ARG, test_ARG, yes_ARG)

xx(lvconvert,
   "Change logical volume layout",
   0,
   "lvconvert "
   "[-m|--mirrors Mirrors [{--mirrorlog {disk|core}|--corelog}]]\n"
   "\t[-R|--regionsize MirrorLogRegionSize]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-b|--background]\n"
   "\t[-d|--debug]\n"
   "\t[-h|-?|--help]\n"
   "\t[-i|--interval seconds]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path] [PhysicalVolume[Path]...]\n\n"

   "lvconvert "
   "[-s|--snapshot]\n"
   "\t[-c|--chunksize]\n"
   "\t[-d|--debug]\n"
   "\t[-h|-?|--help]\n"
   "\t[-v|--verbose]\n"
   "\t[-Z|--zero {y|n}]\n"
   "\t[--version]" "\n"
   "\tOriginalLogicalVolume[Path] SnapshotLogicalVolume[Path]\n",

   alloc_ARG, background_ARG, chunksize_ARG, corelog_ARG, interval_ARG,
   mirrorlog_ARG, mirrors_ARG, regionsize_ARG, snapshot_ARG, test_ARG, zero_ARG)

xx(lvcreate,
   "Create a logical volume",
   0,
   "lvcreate " "\n"
   "\t[-A|--autobackup {y|n}]\n"
   "\t[--addtag Tag]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-C|--contiguous {y|n}]\n"
   "\t[-d|--debug]\n"
   "\t[-h|-?|--help]\n"
   "\t[-i|--stripes Stripes [-I|--stripesize StripeSize]]\n"
   "\t{-l|--extents LogicalExtentsNumber |\n"
   "\t -L|--size LogicalVolumeSize[kKmMgGtTpPeE]}\n"
   "\t[-M|--persistent {y|n}] [--major major] [--minor minor]\n"
   "\t[-m|--mirrors Mirrors [--nosync] [{--mirrorlog {disk|core}|--corelog}]]\n"
   "\t[-n|--name LogicalVolumeName]\n"
   "\t[-p|--permission {r|rw}]\n"
   "\t[-r|--readahead ReadAheadSectors|auto|none]\n"
   "\t[-R|--regionsize MirrorLogRegionSize]\n"
   "\t[-t|--test]\n"
   "\t[--type VolumeType]\n"
   "\t[-v|--verbose]\n"
   "\t[-Z|--zero {y|n}]\n"
   "\t[--version]\n"
   "\tVolumeGroupName [PhysicalVolumePath...]\n\n"

   "lvcreate -s|--snapshot\n"
   "\t[-c|--chunksize]\n"
   "\t[-A|--autobackup {y|n}]\n"
   "\t[--addtag Tag]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-C|--contiguous {y|n}]\n"
   "\t[-d|--debug]\n"
   "\t[-h|-?|--help]\n"
   "\t[-i|--stripes Stripes [-I|--stripesize StripeSize]]\n"
   "\t{-l|--extents LogicalExtentsNumber[%{VG|LV|PVS|FREE}] |\n"
   "\t -L|--size LogicalVolumeSize[kKmMgGtTpPeE]}\n"
   "\t[-M|--persistent {y|n}] [--major major] [--minor minor]\n"
   "\t[-n|--name LogicalVolumeName]\n"
   "\t[-p|--permission {r|rw}]\n"
   "\t[-r|--readahead ReadAheadSectors|auto|none]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]\n"
   "\tOriginalLogicalVolume[Path] [PhysicalVolumePath...]\n\n",

   addtag_ARG, alloc_ARG, autobackup_ARG, chunksize_ARG, contiguous_ARG,
   corelog_ARG, extents_ARG, major_ARG, minor_ARG, mirrorlog_ARG, mirrors_ARG,
   name_ARG, nosync_ARG, permission_ARG, persistent_ARG, readahead_ARG,
   regionsize_ARG, size_ARG, snapshot_ARG, stripes_ARG, stripesize_ARG,
   test_ARG, type_ARG, zero_ARG)

xx(lvdisplay,
   "Display information about a logical volume",
   0,
   "lvdisplay\n"
   "\t[-a|--all]\n"
   "\t[-c|--colon]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[-m|--maps]\n"
   "\t[--nosuffix]\n"
   "\t[-P|--partial] " "\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[LogicalVolume[Path] [LogicalVolume[Path]...]]\n"
   "\n"
   "lvdisplay --columns|-C\n"
   "\t[--aligned]\n"
   "\t[-a|--all]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[-P|--partial] " "\n"
   "\t[--segments]\n"
   "\t[--separator Separator]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[LogicalVolume[Path] [LogicalVolume[Path]...]]\n",

    aligned_ARG, all_ARG, colon_ARG, columns_ARG, disk_ARG,
    ignorelockingfailure_ARG, maps_ARG, noheadings_ARG, nosuffix_ARG,
    options_ARG, sort_ARG, partial_ARG, segments_ARG, separator_ARG,
    unbuffered_ARG, units_ARG)

xx(lvextend,
   "Add space to a logical volume",
   0,
   "lvextend\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-d|--debug]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t[-i|--stripes Stripes [-I|--stripesize StripeSize]]\n"
   "\t{-l|--extents [+]LogicalExtentsNumber[%{VG|PVS|FREE}] |\n"
   "\t -L|--size [+]LogicalVolumeSize[kKmMgGtTpPeE]}\n"
   "\t[-m|--mirrors Mirrors]\n"
   "\t[-n|--nofsck]\n"
   "\t[-r|--resizefs]\n"
   "\t[-t|--test]\n"
   "\t[--type VolumeType]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path] [ PhysicalVolumePath... ]\n",

   alloc_ARG, autobackup_ARG, extents_ARG, force_ARG, mirrors_ARG,
   nofsck_ARG, resizefs_ARG, size_ARG, stripes_ARG, stripesize_ARG,
   test_ARG, type_ARG)

xx(lvmchange,
   "With the device mapper, this is obsolete and does nothing.",
   0,
   "lvmchange\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-R|--reset]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n",

    reset_ARG)

xx(lvmdiskscan,
   "List devices that may be used as physical volumes",
   0,
   "lvmdiskscan\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-l|--lvmpartition]\n"
   "\t[--version]" "\n",

   lvmpartition_ARG)

xx(lvmsadc,
   "Collect activity data",
   0,
   "lvmsadc\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[LogFilePath]\n" )

xx(lvmsar,
   "Create activity report",
   0,
   "lvmsar\n"
   "\t[-d|--debug]\n"
   "\t[-f|--full]\n"
   "\t[-h|--help]\n"
   "\t[-s|--stdin]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tLogFilePath\n",

   full_ARG, stdin_ARG)

xx(lvreduce,
   "Reduce the size of a logical volume",
   0,
   "lvreduce\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t{-l|--extents [-]LogicalExtentsNumber[%{VG|LV|FREE}] |\n"
   "\t -L|--size [-]LogicalVolumeSize[kKmMgGtTpPeE]}\n"
   "\t[-n|--nofsck]\n"
   "\t[-r|--resizefs]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[-y|--yes]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path]\n",

   autobackup_ARG, force_ARG,  extents_ARG, nofsck_ARG, resizefs_ARG,
   size_ARG, test_ARG, yes_ARG)

xx(lvremove,
   "Remove logical volume(s) from the system",
   0,
   "lvremove\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path] [LogicalVolume[Path]...]\n",

   autobackup_ARG, force_ARG, test_ARG)

xx(lvrename,
   "Rename a logical volume",
   0,
   "lvrename "
   "\t[-A|--autobackup {y|n}] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose]" "\n"
   "\t[--version] " "\n"
   "\t{ OldLogicalVolumePath NewLogicalVolumePath |" "\n"
   "\t  VolumeGroupName OldLogicalVolumeName NewLogicalVolumeName }\n",

   autobackup_ARG, test_ARG)

xx(lvresize,
   "Resize a logical volume",
   0,
   "lvresize\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-d|--debug]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t[-i|--stripes Stripes [-I|--stripesize StripeSize]]\n"
   "\t{-l|--extents [+|-]LogicalExtentsNumber[%{VG|LV|PVS|FREE}] |\n"
   "\t -L|--size [+|-]LogicalVolumeSize[kKmMgGtTpPeE]}\n"
   "\t[-n|--nofsck]\n"
   "\t[-r|--resizefs]\n"
   "\t[-t|--test]\n"
   "\t[--type VolumeType]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tLogicalVolume[Path] [ PhysicalVolumePath... ]\n",

   alloc_ARG, autobackup_ARG, extents_ARG, force_ARG, nofsck_ARG,
   resizefs_ARG, size_ARG, stripes_ARG, stripesize_ARG, test_ARG,
   type_ARG)

xx(lvs,
   "Display information about logical volumes",
   0,
   "lvs" "\n"
   "\t[-a|--all]\n"
   "\t[--aligned]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--nameprefixes]\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[-P|--partial] " "\n"
   "\t[--rows]\n"
   "\t[--segments]\n"
   "\t[--separator Separator]\n"
   "\t[--trustcache]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[--unquoted]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[LogicalVolume[Path] [LogicalVolume[Path]...]]\n",

   aligned_ARG, all_ARG, ignorelockingfailure_ARG, nameprefixes_ARG,
   noheadings_ARG, nolocking_ARG, nosuffix_ARG, options_ARG, partial_ARG, 
   rows_ARG, segments_ARG, separator_ARG, sort_ARG, trustcache_ARG,
   unbuffered_ARG, units_ARG, unquoted_ARG)

xx(lvscan,
   "List all logical volumes in all volume groups",
   0,
   "lvscan " "\n"
   "\t[-a|--all]\n"
   "\t[-b|--blockdevice] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[--ignorelockingfailure]\n"
   "\t[-P|--partial] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version]\n",

   all_ARG, blockdevice_ARG, disk_ARG, ignorelockingfailure_ARG, partial_ARG)

xx(pvchange,
   "Change attributes of physical volume(s)",
   0,
   "pvchange\n"
   "\t[-a|--all]\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-t|--test]\n"
   "\t[-u|--uuid]\n"
   "\t[-x|--allocatable y|n]\n"
   "\t[-v|--verbose]\n"
   "\t[--addtag Tag]\n"
   "\t[--deltag Tag]\n"
   "\t[--version]" "\n"
   "\t[PhysicalVolumePath...]\n",

   all_ARG, allocatable_ARG, allocation_ARG, autobackup_ARG, deltag_ARG,
   addtag_ARG, test_ARG, uuid_ARG)

xx(pvresize,
   "Resize physical volume(s)",
   0,
   "pvresize " "\n"
   "\t[-d|--debug]" "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[--setphysicalvolumesize PhysicalVolumeSize[kKmMgGtTpPeE]" "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version] " "\n"
   "\tPhysicalVolume [PhysicalVolume...]\n",

   physicalvolumesize_ARG, test_ARG)

xx(pvck,
   "Check the consistency of physical volume(s)",
   0,
   "pvck "
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--labelsector sector] " "\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tPhysicalVolume [PhysicalVolume...]\n",

   labelsector_ARG)

xx(pvcreate,
   "Initialize physical volume(s) for use by LVM",
   0,
   "pvcreate " "\n"
   "\t[--restorefile file]\n"
   "\t[-d|--debug]" "\n"
   "\t[-f[f]|--force [--force]] " "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[--labelsector sector] " "\n"
   "\t[-M|--metadatatype 1|2]" "\n"
   "\t[--metadatacopies #copies]" "\n"
   "\t[--metadatasize MetadataSize[kKmMgGtTpPeE]]" "\n"
   "\t[--setphysicalvolumesize PhysicalVolumeSize[kKmMgGtTpPeE]" "\n"
   "\t[-t|--test] " "\n"
   "\t[-u|--uuid uuid] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[-y|--yes]" "\n"
   "\t[-Z|--zero {y|n}]\n"
   "\t[--version] " "\n"
   "\tPhysicalVolume [PhysicalVolume...]\n",

   force_ARG, test_ARG, labelsector_ARG, metadatatype_ARG, metadatacopies_ARG,
   metadatasize_ARG, physicalvolumesize_ARG, restorefile_ARG, uuidstr_ARG,
   yes_ARG, zero_ARG)

xx(pvdata,
   "Display the on-disk metadata for physical volume(s)",
   0,
   "pvdata " "\n"
   "\t[-a|--all] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-E|--physicalextent] " "\n"
   "\t[-h|-?|--help]" "\n"
   "\t[-L|--logicalvolume] " "\n"
   "\t[-P[P]|--physicalvolume [--physicalvolume]]" "\n"
   "\t[-U|--uuidlist] " "\n"
   "\t[-v[v]|--verbose [--verbose]] " "\n"
   "\t[-V|--volumegroup]" "\n"
   "\t[--version] " "\n"
   "\tPhysicalVolume [PhysicalVolume...]\n",

   all_ARG,  logicalextent_ARG, physicalextent_ARG,
   physicalvolume_ARG, uuidlist_ARG, volumegroup_ARG)

xx(pvdisplay,
   "Display various attributes of physical volume(s)",
   0,
   "pvdisplay\n"
   "\t[-c|--colon]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[-m|--maps]\n"
   "\t[--nosuffix]\n"
   "\t[-s|--short]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[PhysicalVolumePath [PhysicalVolumePath...]]\n"
   "\n"
   "pvdisplay --columns|-C\n"
   "\t[--aligned]\n"
   "\t[-a|--all]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[--separator Separator]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[PhysicalVolumePath [PhysicalVolumePath...]]\n",

   aligned_ARG, all_ARG, colon_ARG, columns_ARG, ignorelockingfailure_ARG,
   maps_ARG, noheadings_ARG, nosuffix_ARG, options_ARG, separator_ARG,
   short_ARG, sort_ARG, unbuffered_ARG, units_ARG)

xx(pvmove,
   "Move extents from one physical volume to another",
   0,
   "pvmove " "\n"
   "\t[--abort]\n"
   "\t[-A|--autobackup {y|n}]\n"
   "\t[--alloc AllocationPolicy]\n"
   "\t[-b|--background]\n"
   "\t[-d|--debug]\n "
   "\t[-h|-?|--help]\n"
   "\t[-i|--interval seconds]\n"
   "\t[-t|--test]\n "
   "\t[-v|--verbose]\n "
   "\t[--version]\n"
   "\t[{-n|--name} LogicalVolume]\n"
/* "\t[{-n|--name} LogicalVolume[:LogicalExtent[-LogicalExtent]...]]\n" */
   "\tSourcePhysicalVolume[:PhysicalExtent[-PhysicalExtent]...]}\n"
   "\t[DestinationPhysicalVolume[:PhysicalExtent[-PhysicalExtent]...]...]\n",

   abort_ARG, alloc_ARG, autobackup_ARG, background_ARG,
   interval_ARG, name_ARG, test_ARG)

xx(pvremove,
   "Remove LVM label(s) from physical volume(s)",
   0,
   "pvremove " "\n"
   "\t[-d|--debug]" "\n"
   "\t[-f[f]|--force [--force]] " "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[-y|--yes]" "\n"
   "\t[--version] " "\n"
   "\tPhysicalVolume [PhysicalVolume...]\n",

   force_ARG, test_ARG, yes_ARG)

xx(pvs,
   "Display information about physical volumes",
   0,
   "pvs" "\n"
   "\t[--aligned]\n"
   "\t[-a|--all]\n"
   "\t[-d|--debug]" "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--nameprefixes]\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[-P|--partial] " "\n"
   "\t[--rows]\n"
   "\t[--segments]\n"
   "\t[--separator Separator]\n"
   "\t[--trustcache]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[--unquoted]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]\n"
   "\t[PhysicalVolume [PhysicalVolume...]]\n",

   aligned_ARG, all_ARG, ignorelockingfailure_ARG, nameprefixes_ARG,
   noheadings_ARG, nolocking_ARG, nosuffix_ARG, options_ARG, partial_ARG,
   rows_ARG, segments_ARG, separator_ARG, sort_ARG, trustcache_ARG,
   unbuffered_ARG, units_ARG, unquoted_ARG)

xx(pvscan,
   "List all physical volumes",
   0,
   "pvscan " "\n"
   "\t[-d|--debug] " "\n"
   "\t{-e|--exported | -n|--novolumegroup} " "\n"
   "\t[-h|-?|--help]" "\n"
   "\t[--ignorelockingfailure]\n"
   "\t[-P|--partial] " "\n"
   "\t[-s|--short] " "\n"
   "\t[-u|--uuid] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version]\n",

   exported_ARG, ignorelockingfailure_ARG, novolumegroup_ARG, partial_ARG,
   short_ARG, uuid_ARG)

xx(segtypes,
   "List available segment types",
   0,
   "segtypes\n")

xx(vgcfgbackup,
   "Backup volume group configuration(s)",
   0,
   "vgcfgbackup " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-f|--file filename] " "\n"
   "\t[-h|-?|--help] " "\n"
   "\t[--ignorelockingfailure]\n"
   "\t[-P|--partial] " "\n"
   "\t[-v|--verbose]" "\n"
   "\t[--version] " "\n"
   "\t[VolumeGroupName...]\n",

   file_ARG, ignorelockingfailure_ARG, partial_ARG)

xx(vgcfgrestore,
   "Restore volume group configuration",
   0,
   "vgcfgrestore " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-f|--file filename] " "\n"
   "\t[-l[l]|--list [--list]]" "\n"
   "\t[-M|--metadatatype 1|2]" "\n"
   "\t[-n|--name VolumeGroupName] " "\n"
   "\t[-h|--help]" "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose]" "\n"
   "\t[--version] " "\n"
   "\tVolumeGroupName",

   file_ARG, list_ARG, metadatatype_ARG, name_ARG, test_ARG)

xx(vgchange,
   "Change volume group attributes",
   CACHE_VGMETADATA,
   "vgchange" "\n"
   "\t[-A|--autobackup {y|n}] " "\n"
   "\t[--alloc AllocationPolicy] " "\n"
   "\t[-P|--partial] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|--help] " "\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--ignoremonitoring]\n"
   "\t[--monitor {y|n}]\n"
   "\t[--refresh]\n"
   "\t[-t|--test]" "\n"
   "\t[-u|--uuid] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version]" "\n"
   "\t{-a|--available [e|l]{y|n}  |" "\n"
   "\t -c|--clustered {y|n} |" "\n"
   "\t -x|--resizeable {y|n} |" "\n"
   "\t -l|--logicalvolume MaxLogicalVolumes |" "\n"
   "\t -p|--maxphysicalvolumes MaxPhysicalVolumes |" "\n"
   "\t -s|--physicalextentsize PhysicalExtentSize[kKmMgGtTpPeE] |" "\n"
   "\t --addtag Tag |\n"
   "\t --deltag Tag}\n"
   "\t[VolumeGroupName...]\n",

   addtag_ARG, alloc_ARG, allocation_ARG, autobackup_ARG, available_ARG,
   clustered_ARG, deltag_ARG, ignorelockingfailure_ARG, ignoremonitoring_ARG,
   logicalvolume_ARG, maxphysicalvolumes_ARG, monitor_ARG, partial_ARG,
   physicalextentsize_ARG, refresh_ARG, resizeable_ARG, resizable_ARG,
   test_ARG, uuid_ARG)

xx(vgck,
   "Check the consistency of volume group(s)",
   0,
   "vgck "
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[VolumeGroupName...]\n" )

xx(vgconvert,
   "Change volume group metadata format",
   0,
   "vgconvert  " "\n"
   "\t[-d|--debug]" "\n"
   "\t[-h|--help] " "\n"
   "\t[--labelsector sector] " "\n"
   "\t[-M|--metadatatype 1|2]" "\n"
   "\t[--metadatacopies #copies]" "\n"
   "\t[--metadatasize MetadataSize[kKmMgGtTpPeE]]" "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version] " "\n"
   "\tVolumeGroupName [VolumeGroupName...]\n",

   force_ARG, test_ARG, labelsector_ARG, metadatatype_ARG, metadatacopies_ARG,
   metadatasize_ARG )

xx(vgcreate,
   "Create a volume group",
   0,
   "vgcreate" "\n"
   "\t[-A|--autobackup {y|n}] " "\n"
   "\t[--addtag Tag] " "\n"
   "\t[--alloc AllocationPolicy] " "\n"
   "\t[-c|--clustered {y|n}] " "\n"
   "\t[-d|--debug]" "\n"
   "\t[-h|--help]" "\n"
   "\t[-l|--maxlogicalvolumes MaxLogicalVolumes]" "\n"
   "\t[-M|--metadatatype 1|2] " "\n"
   "\t[-p|--maxphysicalvolumes MaxPhysicalVolumes] " "\n"
   "\t[-s|--physicalextentsize PhysicalExtentSize[kKmMgGtTpPeE]] " "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose]" "\n"
   "\t[--version] " "\n"
   "\tVolumeGroupName PhysicalVolume [PhysicalVolume...]\n",

   addtag_ARG, alloc_ARG, autobackup_ARG, clustered_ARG, maxlogicalvolumes_ARG,
   maxphysicalvolumes_ARG, metadatatype_ARG, physicalextentsize_ARG, test_ARG)

xx(vgdisplay,
   "Display volume group information",
   0,
   "vgdisplay " "\n"
   "\t[-c|--colon | -s|--short | -v|--verbose]" "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|--help] " "\n"
   "\t[--ignorelockingfailure]" "\n"
   "\t[--nosuffix]\n"
   "\t[-P|--partial] " "\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[-A|--activevolumegroups | [-D|--disk]" "\n"
   "\t[--version]" "\n"
   "\t[VolumeGroupName [VolumeGroupName...]]\n"
   "\n"
   "vgdisplay --columns|-C\n"
   "\t[--aligned]\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|--help] " "\n"
   "\t[--ignorelockingfailure]" "\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[-P|--partial] " "\n"
   "\t[--separator Separator]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[--verbose]" "\n"
   "\t[--version]" "\n"
   "\t[VolumeGroupName [VolumeGroupName...]]\n",

   activevolumegroups_ARG, aligned_ARG, colon_ARG, columns_ARG, disk_ARG,
   ignorelockingfailure_ARG, noheadings_ARG, nosuffix_ARG, options_ARG,
   partial_ARG, short_ARG, separator_ARG, sort_ARG, unbuffered_ARG, units_ARG)

xx(vgexport,
   "Unregister volume group(s) from the system",
   0,
   "vgexport " "\n"
   "\t[-a|--all] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|--help]" "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version] " "\n"
   "\tVolumeGroupName [VolumeGroupName...]\n",

   all_ARG, test_ARG)

xx(vgextend,
   "Add physical volumes to a volume group",
   0,
   "vgextend\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tVolumeGroupName PhysicalDevicePath [PhysicalDevicePath...]\n",

   autobackup_ARG, test_ARG)

xx(vgimport,
   "Register exported volume group with system",
   0,
   "vgimport " "\n"
   "\t[-a|--all]\n"
   "\t[-d|--debug] " "\n"
   "\t[-f|--force] " "\n"
   "\t[-h|--help] " "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose]" "\n"
   "\t[--version]" "\n"
   "\tVolumeGroupName..." "\n",

   all_ARG, force_ARG, test_ARG)

xx(vgmerge,
   "Merge volume groups",
   0,
   "vgmerge\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-l|--list]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tDestinationVolumeGroupName SourceVolumeGroupName\n",

   autobackup_ARG, list_ARG, test_ARG)

xx(vgmknodes,
   "Create the special files for volume group devices in /dev",
   0,
   "vgmknodes\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--refresh]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\t[VolumeGroupName...]\n",

   ignorelockingfailure_ARG, refresh_ARG)

xx(vgreduce,
   "Remove physical volume(s) from a volume group",
   0,
   "vgreduce\n"
   "\t[-a|--all]\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--mirrorsonly]\n"
   "\t[--removemissing]\n"
   "\t[-f|--force]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tVolumeGroupName\n"
   "\t[PhysicalVolumePath...]\n",

   all_ARG, autobackup_ARG, force_ARG, mirrorsonly_ARG, removemissing_ARG,
   test_ARG)

xx(vgremove,
   "Remove volume group(s)",
   0,
   "vgremove\n"
   "\t[-d|--debug]\n"
   "\t[-f|--force]\n"
   "\t[-h|--help]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tVolumeGroupName [VolumeGroupName...]\n",

   force_ARG, test_ARG)

xx(vgrename,
   "Rename a volume group",
   0,
   "vgrename\n"
   "\t[-A|--autobackup y|n]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[-t|--test]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n"
   "\tOldVolumeGroupPath NewVolumeGroupPath |\n"
   "\tOldVolumeGroupName NewVolumeGroupName\n",

   autobackup_ARG, force_ARG, test_ARG)

xx(vgs,
   "Display information about volume groups",
   0,
   "vgs" "\n"
   "\t[--aligned]\n"
   "\t[-a|--all]\n"
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--nameprefixes]\n"
   "\t[--noheadings]\n"
   "\t[--nosuffix]\n"
   "\t[-o|--options [+]Field[,Field]]\n"
   "\t[-O|--sort [+|-]key1[,[+|-]key2[,...]]]\n"
   "\t[-P|--partial] " "\n"
   "\t[--rows]\n"
   "\t[--separator Separator]\n"
   "\t[--trustcache]\n"
   "\t[--unbuffered]\n"
   "\t[--units hsbkmgtHKMGT]\n"
   "\t[--unquoted]\n"
   "\t[-v|--verbose]\n"
   "\t[--version]\n"
   "\t[VolumeGroupName [VolumeGroupName...]]\n",

   aligned_ARG, all_ARG, ignorelockingfailure_ARG, nameprefixes_ARG,
   noheadings_ARG, nolocking_ARG, nosuffix_ARG, options_ARG, partial_ARG, 
   rows_ARG, separator_ARG, sort_ARG, trustcache_ARG, unbuffered_ARG, units_ARG,
   unquoted_ARG)

xx(vgscan,
   "Search for all volume groups",
   0,
   "vgscan "
   "\t[-d|--debug]\n"
   "\t[-h|--help]\n"
   "\t[--ignorelockingfailure]\n"
   "\t[--mknodes]\n"
   "\t[-P|--partial] " "\n"
   "\t[-v|--verbose]\n"
   "\t[--version]" "\n",

   ignorelockingfailure_ARG, mknodes_ARG, partial_ARG)

xx(vgsplit,
   "Move physical volumes into a new or existing volume group",
   0,
   "vgsplit " "\n"
   "\t[-A|--autobackup {y|n}] " "\n"
   "\t[--alloc AllocationPolicy] " "\n"
   "\t[-c|--clustered {y|n}] " "\n"
   "\t[-d|--debug] " "\n"
   "\t[-h|--help] " "\n"
   "\t[-l|--maxlogicalvolumes MaxLogicalVolumes]" "\n"
   "\t[-M|--metadatatype 1|2] " "\n"
   "\t[-n|--name LogicalVolumeName]\n"
   "\t[-p|--maxphysicalvolumes MaxPhysicalVolumes] " "\n"
   "\t[-t|--test] " "\n"
   "\t[-v|--verbose] " "\n"
   "\t[--version]" "\n"
   "\tSourceVolumeGroupName DestinationVolumeGroupName" "\n"
   "\t[PhysicalVolumePath...]\n",

   alloc_ARG, autobackup_ARG, clustered_ARG,
   maxlogicalvolumes_ARG, maxphysicalvolumes_ARG,
   metadatatype_ARG, name_ARG, test_ARG)

xx(version,
   "Display software and driver version information",
   0,
   "version\n" )

