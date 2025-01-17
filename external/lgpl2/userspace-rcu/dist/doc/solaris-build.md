<!--
SPDX-FileCopyrightText: 2023 EfficiOS Inc.

SPDX-License-Identifier: CC-BY-4.0
-->

# Solaris support
## Solaris 10
### Dependencies
On Solaris 10, the OpenCSW open source software distribution is required for build and runtime dependencies. The following base packages are required :

* SUNWtoo
* SUNWbtool
* SUNWhea
* SUNWarc
* SUNWarcr
* SUNWlibmr
* SUNWlibm

And the following OpenCSW packages are required :
* automake
* autoconf
* libtool
* bison
* flex
* gsed
* gmake
* pkgconfig
* libglib2\_dev
* gcc4core

### Build

Add CSW and CCS to PATH :
```
export PATH="/opt/csw/bin:/usr/ccs/bin:$PATH"
```

Build with gmake :
```
./bootstrap
CFLAGS="-D_XOPEN_SOURCE=1 -D_XOPEN_SOURCE_EXTENDED=1 -D__EXTENSIONS__=1" MAKE=gmake ./configure
gmake
gmake check
gmake regtest
gmake install
```

## Solaris 11
### Dependencies
On Solaris 11, the following base packages are required :
* autoconf
* automake
* gnu-make
* libtool
* gcc
* flex
* bison

### Build
Add Perl5 to PATH :
```
export PATH="$PATH:/usr/perl5/bin"
```

Build with gmake :
```
./bootstrap
CFLAGS="-D_XOPEN_SOURCE=1 -D_XOPEN_SOURCE_EXTENDED=1 -D__EXTENSIONS__=1" MAKE=gmake ./configure --prefix=$PREFIX
gmake
gmake check
gmake regtest
gmake install
```

