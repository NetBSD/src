.. raw:: html

  <style type="text/css">
    .none { background-color: #FFCCCC }
    .partial { background-color: #FFFF99 }
    .good { background-color: #CCFF99 }
  </style>

.. role:: none
.. role:: partial
.. role:: good

==================
MSVC compatibility
==================

When Clang compiles C++ code for Windows, it attempts to be compatible with
MSVC.  There are multiple dimensions to compatibility.

First, Clang attempts to be ABI-compatible, meaning that Clang-compiled code
should be able to link against MSVC-compiled code successfully.  However, C++
ABIs are particular large and complicated, and Clang's support for MSVC's C++
ABI is a work in progress.  If you don't require MSVC ABI compatibility or don't
want to use Microsoft's C and C++ runtimes, the mingw32 toolchain might be a
better fit for your project.

Second, Clang implements many MSVC language extensions, such as
``__declspec(dllexport)`` and a handful of pragmas.  These are typically
controlled by ``-fms-extensions``.

Finally, MSVC accepts some C++ code that Clang will typically diagnose as
invalid.  When these constructs are present in widely included system headers,
Clang attempts to recover and continue compiling the user's program.  Most
parsing and semantic compatibility tweaks are controlled by
``-fms-compatibility`` and ``-fdelayed-template-parsing``, and they are a work
in progress.

ABI features
============

The status of major ABI-impacting C++ features:

* Record layout: :good:`Mostly complete`.  We've attacked this with a fuzzer,
  and most of the remaining failures involve ``#pragma pack``,
  ``__declspec(align(N))``, or other pragmas.

* Class inheritance: :good:`Mostly complete`.  This covers all of the standard
  OO features you would expect: virtual method inheritance, multiple
  inheritance, and virtual inheritance.  Every so often we uncover a bug where
  our tables are incompatible, but this is pretty well in hand.

* Name mangling: :good:`Ongoing`.  Every new C++ feature generally needs its own
  mangling.  For example, member pointer template arguments have an interesting
  and distinct mangling.  Fortunately, incorrect manglings usually do not result
  in runtime errors.  Non-inline functions with incorrect manglings usually
  result in link errors, which are relatively easy to diagnose.  Incorrect
  manglings for inline functions and templates result in multiple copies in the
  final image.  The C++ standard requires that those addresses be equal, but few
  programs rely on this.

* Member pointers: :good:`Mostly complete`.  Standard C++ member pointers are
  fully implemented and should be ABI compatible.  Both `#pragma
  pointers_to_members`_ and the `/vm`_ flags are supported. However, MSVC
  supports an extension to allow creating a `pointer to a member of a virtual
  base class`_.  Clang does not yet support this.

.. _#pragma pointers_to_members:
  http://msdn.microsoft.com/en-us/library/83cch5a6.aspx
.. _/vm: http://msdn.microsoft.com/en-us/library/yad46a6z.aspx
.. _pointer to a member of a virtual base class: http://llvm.org/PR15713

* Debug info: :partial:`Minimal`.  Clang emits CodeView line tables into the
  object file, similar to what MSVC emits when given the ``/Z7`` flag.
  Microsoft's link.exe will read this information and use it to create a PDB,
  enabling stack traces in all modern Windows debuggers.  Clang does not emit
  any type info or description of variable layout.

* `RTTI`_: :none:`Unstarted`.  See the bug for a discussion of what needs to
  happen first.

.. _RTTI: http://llvm.org/PR18951

* Exceptions and SEH: :none:`Unstarted`.  Clang can parse both constructs, but
  does not know how to emit compatible handlers.  This depends on RTTI.

* Thread-safe initialization of local statics: :none:`Unstarted`.  We are ABI
  compatible with MSVC 2012, which does not support thread-safe local statics.
  MSVC 2013 changed the ABI to make initialization of local statics thread safe,
  and we have not yet implemented this.

* Lambdas in ABI boundaries: :none:`Infeasible`.  It is unlikely that we will
  ever be fully ABI compatible with lambdas declared in inline functions due to
  what appears to be a hash code in the name mangling.  Lambdas that are not
  externally visible should work fine.

Template instantiation and name lookup
======================================

In addition to the usual `dependent name lookup FAQs `_, Clang is often unable
to parse certain invalid C++ constructs that MSVC allows.  As of this writing,
Clang will reject code with missing ``typename`` annotations:

.. _dependent name lookup FAQs:
  http://clang.llvm.org/compatibility.html#dep_lookup

.. code-block:: c++

  struct X {
    typedef int type;
  };
  template<typename T> int f() {
    // missing typename keyword
    return sizeof(/*typename*/ T::type);
  }
  template void f<X>();

Accepting code like this is ongoing work.  Ultimately, it may be cleaner to
`implement a token-based template instantiation mode`_ than it is to add
compatibility hacks to the existing AST-based instantiation.

.. _implement a token-based template instantiation mode: http://llvm.org/PR18714
