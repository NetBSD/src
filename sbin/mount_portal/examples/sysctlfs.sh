#!/bin/sh
## $NetBSD: sysctlfs.sh,v 1.1 2000/01/17 07:22:45 bgrayson Exp $
##  Fast hack at a sysctl filesystem.  The path can be either in
##  dot-style (kern.mbuf.msize) or in slash-style (kern/mbuf/msize).
##  Hacked as an example by Brian Grayson (bgrayson@netbsd.org) in Sep 1999.
for path in $*; do
  ##  First, change any slashes into dots.
  path=`echo $path | tr '/' '.'`
  ##  Now invoke sysctl, and post-process the output to make it
  ##  friendlier.  In particular:
  ##    1.  Remove the leading prefix.
  ##    2.  Remove a now-leading ".", if any.
  ##    3.  If we are left with " = <val>", strip out the " = " also.
  sysctl $path | sed -e "s/$path//;s/^\.//;s/^ = //"
done
