<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->
## BIND 9 Source Access and Contributor Guidelines
*May 28, 2020*

### Contents

1. [Access to source code](#access)
1. [Reporting bugs](#bugs)
1. [Contributing code](#contrib)

### Introduction

Thank you for using BIND 9!

BIND is open source software that implements the Domain Name System (DNS)
protocols for the Internet. It is a reference implementation of those
protocols, but it is also production-grade software, suitable for use in
high-volume and high-reliability applications.  It is very
widely used DNS software, providing a robust and stable platform on top of
which organizations can build distributed computing systems with the
knowledge that those systems are fully compliant with published DNS
standards.

BIND is and will always remain free and openly available.  It can be
used and modified in any way by anyone.

BIND is maintained by [Internet Systems Consortium](https://www.isc.org),
a public-benefit 501(c)(3) nonprofit, using a "managed open source" approach:
anyone can see the source, but only ISC employees have commit access.
In the past, the source could only be seen once ISC had published
a release; read access to the source repository was restricted just
as commit access was.  That has changed, as ISC now provides a
public git repository of the BIND source tree (see below).

At ISC, we're committed to
building communities that are welcoming and inclusive: environments where people
are encouraged to share ideas, treat each other with respect, and collaborate
towards the best solutions. To reinforce our commitment, ISC
has adopted a slightly modified version of the Django
[Code of Conduct](https://gitlab.isc.org/isc-projects/bind9/-/blob/main/CODE_OF_CONDUCT.md)
for the BIND 9 project, as well as for the conduct of our developers throughout
the industry.

### <a name="access"></a>Access to source code

Public BIND releases are always available from the
[ISC FTP site](ftp://ftp.isc.org/isc/bind9).

A public-access git repository is also available at
[https://gitlab.isc.org](https://gitlab.isc.org).  This repository
contains all public release branches. Upcoming releases can be viewed in
their current state at any time.  Short-lived development branches
contain unreviewed work in progress.  Commits which address security
vulnerablilities are withheld until after public disclosure.

You can browse the source online via
[https://gitlab.isc.org/isc-projects/bind9](https://gitlab.isc.org/isc-projects/bind9)

To clone the repository, use:

>       $ git clone https://gitlab.isc.org/isc-projects/bind9.git

Release branch names are of the form `bind-9.X`, where X represents the second
number in the BIND 9 version number.  So, to check out the BIND 9.18
branch, use:

>       $ git checkout bind-9.18

Whenever a branch is ready for publication, a tag is placed of the
form `v9.X.Y`.  The 9.18.0 release, for instance, is tagged as `v9.18.0`.

The branch in which the next major release is being developed is called
`main`.

### <a name="bugs"></a>Reporting bugs

Reports of flaws in the BIND package, including software bugs, errors
in the documentation, missing files in the tarball, suggested changes
or requests for new features, etc., can be filed using
[https://gitlab.isc.org/isc-projects/bind9/issues](https://gitlab.isc.org/isc-projects/bind9/issues).

Due to a large ticket backlog, we are sometimes slow to respond,
especially if a bug is cosmetic or if a feature request is vague or
low in priority, but we try at least to acknowledge legitimate
bug reports within a week.

ISC's GitLab system is publicly readable; however, you must have
an account to create a new issue. You can either register locally or
use credentials from an existing account at GitHub, GitLab, Google,
Twitter, or Facebook.

### Reporting possible security issues

See `SECURITY.md`.

### <a name="contrib"></a>Contributing code

BIND is licensed under the
[Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/).
Earlier versions (BIND 9.10 and earlier) were licensed under the
[ISC License](https://www.isc.org/licenses/)

ISC does not require an explicit copyright assignment for patch
contributions.  However, by submitting a patch to ISC, you implicitly
certify that you are the author of the code, that you intend to relinquish
exclusive copyright, and that you grant permission to publish your work
under the open source license used for the BIND version(s) to which your
patch will be applied.

#### <a name="bind"></a>BIND code

Patches for BIND may be submitted directly via merge requests in
[ISC's GitLab](https://gitlab.isc.org/isc-projects/bind9/) source
repository for BIND.

Patches can also be submitted as diffs against a specific version of
BIND -- preferably the current top of the `main` branch.  Diffs may
be generated using either `git format-patch` or `git diff`.

Those wanting to write code for BIND may be interested in the
[developer information](doc/dev/dev.md) page, which includes information
about BIND design and coding practices, including discussion of internal
APIs and overall system architecture.

Every patch submitted is reviewed by ISC engineers following our
[code review process](doc/dev/dev.md#reviews) before it is merged.

It may take considerable time to review patch submissions, especially if
they don't meet ISC style and quality guidelines.  If a patch is a good
idea, we can and will do additional work to bring it up to par, but if
we're busy with other work, it may take us a long time to get to it.

To ensure your patch is acted on as promptly as possible, please:

* Try to adhere to the [BIND 9 coding style](doc/dev/style.md).
* Run `make check` to ensure your change hasn't caused any
  functional regressions.
* Document your work, both in the patch itself and in the
  accompanying email.
* In patches that make non-trivial functional changes, include system
  tests if possible; when introducing or substantially altering a
  library API, include unit tests. See [Testing](doc/dev/dev.md#testing)
  for more information.

##### Changes to `configure`

If you need to make changes to `configure`, you should not edit it
directly; instead, edit `configure.in`, then run `autoconf`.  Similarly,
instead of editing `config.h.in` directly, edit `configure.in` and run
`autoheader`.

When submitting a patch as a diff, it's fine to omit the `configure`
diffs to save space.  Just send the `configure.in` diffs and we'll
generate the new `configure` during the review process.

##### Documentation

All functional changes should be documented. There are three types
of documentation in the BIND source tree:

* Man pages are kept alongside the source code for the commands
  they document, in files ending in `.rst`: for example, the
  `named` man page is `bin/named/named.rst`.
* The *BIND 9 Administrator Reference Manual* is in the .rst files in
  `doc/arm/`; the PDF and HTML versions are automatically generated from
  the `.rst` files.
* API documentation is in the header file describing the API, in
  Doxygen-formatted comments.

Patches to improve existing documentation are also very welcome!

##### Tests

BIND is a large and complex project. We rely heavily on continuous
automated testing and cannot merge new code without adequate test coverage.
Please see [the "Testing" section of doc/dev/dev.md](doc/dev/dev.md#testing)
for more information.

#### Thanks

Thank you for your interest in contributing to the ongoing development
of BIND 9.
