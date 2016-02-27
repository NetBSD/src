==========================
UndefinedBehaviorSanitizer
==========================

.. contents::
   :local:

Introduction
============

UndefinedBehaviorSanitizer (UBSan) is a fast undefined behavior detector.
UBSan modifies the program at compile-time to catch various kinds of undefined
behavior during program execution, for example:

* Using misaligned or null pointer
* Signed integer overflow
* Conversion to, from, or between floating-point types which would
  overflow the destination

See the full list of available :ref:`checks <ubsan-checks>` below.

UBSan has an optional run-time library which provides better error reporting.
The checks have small runtime cost and no impact on address space layout or ABI.

How to build
============

Build LLVM/Clang with `CMake <http://llvm.org/docs/CMake.html>`_.

Usage
=====

Use ``clang++`` to compile and link your program with ``-fsanitize=undefined``
flag. Make sure to use ``clang++`` (not ``ld``) as a linker, so that your
executable is linked with proper UBSan runtime libraries. You can use ``clang``
instead of ``clang++`` if you're compiling/linking C code.

.. code-block:: console

  % cat test.cc
  int main(int argc, char **argv) {
    int k = 0x7fffffff;
    k += argc;
    return 0;
  }
  % clang++ -fsanitize=undefined test.cc
  % ./a.out
  test.cc:3:5: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'

You can enable only a subset of :ref:`checks <ubsan-checks>` offered by UBSan,
and define the desired behavior for each kind of check:

* print a verbose error report and continue execution (default);
* print a verbose error report and exit the program;
* execute a trap instruction (doesn't require UBSan run-time support).

For example if you compile/link your program as:

.. code-block:: console

  % clang++ -fsanitize=signed-integer-overflow,null,alignment -fno-sanitize-recover=null -fsanitize-trap=alignment

the program will continue execution after signed integer overflows, exit after
the first invalid use of a null pointer, and trap after the first use of misaligned
pointer.

.. _ubsan-checks:

Availablle checks
=================

Available checks are:

  -  ``-fsanitize=alignment``: Use of a misaligned pointer or creation
     of a misaligned reference.
  -  ``-fsanitize=bool``: Load of a ``bool`` value which is neither
     ``true`` nor ``false``.
  -  ``-fsanitize=bounds``: Out of bounds array indexing, in cases
     where the array bound can be statically determined.
  -  ``-fsanitize=enum``: Load of a value of an enumerated type which
     is not in the range of representable values for that enumerated
     type.
  -  ``-fsanitize=float-cast-overflow``: Conversion to, from, or
     between floating-point types which would overflow the
     destination.
  -  ``-fsanitize=float-divide-by-zero``: Floating point division by
     zero.
  -  ``-fsanitize=function``: Indirect call of a function through a
     function pointer of the wrong type (Linux, C++ and x86/x86_64 only).
  -  ``-fsanitize=integer-divide-by-zero``: Integer division by zero.
  -  ``-fsanitize=nonnull-attribute``: Passing null pointer as a function
     parameter which is declared to never be null.
  -  ``-fsanitize=null``: Use of a null pointer or creation of a null
     reference.
  -  ``-fsanitize=object-size``: An attempt to use bytes which the
     optimizer can determine are not part of the object being
     accessed. The sizes of objects are determined using
     ``__builtin_object_size``, and consequently may be able to detect
     more problems at higher optimization levels.
  -  ``-fsanitize=return``: In C++, reaching the end of a
     value-returning function without returning a value.
  -  ``-fsanitize=returns-nonnull-attribute``: Returning null pointer
     from a function which is declared to never return null.
  -  ``-fsanitize=shift``: Shift operators where the amount shifted is
     greater or equal to the promoted bit-width of the left hand side
     or less than zero, or where the left hand side is negative. For a
     signed left shift, also checks for signed overflow in C, and for
     unsigned overflow in C++. You can use ``-fsanitize=shift-base`` or
     ``-fsanitize=shift-exponent`` to check only left-hand side or
     right-hand side of shift operation, respectively.
  -  ``-fsanitize=signed-integer-overflow``: Signed integer overflow,
     including all the checks added by ``-ftrapv``, and checking for
     overflow in signed division (``INT_MIN / -1``).
  -  ``-fsanitize=unreachable``: If control flow reaches
     ``__builtin_unreachable``.
  -  ``-fsanitize=unsigned-integer-overflow``: Unsigned integer
     overflows.
  -  ``-fsanitize=vla-bound``: A variable-length array whose bound
     does not evaluate to a positive value.
  -  ``-fsanitize=vptr``: Use of an object whose vptr indicates that
     it is of the wrong dynamic type, or that its lifetime has not
     begun or has ended. Incompatible with ``-fno-rtti``. Link must
     be performed by ``clang++``, not ``clang``, to make sure C++-specific
     parts of the runtime library and C++ standard libraries are present.

You can also use the following check groups:
  -  ``-fsanitize=undefined``: All of the checks listed above other than
     ``unsigned-integer-overflow``.
  -  ``-fsanitize=undefined-trap``: Deprecated alias of
     ``-fsanitize=undefined``.
  -  ``-fsanitize=integer``: Checks for undefined or suspicious integer
     behavior (e.g. unsigned integer overflow).

Stack traces and report symbolization
=====================================
If you want UBSan to print symbolized stack trace for each error report, you
will need to:

#. Compile with ``-g`` and ``-fno-omit-frame-pointer`` to get proper debug
   information in your binary.
#. Run your program with environment variable
   ``UBSAN_OPTIONS=print_stacktrace=1``.
#. Make sure ``llvm-symbolizer`` binary is in ``PATH``.

Issue Suppression
=================

UndefinedBehaviorSanitizer is not expected to produce false positives.
If you see one, look again; most likely it is a true positive!

Disabling Instrumentation with ``__attribute__((no_sanitize("undefined")))``
----------------------------------------------------------------------------

You disable UBSan checks for particular functions with
``__attribute__((no_sanitize("undefined")))``. You can use all values of
``-fsanitize=`` flag in this attribute, e.g. if your function deliberately
contains possible signed integer overflow, you can use
``__attribute__((no_sanitize("signed-integer-overflow")))``.

This attribute may not be
supported by other compilers, so consider using it together with
``#if defined(__clang__)``.

Suppressing Errors in Recompiled Code (Blacklist)
-------------------------------------------------

UndefinedBehaviorSanitizer supports ``src`` and ``fun`` entity types in
:doc:`SanitizerSpecialCaseList`, that can be used to suppress error reports
in the specified source files or functions.

Runtime suppressions
--------------------

Sometimes you can suppress UBSan error reports for specific files, functions,
or libraries without recompiling the code. You need to pass a path to
suppression file in a ``UBSAN_OPTIONS`` environment variable.

.. code-block:: bash

    UBSAN_OPTIONS=suppressions=MyUBSan.supp

You need to specify a :ref:`check <ubsan-checks>` you are suppressing and the
bug location. For example:

.. code-block:: bash

  signed-integer-overflow:file-with-known-overflow.cpp
  alignment:function_doing_unaligned_access
  vptr:shared_object_with_vptr_failures.so

There are several limitations:

* Sometimes your binary must have enough debug info and/or symbol table, so
  that the runtime could figure out source file or function name to match
  against the suppression.
* It is only possible to suppress recoverable checks. For the example above,
  you can additionally pass
  ``-fsanitize-recover=signed-integer-overflow,alignment,vptr``, although
  most of UBSan checks are recoverable by default.
* Check groups (like ``undefined``) can't be used in suppressions file, only
  fine-grained checks are supported.

Supported Platforms
===================

UndefinedBehaviorSanitizer is supported on the following OS:

* Android
* Linux
* FreeBSD
* OS X 10.6 onwards

and for the following architectures:

* i386/x86\_64
* ARM
* AArch64
* PowerPC64
* MIPS/MIPS64

Current Status
==============

UndefinedBehaviorSanitizer is available on selected platforms starting from LLVM
3.3. The test suite is integrated into the CMake build and can be run with
``check-ubsan`` command.

More Information
================

* From LLVM project blog:
  `What Every C Programmer Should Know About Undefined Behavior
  <http://blog.llvm.org/2011/05/what-every-c-programmer-should-know.html>`_
* From John Regehr's *Embedded in Academia* blog:
  `A Guide to Undefined Behavior in C and C++
  <http://blog.regehr.org/archives/213>`_
