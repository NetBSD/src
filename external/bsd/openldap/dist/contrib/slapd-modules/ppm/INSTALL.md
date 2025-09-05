INSTALLATION
============

Dependencies
------------------
ppm is provided along with OpenLDAP sources. By default, it is available into contrib/slapd-modules.
 - make sure both OpenLDAP sources and ppm are available for building.
 - install cracklib development files if you want to test passwords against cracklib
 - install pandoc if you want to build the man page


Build
-----
Enter contrib/slapd-modules/ppm directory

You can optionally customize some variables if you don't want the default ones:
- prefix: prefix of the path where ppm is to be installed (defaults to /usr/local)
- ldap_subdir: OpenLDAP specific subdirectory for modules and configurations (defaults to  openldap )
- moduledir: where the ppm module is to be deployed (defaults to $prefix/$libexecdir/$ldap_subdir)
- etcdir: used to compose default sysconfdir location (defaults to $prefix/etc)
- sysconfdir: where the ppm example policy is to be deployed (defaults to $prefix/$etcdir/$ldap_subdir)
- LDAP_SRC: path to OpenLDAP source directory
- Options in DEFS variable:
    CONFIG_FILE: (DEPRECATED) path to a ppm configuration file (see PPM_READ_FILE in ppm.h)
        note: ppm configuration now lies into pwdCheckModuleArg password policy attribute
              provided example file is only helpful as an example or for testing
    CRACKLIB: if defined, link against cracklib
    DEBUG: If defined, ppm logs its actions with syslog


To build ppm, simply run these commands:
(based upon the default prefix /usr/local of OpenLDAP)

```
make clean
make
make test
make doc
make install
```

Here is an illustrative example showing how to overload some options:

```
make clean
make LDAP_SRC=../../.. prefix=/usr/local libdir=/usr/local/lib
make test LDAP_SRC=../../..
make doc prefix=/usr/local
make install prefix=/usr/local libdir=/usr/local/lib
```

