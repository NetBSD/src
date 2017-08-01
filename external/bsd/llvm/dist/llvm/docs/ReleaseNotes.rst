========================
LLVM 5.0.0 Release Notes
========================

.. contents::
    :local:

.. warning::
   These are in-progress notes for the upcoming LLVM 5 release.
   Release notes for previous releases can be found on
   `the Download Page <http://releases.llvm.org/download.html>`_.


Introduction
============

This document contains the release notes for the LLVM Compiler Infrastructure,
release 5.0.0.  Here we describe the status of LLVM, including major improvements
from the previous release, improvements in various subprojects of LLVM, and
some of the current users of the code.  All LLVM releases may be downloaded
from the `LLVM releases web site <http://llvm.org/releases/>`_.

For more information about LLVM, including information about the latest
release, please check out the `main LLVM web site <http://llvm.org/>`_.  If you
have questions or comments, the `LLVM Developer's Mailing List
<http://lists.llvm.org/mailman/listinfo/llvm-dev>`_ is a good place to send
them.

Note that if you are reading this file from a Subversion checkout or the main
LLVM web page, this document applies to the *next* release, not the current
one.  To see the release notes for a specific release, please see the `releases
page <http://llvm.org/releases/>`_.

Non-comprehensive list of changes in this release
=================================================
.. NOTE
   For small 1-3 sentence descriptions, just add an entry at the end of
   this list. If your description won't fit comfortably in one bullet
   point (e.g. maybe you would like to give an example of the
   functionality, or simply have a lot to talk about), see the `NOTE` below
   for adding a new subsection.

* LLVM's ``WeakVH`` has been renamed to ``WeakTrackingVH`` and a new ``WeakVH``
  has been introduced.  The new ``WeakVH`` nulls itself out on deletion, but
  does not track values across RAUW.
  
* A new library named ``BinaryFormat`` has been created which holds a collection
  of code which previously lived in ``Support``.  This includes the
  ``file_magic`` structure and ``identify_magic`` functions, as well as all the
  structure and type definitions for DWARF, ELF, COFF, WASM, and MachO file
  formats.
  
* The tool ``llvm-pdbdump`` has been renamed ``llvm-pdbutil`` to better reflect
  its nature as a general purpose PDB manipulation / diagnostics tool that does
  more than just dumping contents.
  
* The ``BBVectorize`` pass has been removed. It was fully replaced and no
  longer used back in 2014 but we didn't get around to removing it. Now it is
  gone. The SLP vectorizer is the suggested non-loop vectorization pass.

.. NOTE
   If you would like to document a larger change, then you can add a
   subsection about it right here. You can copy the following boilerplate
   and un-indent it (the indentation causes it to be inside this comment).

   Special New Feature
   -------------------

   Makes programs 10x faster by doing Special New Thing.

Changes to the LLVM IR
----------------------

* The datalayout string may now indicate an address space to use for
  the pointer type of alloca rather than the default of 0.

* Added speculatable attribute indicating a function which does has no
  side-effects which could inhibit hoisting of calls.

Changes to the ARM Backend
--------------------------

 During this release ...


Changes to the MIPS Target
--------------------------

 During this release ...


Changes to the PowerPC Target
-----------------------------

 During this release ...

Changes to the X86 Target
-------------------------

* Added initial AMD Ryzen (znver1) scheduler support.

* Added support for Intel Goldmont CPUs.

* Add support for avx512vpopcntdq instructions.

* Added heuristics to convert CMOV into branches when it may be profitable.

* More aggressive inlining of memcmp calls.

* Improve vXi64 shuffles on 32-bit targets.

* Improved use of PMOVMSKB for any_of/all_of comparision reductions.

* Improved Silvermont, Sandybridge, and Jaguar (btver2) schedulers.

* Improved support for AVX512 vector rotations.

* Added support for AMD Lightweight Profiling (LWP) instructions.

Changes to the AMDGPU Target
-----------------------------

* Initial gfx9 support

Changes to the AVR Target
-----------------------------

This release consists mainly of bugfixes and implementations of features
required for compiling basic Rust programs.

* Enable the branch relaxation pass so that we don't crash on large
  stack load/stores

* Add support for lowering bit-rotations to the native `ror` and `rol`
  instructions

* Fix bug where function pointers were treated as pointers to RAM and not
  pointers to program memory

* Fix broken code generaton for shift-by-variable expressions

* Support zero-sized types in argument lists; this is impossible in C,
  but possible in Rust

Changes to the OCaml bindings
-----------------------------

 During this release ...


Changes to the C API
--------------------

* Deprecated the ``LLVMAddBBVectorizePass`` interface since the ``BBVectorize``
  pass has been removed. It is now a no-op and will be removed in the next
  release. Use ``LLVMAddSLPVectorizePass`` instead to get the supported SLP
  vectorizer.


External Open Source Projects Using LLVM 5
==========================================

* A project...


Additional Information
======================

A wide variety of additional information is available on the `LLVM web page
<http://llvm.org/>`_, in particular in the `documentation
<http://llvm.org/docs/>`_ section.  The web page also contains versions of the
API documentation which is up-to-date with the Subversion version of the source
code.  You can access versions of these documents specific to this release by
going into the ``llvm/docs/`` directory in the LLVM tree.

If you have any questions or comments about LLVM, please feel free to contact
us via the `mailing lists <http://llvm.org/docs/#maillist>`_.
