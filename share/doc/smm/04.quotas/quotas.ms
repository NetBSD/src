.\"	$NetBSD: quotas.ms,v 1.4.56.1 2012/08/15 01:20:23 riz Exp $
.\"
.\" Copyright (c) 1983, 1993
.\"	The Regents of the University of California.  All rights reserved.
.\"
.\" Redistribution and use in source and binary forms, with or without
.\" modification, are permitted provided that the following conditions
.\" are met:
.\" 1. Redistributions of source code must retain the above copyright
.\"    notice, this list of conditions and the following disclaimer.
.\" 2. Redistributions in binary form must reproduce the above copyright
.\"    notice, this list of conditions and the following disclaimer in the
.\"    documentation and/or other materials provided with the distribution.
.\" 3. Neither the name of the University nor the names of its contributors
.\"    may be used to endorse or promote products derived from this software
.\"    without specific prior written permission.
.\"
.\" THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
.\" ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
.\" IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
.\" ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
.\" FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
.\" DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
.\" OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
.\" HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
.\" LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
.\" OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
.\" SUCH DAMAGE.
.\"
.\"	@(#)quotas.ms	8.1 (Berkeley) 6/8/93
.\"
.\"
.\" Copyright (c) 2012 The NetBSD Foundation, Inc.
.\" All rights reserved.
.\"
.\" This code is derived from software contributed to The NetBSD Foundation
.\" by David A. Holland.
.\"
.\" Redistribution and use in source and binary forms, with or without
.\" modification, are permitted provided that the following conditions
.\" are met:
.\" 1. Redistributions of source code must retain the above copyright
.\"    notice, this list of conditions and the following disclaimer.
.\" 2. Redistributions in binary form must reproduce the above copyright
.\"    notice, this list of conditions and the following disclaimer in the
.\"    documentation and/or other materials provided with the distribution.
.\"
.\" THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
.\" ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
.\" TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
.\" PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
.\" BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
.\" CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
.\" SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
.\" INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
.\" CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
.\" ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
.\" POSSIBILITY OF SUCH DAMAGE.
.\"
.\"
.\" .EH 'SMM:4-%''Disc Quotas in a \s-2UNIX\s+2 Environment'
.\" .OH 'Disc Quotas in a \s-2UNIX\s+2 Environment''SMM:4-%'
.\" .ND 5th July, 1983
.\" .TL
.\" Disc Quotas in a \s-2UNIX\s+2\s-3\u*\d\s0 Environment
.\" .FS
.\" * UNIX is a trademark of Bell Laboratories.
.\" .FE
.EH 'SMM:4-%''Disk Quotas in NetBSD'
.OH 'Disk Quotas in NetBSD''SMM:4-%'
.ND May 20, 2012
.TL
Disk Quotas in NetBSD
.AU
David A. Holland
.\" (XXX: what's the best way to do this?)
.\" Robert Elz
.AI
Based upon an earlier version by Robert Elz.
.AB
.PP
In most computing environments, disk space is not
infinite.
The disk quota system provides a mechanism
to control usage of disk space on an
individual basis.
.PP
Quotas may be set for each individual user, and for groups.
Quotas may be set (or not set) independently on each mounted file
system.
Quotas are not supported on all file system types, but do work over
NFS.
.PP
The quota system will warn users when they
exceed their allotted limit, but allow some
extra space for current work.
Remaining over quota for long periods of time
will eventually cause a fatal over-quota condition.
.PP
File system independent access to quotas is provided via a library,
\fIlibquota\fP\|(3),
shared by the quota tools and available to third-party software.
Dump and restore tools are provided for backing up quota information.
.AE
.NH 1
Users' view of disk quotas
.PP
To an ordinary user, having a disk quota is almost the same as having
a smaller disk.
When the amount of space available is consumed, further attempts to
use more will fail.
.PP
To make this less painful, the quota system actually provides two
limits for every resource: a
\fIhard\fP
limit and a
\fIsoft\fP
limit.
The hard limit may not be exceeded.
The soft limit may be exceeded temporarily: when the soft limit is
first exceeded, a timer is initialized.
If usage is reduced below the soft limit, the timer is cleared.
Otherwise, when the timer expires, the soft limit becomes the hard
limit: until usage is reduced below the soft limit, further use of
space is denied.
The length of time allowed is called the
\fIgrace period\fP
and is configurable by the system administrator.
.PP
Quotas may be applied to both users and groups.
When both a user quota and a group quota apply, the most restrictive
is used.
.PP
The basic purpose of quotas is to restrict the usage of disk space.
This is measured in blocks.
(For historical reasons, quotas are counted in terms of 512-octet
blocks, even though most file systems use larger blocks internally.)
Quotas can be imposed on other resource types too.
Generally there will also be a quota on the number of files that may
be owned.
This functionality is supported by nearly all file system types that
support quotas at all.
Some file system types may support additional quotas on other file
system resources.
.PP
The
\fIquota\fP\|(1)
command provides information on the disk quotas applying to a user or
group.
For each resource type, and each file system, it prints the current
usage, the soft limit
(`quota'),
the hard limit
(`limit'),
and the expiration time of the current grace period, if any.
User quotas are reported by default; the
\fB\-g\fP
flag allows retrieving group quotas.
.PP
The soft limit is the usage level that the user is expected to remain
below.
As described above, the soft limit can be exceeded temporarily;
.\" XXX: does it really nowadays? I'm not so sure it does...
doing so generates a warning.
The hard limit cannot be exceeded.
When the usage exceeds the applicable limit, further requests for
space (or attempts to create a file, or allocate other resources)
will fail with the
EDQUOT
error.
The first time
this occurs, a message will be written to the user's
terminal.
To avoid flooding, the message will not be repeated until after the
usage is lowered again.
.NH 2 
Surviving when quota limit is reached
.PP
In most cases, the only way to recover from an over-quota
condition is to stop whatever activity was in progress, remove
sufficient files to bring the limit back below quota,
and retry the failed program.
.PP
Be careful not to exit applications whose document save operations
have or may have failed because of the over-quota conditions.
This can often lead to losing data: the prior saved version of the
document may have been replaced with an empty, partial, or invalid
half-saved version.
If it is not possible to open an additional terminal or other tools to
do the clean-up work, use job control to suspend these applications
rather than exiting.
It may also be possible to save documents to a different file system
(perhaps in /tmp)
and then move them back into place after cleaning up.
.NH 1
Back-end quota implementations
.PP
In NetBSD there are three categories of quota implementations.
First are file systems with native internal support for quotas.
In these, the quota data is stored internally within the file system,
maintained by the file system's own consistency management and
\fIfsck\fP\|(8)
utility, and active whenever the file system volume is mounted.
The mechanism for setting up quotas on a particular volume is
file-system-dependent but generally involves arguments to
\fInewfs\fP\|(8)
and/or
\fItunefs\fP\|(8).
The
``quota2''
quotas for
FFS
that appeared in
NetBSD 6.0
are an example of this type.
.PP
The second category is
NFS.
In NFS, quotas are handled by a separate SunRPC protocol not directly
connected to the NFS mount.
The quota utilities speak this protocol when invoked on NFS volumes.
On an NFS server, the
\fIrpc.rquotad\fP\|(8)
daemon must be enabled to serve this protocol.
See below for further details.
.PP
The third category is instances of the historical quota system,
which was developed in the 1980s and remained largely unchanged until
being replaced during the development of
NetBSD 6.0.
In the historical quota system, quotas are stored in external
(user-visible)
files.
These appear by default in the root directory of the volume they apply
to, but may be placed anywhere, including sometimes on another file
system.
While file-system-level support is required, and enforcement is done in
the file system, much of the work involved
in maintaining the quota system is external to the file system.
The program
\fIquotacheck\fP\|(8)
is run at boot time as a form of
\fIfsck\fP\|(8)
for the quota information.
Once this is done, the program
\fIquotaon\fP\|(8)
is used to activate the quotas.
The program
\fIquotaoff\fP\|(8)
is used to deactivate the quotas during system shutdown.
This is normally done before unmounting; if not, unmounting will
do it implicitly.
If the file system volume is modified after quotaoff is run,
quotacheck
\fImust\fP
be run before quotaon is run again, or the quota data will become
corrupted.
Setting up the historical quota system requires several steps; see
below.
The original or
``quota1''
quotas for FFS are an example of this type of quota implementation.
.PP
The
\fIlibquota\fP\|(3)
library provides a common interface to all three implementation types,
so the same quota administration tools can be used with all of them.
Not all functionality is available in all cases, however.
For example, NFS quotas are read-only.
To adjust quotas on an NFS volume, one must log into its file server.
.NH 1
Setting up quotas
.PP
To set up quotas of any type requires several steps.
These are described in detail, for each file system type supporting
quotas, in the following sections.
.PP
The first step in all cases, however, is to decide what file systems
need to have quotas.
Typically, only file systems that house users' home directories,
or other user files, will need quotas.
Do not forget the mail spool.
It may also prove useful to also include
\fB/usr\fR.
If possible, \fB/tmp\fP should preferably be free of quotas.
.PP
It is also necessary to decide what the quotas will be, keeping in
mind the available space and the legitimate needs of the user
population.
.PP
Note that because the quota system tracks per-user and per-group disk
usage, it can be useful in some contexts to enable quotas purely to
provide that information.
It is perfectly reasonable in this case to set quotas that are the
same size as, or larger than, the space available on the volume.
.NH 2
Setting up new
(``quota2'')
FFS quotas
.PP
First, make sure your kernel includes FFS and FFS quota2 support.
These are present if the lines
.DS
file-system  FFS 
options      QUOTA2
.DE
(respectively)
are present in the kernel configuration.
In
NetBSD 6.0
and higher the
GENERIC
kernels should all include FFS and FFS quota2 support.
If FFS is compiled as a loadable module, the module will always
include quota2 support.
.\" .FS
.\" * See also the document ``Building 4.2BSD UNIX Systems with Config''.
.\" .FE
.PP
Then, when creating the volume, use the
\fB\-q\fP user
and/or
\fB\-q\fP group
options to
\fInewfs\fP\|(8)
as desired.
This will create and initialize the on-disk data structures for
user and/or group quotas respectively.
These can also be created using
\fItunefs\fP\|(8).
.PP
There is no need to set mount options in
\fIfstab\fP\|(5).
In particular, do
\fInot\fP
set the
userquota
or
groupquota
options as these are used for controlling the historic quota system
only.
Trying to use both quota systems at once will fail in possibly obscure
ways.
.NH 2
Setting up quotas on an NFS client
.PP
There is nothing special that needs to be done on NFS clients.
Enforcement (and configuration) of quotas occurs on the NFS server.
The
\fIlibquota\fP\|(3)
library recognizes NFS mounts and automatically uses the
rquotad
protocol to handle them.
.NH 2
Setting up quotas on an NFS server
.PP
To set up quotas on an NFS server, first set up quotas on the file
systems that will be served via NFS.
Then enable the
\fIrquotad\fP\|(8)
service
in
\fB/etc/inetd.conf\fP\|(5).
Restart
(or start)
\fIinetd\fP\|(8)
if necessary.
Make sure
.DS
inetd=YES
.DE
is present in
\fB/etc/rc.conf\fP.
This is the default and should not need to be changed explicitly.
.NH 2
Setting up historical
(``quota1'')
FFS quotas
.PP
First, make sure your kernel includes FFS and FFS quota1 support.
These are present if the lines
.DS
file-system  FFS 
options      QUOTA
.DE
(respectively)
are present in the kernel configuration.
In all NetBSD versions the
GENERIC
kernels should include FFS and FFS quota1 support.
If FFS is compiled as a loadable module, the module will always
include quota1 support.
.\" .FS
.\" * See also the document ``Building 4.2BSD UNIX Systems with Config''.
.\" .FE
.PP
Note that it is possible that the historic quota system will be
removed entirely at some point in the future.
This is not presently intended, however.
.PP
No special options are required when creating the file system.
Instead, add the
userquota
and/or
groupquota
options to the file system's entry in
\fIfstab\fP\|(5).
The file system
\fImust\fP
be listed in
\fIfstab\fP\|(5)
for the historical quota system to work.
To use quota files other than the default, use the form
userquota=/path/to/file
and/or
groupquota=/path/to/file
as desired.
The default files are
\fBquota.user\fP
and
\fBquota.group\fP
respectively.
Create empty quota files with
\fItouch\fP\|(1).
.PP
If the file system is not brand new, now run
\fIquotacheck\fP\|(8)
on it.
(The file system must be mounted for this step.
Be sure nothing else is writing to the file system during this time.)
.PP
Now run
\fIquotaon\fP\|(8)
on the file system.
.PP
You must also make sure that the setting
.DS
quota=YES
.DE
is present in
\fB/etc/rc.conf\fP.
This is the default and should not need to be changed explicitly.
This setting causes
\fIquotacheck\fP\|(8)
and
\fIquotaon\fP\|(8)
to be run at system boot time.
.NH 2
Setting up historical quotas on other file system types
.PP
In theory, the historical quota system can also be used on
LFS
and
ext2fs
file systems.
The procedure for this is the same as for FFS.
.PP
There is definitely at least some code present to support quotas on
LFS; however, as of this writing it is believed that quotas do not
actually work with either LFS or ext2fs.
.NH 1
Quota administration
.PP
After the quota system has been set up, and also on a continuing basis
as users come and go and their requirements change, the actual limit
values need to be configured.
This is done with the
\fIedquota\fP\|(8)
command.
.PP
In cases where large classes of users are to be given the same quota
settings, the
\fB\-p\fP
option of
\fIedquota\fP\|(8)
can be used to streamline the process.
.PP
There is also a
\fIdefault\fP
entry.
Users (or groups) whose quotas have not been set explicitly are
subjected to the default quotas instead.
.PP
The
\fIquota\fP\|(1)
command can be used to inspect an individual user or group's quotas
across all file systems.
The
\fIrepquota\fP\|(8)
command can be used to retrieve quotas across all users or groups on a
per-file-system basis.
.NH 2
Administrative considerations for native quotas
.PP
The
\fIquotacheck\fP\|(8),
\fIquotaon\fP\|(8),
and
\fIquotaoff\fP\|(8)
programs cannot be used with native quotas.
They will fail if run.
.PP
To avoid confusion be sure not to use the
userquota
or
groupquota
options in
\fIfstab\fP\|(5)
on a file system with native quotas.
Due to implementation quirks of the historic quota system, using these
options will not necessarily cause errors at mount time, but may confuse
\fIquotacheck\fP\|(8)
and/or
\fIquotaon\fP\|(8)
and produce bizarre results.
.PP
Native quotas do not require
\fIquotacheck\fP\|(8)
and thus can be much faster at boot time.
In particular, the native FFS quotas used in conjunction with WAPBL
journaling are themselves journaled and require only a journal replay
after a crash.
.PP
Note however that native FFS quotas are not backward-compatible to
older NetBSD installations.
As of this writing they are also not understood by FreeBSD's FFS
implementation.
.PP
There is currently no way to temporarily suspend enforcement of native
quotas.
.NH 2
Administrative considerations for NFS quotas
.PP
Most quota administration should
(must)
be done on the NFS server by working with the volumes being served.
The
rquotad
protocol is very limited and supports only the most basic operations.
Notably,
\fIrepquota\fP\|(8)
does not work on NFS volumes.
.NH 2
Administrative considerations for historic quotas
.PP
The historic quota system does not support all of the possible
functionality.
There is no separate default entry, and the grace period cannot be
configured individually; instead, one grace period is configured for
all users.
.PP
In the historic quota system the default values and the global grace
period are stored in the quota entry for UID 
(or GID)
0.
This scheme partly shows through to the quota tools.
It is also possible to attempt to establish quota configurations that
cannot be represented.
These will fail.
\fIedquota\fP\|(8)
attempts to detect these before submitting changes to the kernel in
order to offer a cogent error message; however, it may not always
succeed.
.PP
Before
\fIquotaon\fP\|(8)
is run, the quota information is not accessible to the kernel.
The
\fIlibquota\fP\|(3)
library detects this case and falls back to direct access to the quota
files.
This should be fully transparent but it is possible that glitches may
arise.
.PP
It is possible to temporarily disable quota enforcement by using
\fIquotaoff\fP\|(8).
However, this also disables usage tracking.
Consequently, if this is done while the system is live, it is in
general necessary to run
\fIquotacheck\fP\|(8)
to correct the usage information before running
\fIquotaon\fP\|(8)
again, and the file system must be idle between the time quotacheck is
started and the time quotaon completes.
Otherwise the usage information in the quota files will go out of sync
with the file system.
This can lead to improper behavior later on, and in some cases may
cause panics.
.PP
The historical quota system is 32-bit and thus cannot cope with quotas
or usage amounts that cannot be represented in a 32-bit value.
Use native quotas on large volumes.
.NH 1
Backups
.PP
While the disk usage information in the quota data can be
reconstructed by scanning the file system
(this is what quotacheck does),
the configured quotas themselves are system configuration that should
in general be backed up.
With the historical quota system, the quota information is stored in
regular files and is backed up in the normal way like other files.
However, with native quotas, the quota information is hidden inside
the file system and additional steps are necessary to back it up.
.PP
The
\fIquotadump\fP\|(8)
and
\fIquotarestore\fP\|(8)
programs are provided for this purpose.
.PP
The
\fIquotadump\fP\|(8)
program is the same as
\fIrepquota\fP\|(8)
with the
\fB-x\fP
option.
It produces a complete tabular dump of the quota settings on the
selected file system.
This dump file can be saved on backup media.
.PP
The
\fIquotarestore\fP\|(8)
reads a dump file produced by
\fIquotadump\fP\|(8)
and enters the configured quota information into the selected file
system.
It can be used to restore from backup, or to migrate quotas from one
volume to another.
.NH 1
Migrating to the new native FFS quota implementation
.PP
The procedure for migrating from the historical
(``quota1'')
FFS quotas to the native
(``quota2'')
FFS quotas is as follows.
.PP
First, make sure the volume being migrated is fully backed up.
This is important in case something goes wrong.
.PP
Now, drop to single user mode.
Then, dump the existing quota information with
\fIquotadump\fP\|(8).
Save the dump file someplace safe, i.e. not in
\fB/tmp\fP,
in case it becomes necessary to reboot.
.PP
Unmount the volume.
Edit
\fB/etc/fstab\fP
and remove the
userquota
and/or
groupquota
options.
Leave the old quota files in place for now; they will do no harm.
.PP
Use
\fItunefs\fP\|(8)
to add native quotas to the file system.
If the volume is very old you might first need to update the
superblock.
.PP
Mount the file system.
Use
\fIquotarestore\fP\|(8)
to load the dump file into the new quota system.
.PP
Confirm using
\fIrepquota\fP\|(8)
and/or
\fIquota\fP\|(1)
and/or by explicit testing
that the quotas have been loaded and the new quota system is behaving
as intended.
If paranoid, reboot and test again as a precaution.
.PP
Once you are fully satisfied, preferably after a few days' usage,
delete or archive the old quota files and the dump file used for
transition.
.PP
Remember to set up backup procedures for the quota data.
.NH 1
Summary of changes from the historic to new/native FFS quota implementations
.PP
Quotas are set up with newfs or tunefs, rather than by editing fstab
and running quotacheck.
.PP
The quotacheck, quotaon, and quotaoff tools are not used.
Repair is done with fsck instead.
.PP
The quotas are integrated with WAPBL journaling, allowing fast crash
recovery.
.PP
The userquota and groupquota mount options are not used.
.PP
The grace period is per-user instead of global.
.PP
The defaults do not overlap with the id 0 quota entry.
.NH 1
Some implementation details for the historic quotas
.PP
The data in the quota files is an array of
dquot
structures, indexed by id
(UID or GID).
There is an entry for every id on the system, whether or not
a quota is configured for that id.
If the id space is sparse, then the file may have holes in it.
Copying the files will fill in the holes, so it is best to avoid this.
.PP
The
userquota
and
groupquota
options are actually ignored by
\fImount\fP\|(8).
They are instead found at run time by the quota tools
(actually by libquota);
this is why file systems using historic quotas must be listed in
\fB/etc/fstab\fP.
.PP
The kernel is informed of the existence and identities of the quota
files by the
\fIquotaon\fP\|(8)
utility.
Until this point
(e.g. while quotacheck is running)
the kernel does not know which volumes are supposed to have quotas and
which are not.
.PP
When quotas are turned on,
the kernel reads the quota entries for each user
(or group)
currently active, and then the quota entries needed for any files open
owned by users
(or groups)
who are not currently active.
Each subsequent open of a file on the file system will
be accompanied by a pairing with its quota information.
In most cases this information will be retained in core,
either because the user who owns the file is running some
process, because other files are open owned by the same
user, or because some file (perhaps this one) was recently
accessed.
In memory, the quota information is kept hashed by id
and file system, and retained in an LRU chain so recently
released data can be easily reclaimed.
Information about those users whose last process has
recently terminated is also retained in this way.
.PP
Each time a block is accessed or released, and each time an inode
is allocated or freed, the quota system gets told
about it, and in the case of allocations, gets the
opportunity to object.
.\" .PP
.\" Measurements have shown
.\" that the quota code uses a very small percentage of the system
.\" CPU time consumed in writing a new block to disk.
.NH 1
History and acknowledgments
.PP
The historic quota system was loosely based upon a very
early scheme implemented at the University of New South
Wales, and Sydney University in the mid 70's. That system
implemented a single combined limit for both files and blocks
on all file systems.
.PP
A later system was implemented at the University of Melbourne
by Robert Elz, but was not kept highly accurately.
For example, chown did not affect quotas, nor did I/O to a file
owned by a different user.
.PP
The historic quota system was put into place
in January 1982 at Melbourne.
.\" It is actually just a small part of a much broader resource
.\" control scheme, which is capable of controlling almost
.\" anything that is usually uncontrolled in UNIX. The rest
.\" of this is, as yet, still in a state where it is far too
.\" subject to change to be considered for distribution.
.PP
For the 4.2BSD release, much work was done to clean
up and sanely incorporate the quota code by Sam Leffler and
Kirk McKusick at The University of California at Berkeley.
.PP
The historic quota system has remained in use
(with only minor modifications)
through many BSD releases and versions from then right up until 2011,
and remains available.
.PP
In 2011, Manuel Bouyer implemented a new quota system for FFS
with the properties described above
(in-FS, journaled, etc.)
and reworked the kernel interface.
That interface was later withdrawn.
In 2012 David A. Holland implemented a simpler kernel interface and
more comprehensive quota library and reworked the quota tools to be
file-system-independent.
This material first appeared in
NetBSD 6.0.
