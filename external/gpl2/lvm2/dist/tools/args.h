/*	$NetBSD: args.h,v 1.1.1.2 2009/12/02 00:25:46 haad Exp $	*/

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

/*
 * Put all long args that don't have a
 * corresponding short option first ...
 */
/* *INDENT-OFF* */
arg(version_ARG, '\0', "version", NULL, 0)
arg(quiet_ARG, '\0', "quiet", NULL, 0)
arg(physicalvolumesize_ARG, '\0', "setphysicalvolumesize", size_mb_arg, 0)
arg(ignorelockingfailure_ARG, '\0', "ignorelockingfailure", NULL, 0)
arg(nolocking_ARG, '\0', "nolocking", NULL, 0)
arg(pvmetadatacopies_ARG, '\0', "pvmetadatacopies", int_arg, 0)
arg(metadatacopies_ARG, '\0', "metadatacopies", int_arg, 0)
arg(metadatasize_ARG, '\0', "metadatasize", size_mb_arg, 0)
arg(restorefile_ARG, '\0', "restorefile", string_arg, 0)
arg(labelsector_ARG, '\0', "labelsector", int_arg, 0)
arg(driverloaded_ARG, '\0', "driverloaded", yes_no_arg, 0)
arg(aligned_ARG, '\0', "aligned", NULL, 0)
arg(unbuffered_ARG, '\0', "unbuffered", NULL, 0)
arg(noheadings_ARG, '\0', "noheadings", NULL, 0)
arg(segments_ARG, '\0', "segments", NULL, 0)
arg(units_ARG, '\0', "units", string_arg, 0)
arg(nosuffix_ARG, '\0', "nosuffix", NULL, 0)
arg(removemissing_ARG, '\0', "removemissing", NULL, 0)
arg(abort_ARG, '\0', "abort", NULL, 0)
arg(addtag_ARG, '\0', "addtag", tag_arg, 0)
arg(deltag_ARG, '\0', "deltag", tag_arg, 0)
arg(refresh_ARG, '\0', "refresh", NULL, 0)
arg(mknodes_ARG, '\0', "mknodes", NULL, 0)
arg(minor_ARG, '\0', "minor", minor_arg, 0)
arg(type_ARG, '\0', "type", segtype_arg, 0)
arg(alloc_ARG, '\0', "alloc", alloc_arg, 0)
arg(separator_ARG, '\0', "separator", string_arg, 0)
arg(mirrorsonly_ARG, '\0', "mirrorsonly", NULL, 0)
arg(nosync_ARG, '\0', "nosync", NULL, 0)
arg(resync_ARG, '\0', "resync", NULL, 0)
arg(corelog_ARG, '\0', "corelog", NULL, 0)
arg(mirrorlog_ARG, '\0', "mirrorlog", string_arg, 0)
arg(repair_ARG, '\0', "repair", NULL, 0)
arg(use_policies_ARG, '\0', "use-policies", NULL, 0)
arg(monitor_ARG, '\0', "monitor", yes_no_arg, 0)
arg(config_ARG, '\0', "config", string_arg, 0)
arg(trustcache_ARG, '\0', "trustcache", NULL, 0)
arg(ignoremonitoring_ARG, '\0', "ignoremonitoring", NULL, 0)
arg(nameprefixes_ARG, '\0', "nameprefixes", NULL, 0)
arg(unquoted_ARG, '\0', "unquoted", NULL, 0)
arg(rows_ARG, '\0', "rows", NULL, 0)
arg(dataalignment_ARG, '\0', "dataalignment", size_kb_arg, 0)
arg(dataalignmentoffset_ARG, '\0', "dataalignmentoffset", size_kb_arg, 0)
arg(virtualoriginsize_ARG, '\0', "virtualoriginsize", size_mb_arg, 0)
arg(virtualsize_ARG, '\0', "virtualsize", size_mb_arg, 0)
arg(noudevsync_ARG, '\0', "noudevsync", NULL, 0)

/* Allow some variations */
arg(resizable_ARG, '\0', "resizable", yes_no_arg, 0)
arg(allocation_ARG, '\0', "allocation", yes_no_arg, 0)

/*
 * ... and now the short args.
 */
arg(available_ARG, 'a', "available", yes_no_excl_arg, 0)
arg(all_ARG, 'a', "all", NULL, 0)
arg(autobackup_ARG, 'A', "autobackup", yes_no_arg, 0)
arg(activevolumegroups_ARG, 'A', "activevolumegroups", NULL, 0)
arg(background_ARG, 'b', "background", NULL, 0)
arg(blockdevice_ARG, 'b', "blockdevice", NULL, 0)
arg(chunksize_ARG, 'c', "chunksize", size_kb_arg, 0)
arg(clustered_ARG, 'c', "clustered", yes_no_arg, 0)
arg(colon_ARG, 'c', "colon", NULL, 0)
arg(columns_ARG, 'C', "columns", NULL, 0)
arg(contiguous_ARG, 'C', "contiguous", yes_no_arg, 0)
arg(debug_ARG, 'd', "debug", NULL, ARG_REPEATABLE)
arg(exported_ARG, 'e', "exported", NULL, 0)
arg(physicalextent_ARG, 'E', "physicalextent", NULL, 0)
arg(file_ARG, 'f', "file", string_arg, 0)
arg(force_ARG, 'f', "force", NULL, ARG_REPEATABLE)
arg(full_ARG, 'f', "full", NULL, 0)
arg(help_ARG, 'h', "help", NULL, 0)
arg(help2_ARG, '?', "", NULL, 0)
arg(stripesize_ARG, 'I', "stripesize", size_kb_arg, 0)
arg(stripes_ARG, 'i', "stripes", int_arg, 0)
arg(interval_ARG, 'i', "interval", int_arg, 0)
arg(iop_version_ARG, 'i', "iop_version", NULL, 0)
arg(logicalvolume_ARG, 'l', "logicalvolume", int_arg, 0)
arg(maxlogicalvolumes_ARG, 'l', "maxlogicalvolumes", int_arg, 0)
arg(extents_ARG, 'l', "extents", int_arg_with_sign_and_percent, 0)
arg(lvmpartition_ARG, 'l', "lvmpartition", NULL, 0)
arg(list_ARG, 'l', "list", NULL, 0)
arg(size_ARG, 'L', "size", size_mb_arg, 0)
arg(logicalextent_ARG, 'L', "logicalextent", int_arg_with_sign, 0)
arg(persistent_ARG, 'M', "persistent", yes_no_arg, 0)
arg(major_ARG, 'j', "major", major_arg, 0)
arg(mirrors_ARG, 'm', "mirrors", int_arg_with_sign, 0)
arg(metadatatype_ARG, 'M', "metadatatype", metadatatype_arg, 0)
arg(maps_ARG, 'm', "maps", NULL, 0)
arg(name_ARG, 'n', "name", string_arg, 0)
arg(oldpath_ARG, 'n', "oldpath", NULL, 0)
arg(nofsck_ARG, 'n', "nofsck", NULL, 0)
arg(novolumegroup_ARG, 'n', "novolumegroup", NULL, 0)
arg(options_ARG, 'o', "options", string_arg, 0)
arg(sort_ARG, 'O', "sort", string_arg, 0)
arg(permission_ARG, 'p', "permission", permission_arg, 0)
arg(maxphysicalvolumes_ARG, 'p', "maxphysicalvolumes", int_arg, 0)
arg(partial_ARG, 'P', "partial", NULL, 0)
arg(physicalvolume_ARG, 'P', "physicalvolume", NULL, 0)
arg(readahead_ARG, 'r', "readahead", readahead_arg, 0)
arg(resizefs_ARG, 'r', "resizefs", NULL, 0)
arg(reset_ARG, 'R', "reset", NULL, 0)
arg(regionsize_ARG, 'R', "regionsize", size_mb_arg, 0)
arg(physicalextentsize_ARG, 's', "physicalextentsize", size_mb_arg, 0)
arg(stdin_ARG, 's', "stdin", NULL, 0)
arg(snapshot_ARG, 's', "snapshot", NULL, 0)
arg(short_ARG, 's', "short", NULL, 0)
arg(test_ARG, 't', "test", NULL, 0)
arg(uuid_ARG, 'u', "uuid", NULL, 0)
arg(uuidstr_ARG, 'u', "uuid", string_arg, 0)
arg(uuidlist_ARG, 'U', "uuidlist", NULL, 0)
arg(verbose_ARG, 'v', "verbose", NULL, ARG_REPEATABLE)
arg(volumegroup_ARG, 'V', "volumegroup", NULL, 0)
arg(allocatable_ARG, 'x', "allocatable", yes_no_arg, 0)
arg(resizeable_ARG, 'x', "resizeable", yes_no_arg, 0)
arg(yes_ARG, 'y', "yes", NULL, 0)
arg(zero_ARG, 'Z', "zero", yes_no_arg, 0)

/* this should always be last */
arg(ARG_COUNT, '-', "", NULL, 0)
/* *INDENT-ON* */
