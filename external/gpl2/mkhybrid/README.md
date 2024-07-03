# tools'fied mkhybrid to build HFS/ISO9660 hybrid image for mac68k and macppc

(See src/distrib/cdrom/README how to fetch set binaries and build iso images)

## What's this?

This external/gpl2/mkhybrid provides tools'fied mkhybrid(8) to build
HFS/ISO9660 hybrid CD images for mac68k and macppc install media,
based on mkhybrid 1.12b5.1 in OpenBSD 7.3:
 http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/gnu/usr.sbin/mkhybrid/src/

## Changes from OpenBSD's one

- pull sources in OpenBSD's src/gnu/usr.sbin/mkhybrid/src except libfile
  into NetBSD's src/external/gpl2/mkhybrid/dist
  (unnecessary files for tools builds are not imported)
- pull 2 clause BSD licensed libfile sources from upstream cdrtools-3.01
- pull Makefile in OpenBSD's src/gnu/usr.sbin/mkhybrid/mkhybrid
  into NetBSD's src/external/gpl2/mkhybrid/bin
- src/external/gpl2/mkhybrid/bin is prepared to build tools version
  in src/tools/mkhybrid using src/tools/Makefile.host
- tweak configure to pull several header files for NetBSD tools builds
- appease various dumb warnings
- pull -hide-rr-moved option from upstream mkisofs-1.13
- pull -graft-points option from upstream mkisofs-1.13 and cdrtools-2.01
- pull malloc related fixes in tree.c from upstream cdrtools-2.01

## Current status

- builds on NetBSD, ubuntu, and Cygwin hosts are tested

See commit logs and diffs for more details.

## TODO

- add support to specify permissions via mtree-specfiles
  as native makefs(8) for non-root build
