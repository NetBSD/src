NetBSD
------

NetBSD is a complete operating system.  
It supports a wide range of machines from recent aarch64 and amd64
machines to VAX and M68k.

Building
--------

Cross-building is possible from most UNIX-like operating systems.  
To build for amd64 (x86_64), in the src directory:  
    ./build.sh -U -u -j4 -m amd64 -O ~/obj release

Additional build information available in the [BUILDING](BUILDING) file.

Binaries
--------

- [Daily builds](https://nycdn.netbsd.org/pub/NetBSD-daily/HEAD/latest/)  
- [Releases](https://cdn.netbsd.org/pub/NetBSD/)

Testing
-------

On a running NetBSD system  
    cd /usr/tests; atf-run | atf-report

Troubleshooting
---------------

- Bugs and patches can be sent [via web form](https://www.netbsd.org/cgi-bin/sendpr.cgi?gndb=netbsd).  
- Several [mailing lists](https://www.netbsd.org/mailinglists/) exist.  
  The [netbsd-users](https://netbsd.org/mailinglists/#netbsd-users) list is a good choice for many problems.  
- A community IRC channel exist on [#netbsd @ freenode](https://webchat.freenode.net/?channels=#netbsd)

Latest sources
--------------

To fetch the main CVS repository:  
    cvs -d anoncvs@anoncvs.NetBSD.org:/cvsroot checkout -P src

Additional Links
----------------

[The NetBSD Guide](https://www.netbsd.org/docs/guide/en/)
[NetBSD manual pages](http://man.netbsd.org/)
[NetBSD Cross-Reference](https://nxr.netbsd.org/)
