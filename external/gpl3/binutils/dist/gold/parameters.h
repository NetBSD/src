// parameters.h -- general parameters for a link using gold  -*- C++ -*-

// Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#ifndef GOLD_PARAMETERS_H
#define GOLD_PARAMETERS_H

namespace gold
{

class General_options;
class Errors;
class Target;

// Here we define the Parameters class which simply holds simple
// general parameters which apply to the entire link.  We use a global
// variable for this.  The parameters class holds three types of data:
//    1) An Errors struct.  Any part of the code that wants to log an
//       error can use parameters->errors().
//    2) A const General_options.  These are the options as read on
//       the commandline.
//    3) Target information, such as size and endian-ness.  This is
//       available as soon as we've decided on the Target (after
//       parsing the first .o file).
//    4) Whether we're doing a static link or not.  This is set
//       after all inputs have been read and we know if any is a
//       dynamic library.

class Parameters
{
 public:
  Parameters()
    : errors_(NULL), options_(NULL), target_(NULL),
      doing_static_link_valid_(false), doing_static_link_(false),
      debug_(0)
  { }

  // These should be called as soon as they are known.
  void
  set_errors(Errors* errors);

  void
  set_options(const General_options* options);

  void
  set_target(const Target* target);

  void
  set_doing_static_link(bool doing_static_link);

  // Return the error object.
  Errors*
  errors() const
  { return this->errors_; }

  // Whether the options are valid.  This should not normally be
  // called, but it is needed by gold_exit.
  bool
  options_valid() const
  { return this->options_ != NULL; }

  // Return the options object.
  const General_options&
  options() const
  {
    gold_assert(this->options_valid());
    return *this->options_;
  }

  // Return whether the target field has been set.
  bool
  target_valid() const
  { return this->target_ != NULL; }

  // The target of the output file we are generating.
  const Target&
  target() const
  {
    gold_assert(this->target_valid());
    return *this->target_;
  }

  // When we don't have an output file to associate a target, make a
  // default one, with guesses about size and endianness.
  const Target&
  default_target() const;

  bool
  doing_static_link() const
  {
    gold_assert(this->doing_static_link_valid_);
    return this->doing_static_link_;
  }

  // This is just a copy of options().debug().  We make a copy so we
  // don't have to #include options.h in order to inline
  // is_debugging_enabled, below.
  int
  debug() const
  {
    // This can be called before the options are set up.
    if (!this->options_valid())
      return 0;
    return debug_;
  }

  // A convenience routine for combining size and endianness.  It also
  // checks the HAVE_TARGET_FOO configure options and dies if the
  // current target's size/endianness is not supported according to
  // HAVE_TARGET_FOO.  Otherwise it returns this enum
  enum Target_size_endianness
  { TARGET_32_LITTLE, TARGET_32_BIG, TARGET_64_LITTLE, TARGET_64_BIG };

  Target_size_endianness
  size_and_endianness() const;


 private:
  Errors* errors_;
  const General_options* options_;
  const Target* target_;
  bool doing_static_link_valid_;
  bool doing_static_link_;
  int debug_;
};

// This is a global variable.
extern const Parameters* parameters;

// We use free functions for these since they affect a global variable
// that is internal to parameters.cc.

extern void
set_parameters_errors(Errors* errors);

extern void
set_parameters_options(const General_options* options);

extern void
set_parameters_target(const Target* target);

extern void
set_parameters_doing_static_link(bool doing_static_link);
  
// Return whether we are doing a particular debugging type.  The
// argument is one of the flags from debug.h.

inline bool
is_debugging_enabled(unsigned int type)
{ return (parameters->debug() & type) != 0; }

} // End namespace gold.

#endif // !defined(GOLD_PARAMETERS_H)
