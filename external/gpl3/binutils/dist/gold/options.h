// options.h -- handle command line options for gold  -*- C++ -*-

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

// General_options (from Command_line::options())
//   All the options (a.k.a. command-line flags)
// Input_argument (from Command_line::inputs())
//   The list of input files, including -l options.
// Command_line
//   Everything we get from the command line -- the General_options
//   plus the Input_arguments.
//
// There are also some smaller classes, such as
// Position_dependent_options which hold a subset of General_options
// that change as options are parsed (as opposed to the usual behavior
// of the last instance of that option specified on the commandline wins).

#ifndef GOLD_OPTIONS_H
#define GOLD_OPTIONS_H

#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#include "elfcpp.h"
#include "script.h"

namespace gold
{

class Command_line;
class General_options;
class Search_directory;
class Input_file_group;
class Position_dependent_options;
class Target;

// The nested namespace is to contain all the global variables and
// structs that need to be defined in the .h file, but do not need to
// be used outside this class.
namespace options
{
typedef std::vector<Search_directory> Dir_list;
typedef Unordered_set<std::string> String_set;

// These routines convert from a string option to various types.
// Each gives a fatal error if it cannot parse the argument.

extern void
parse_bool(const char* option_name, const char* arg, bool* retval);

extern void
parse_uint(const char* option_name, const char* arg, int* retval);

extern void
parse_uint64(const char* option_name, const char* arg, uint64_t* retval);

extern void
parse_double(const char* option_name, const char* arg, double* retval);

extern void
parse_string(const char* option_name, const char* arg, const char** retval);

extern void
parse_optional_string(const char* option_name, const char* arg,
		      const char** retval);

extern void
parse_dirlist(const char* option_name, const char* arg, Dir_list* retval);

extern void
parse_set(const char* option_name, const char* arg, String_set* retval);

extern void
parse_choices(const char* option_name, const char* arg, const char** retval,
              const char* choices[], int num_choices);

struct Struct_var;

// Most options have both a shortname (one letter) and a longname.
// This enum controls how many dashes are expected for longname access
// -- shortnames always use one dash.  Most longnames will accept
// either one dash or two; the only difference between ONE_DASH and
// TWO_DASHES is how we print the option in --help.  However, some
// longnames require two dashes, and some require only one.  The
// special value DASH_Z means that the option is preceded by "-z".
enum Dashes
{
  ONE_DASH, TWO_DASHES, EXACTLY_ONE_DASH, EXACTLY_TWO_DASHES, DASH_Z
};

// LONGNAME is the long-name of the option with dashes converted to
//    underscores, or else the short-name if the option has no long-name.
//    It is never the empty string.
// DASHES is an instance of the Dashes enum: ONE_DASH, TWO_DASHES, etc.
// SHORTNAME is the short-name of the option, as a char, or '\0' if the
//    option has no short-name.  If the option has no long-name, you
//    should specify the short-name in *both* VARNAME and here.
// DEFAULT_VALUE is the value of the option if not specified on the
//    commandline, as a string.
// HELPSTRING is the descriptive text used with the option via --help
// HELPARG is how you define the argument to the option.
//    --help output is "-shortname HELPARG, --longname HELPARG: HELPSTRING"
//    HELPARG should be NULL iff the option is a bool and takes no arg.
// OPTIONAL_ARG is true if this option takes an optional argument.  An
//    optional argument must be specifid as --OPTION=VALUE, not
//    --OPTION VALUE.
// READER provides parse_to_value, which is a function that will convert
//    a char* argument into the proper type and store it in some variable.
// A One_option struct initializes itself with the global list of options
// at constructor time, so be careful making one of these.
struct One_option
{
  std::string longname;
  Dashes dashes;
  char shortname;
  const char* default_value;
  const char* helpstring;
  const char* helparg;
  bool optional_arg;
  Struct_var* reader;

  One_option(const char* ln, Dashes d, char sn, const char* dv,
             const char* hs, const char* ha, bool oa, Struct_var* r)
    : longname(ln), dashes(d), shortname(sn), default_value(dv ? dv : ""),
      helpstring(hs), helparg(ha), optional_arg(oa), reader(r)
  {
    // In longname, we convert all underscores to dashes, since GNU
    // style uses dashes in option names.  longname is likely to have
    // underscores in it because it's also used to declare a C++
    // function.
    const char* pos = strchr(this->longname.c_str(), '_');
    for (; pos; pos = strchr(pos, '_'))
      this->longname[pos - this->longname.c_str()] = '-';

    // We only register ourselves if our helpstring is not NULL.  This
    // is to support the "no-VAR" boolean variables, which we
    // conditionally turn on by defining "no-VAR" help text.
    if (this->helpstring)
      this->register_option();
  }

  // This option takes an argument iff helparg is not NULL.
  bool
  takes_argument() const
  { return this->helparg != NULL; }

  // Whether the argument is optional.
  bool
  takes_optional_argument() const
  { return this->optional_arg; }

  // Register this option with the global list of options.
  void
  register_option();

  // Print this option to stdout (used with --help).
  void
  print() const;
};

// All options have a Struct_##varname that inherits from this and
// actually implements parse_to_value for that option.
struct Struct_var
{
  // OPTION: the name of the option as specified on the commandline,
  //    including leading dashes, and any text following the option:
  //    "-O", "--defsym=mysym=0x1000", etc.
  // ARG: the arg associated with this option, or NULL if the option
  //    takes no argument: "2", "mysym=0x1000", etc.
  // CMDLINE: the global Command_line object.  Used by DEFINE_special.
  // OPTIONS: the global General_options object.  Used by DEFINE_special.
  virtual void
  parse_to_value(const char* option, const char* arg,
                 Command_line* cmdline, General_options* options) = 0;
  virtual
  ~Struct_var()  // To make gcc happy.
  { }
};

// This is for "special" options that aren't of any predefined type.
struct Struct_special : public Struct_var
{
  // If you change this, change the parse-fn in DEFINE_special as well.
  typedef void (General_options::*Parse_function)(const char*, const char*,
                                                  Command_line*);
  Struct_special(const char* varname, Dashes dashes, char shortname,
                 Parse_function parse_function,
                 const char* helpstring, const char* helparg)
    : option(varname, dashes, shortname, "", helpstring, helparg, false, this),
      parse(parse_function)
  { }

  void parse_to_value(const char* option, const char* arg,
                      Command_line* cmdline, General_options* options)
  { (options->*(this->parse))(option, arg, cmdline); }

  One_option option;
  Parse_function parse;
};

}  // End namespace options.


// These are helper macros use by DEFINE_uint64/etc below.
// This macro is used inside the General_options_ class, so defines
// var() and set_var() as General_options methods.  Arguments as are
// for the constructor for One_option.  param_type__ is the same as
// type__ for built-in types, and "const type__ &" otherwise.
#define DEFINE_var(varname__, dashes__, shortname__, default_value__,        \
                   default_value_as_string__, helpstring__, helparg__,       \
                   optional_arg__, type__, param_type__, parse_fn__)	     \
 public:                                                                     \
  param_type__                                                               \
  varname__() const                                                          \
  { return this->varname__##_.value; }                                       \
                                                                             \
  bool                                                                       \
  user_set_##varname__() const                                               \
  { return this->varname__##_.user_set_via_option; }                         \
                                                                             \
  void									     \
  set_user_set_##varname__()						     \
  { this->varname__##_.user_set_via_option = true; }			     \
									     \
 private:                                                                    \
  struct Struct_##varname__ : public options::Struct_var                     \
  {                                                                          \
    Struct_##varname__()                                                     \
      : option(#varname__, dashes__, shortname__, default_value_as_string__, \
               helpstring__, helparg__, optional_arg__, this),		     \
        user_set_via_option(false), value(default_value__)                   \
    { }                                                                      \
                                                                             \
    void                                                                     \
    parse_to_value(const char* option_name, const char* arg,                 \
                   Command_line*, General_options*)                          \
    {                                                                        \
      parse_fn__(option_name, arg, &this->value);                            \
      this->user_set_via_option = true;                                      \
    }                                                                        \
                                                                             \
    options::One_option option;                                              \
    bool user_set_via_option;                                                \
    type__ value;                                                            \
  };                                                                         \
  Struct_##varname__ varname__##_;                                           \
  void                                                                       \
  set_##varname__(param_type__ value)                                        \
  { this->varname__##_.value = value; }

// These macros allow for easy addition of a new commandline option.

// If no_helpstring__ is not NULL, then in addition to creating
// VARNAME, we also create an option called no-VARNAME (or, for a -z
// option, noVARNAME).
#define DEFINE_bool(varname__, dashes__, shortname__, default_value__,   \
                    helpstring__, no_helpstring__)                       \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,          \
             default_value__ ? "true" : "false", helpstring__, NULL,     \
             false, bool, bool, options::parse_bool)			 \
  struct Struct_no_##varname__ : public options::Struct_var              \
  {                                                                      \
    Struct_no_##varname__() : option((dashes__ == options::DASH_Z	 \
				      ? "no" #varname__			 \
				      : "no-" #varname__),		 \
				     dashes__, '\0',			 \
                                     default_value__ ? "false" : "true", \
                                     no_helpstring__, NULL, false, this) \
    { }                                                                  \
                                                                         \
    void                                                                 \
    parse_to_value(const char*, const char*,                             \
                   Command_line*, General_options* options)              \
    { options->set_##varname__(false); }                                 \
                                                                         \
    options::One_option option;                                          \
  };                                                                     \
  Struct_no_##varname__ no_##varname__##_initializer_

#define DEFINE_enable(varname__, dashes__, shortname__, default_value__, \
                      helpstring__, no_helpstring__)                     \
  DEFINE_var(enable_##varname__, dashes__, shortname__, default_value__, \
             default_value__ ? "true" : "false", helpstring__, NULL,     \
             false, bool, bool, options::parse_bool)			 \
  struct Struct_disable_##varname__ : public options::Struct_var         \
  {                                                                      \
    Struct_disable_##varname__() : option("disable-" #varname__,         \
                                     dashes__, '\0',                     \
                                     default_value__ ? "false" : "true", \
                                     no_helpstring__, NULL, false, this) \
    { }                                                                  \
                                                                         \
    void                                                                 \
    parse_to_value(const char*, const char*,                             \
                   Command_line*, General_options* options)              \
    { options->set_enable_##varname__(false); }                          \
                                                                         \
    options::One_option option;                                          \
  };                                                                     \
  Struct_disable_##varname__ disable_##varname__##_initializer_

#define DEFINE_uint(varname__, dashes__, shortname__, default_value__,  \
                   helpstring__, helparg__)                             \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,         \
             #default_value__, helpstring__, helparg__, false,		\
             int, int, options::parse_uint)

#define DEFINE_uint64(varname__, dashes__, shortname__, default_value__, \
                      helpstring__, helparg__)                           \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,          \
             #default_value__, helpstring__, helparg__, false,		 \
             uint64_t, uint64_t, options::parse_uint64)

#define DEFINE_double(varname__, dashes__, shortname__, default_value__, \
		      helpstring__, helparg__)				 \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,		 \
	     #default_value__, helpstring__, helparg__, false,		 \
	     double, double, options::parse_double)

#define DEFINE_string(varname__, dashes__, shortname__, default_value__, \
                      helpstring__, helparg__)                           \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,          \
             default_value__, helpstring__, helparg__, false,		 \
             const char*, const char*, options::parse_string)

// This is like DEFINE_string, but we convert each occurrence to a
// Search_directory and store it in a vector.  Thus we also have the
// add_to_VARNAME() method, to append to the vector.
#define DEFINE_dirlist(varname__, dashes__, shortname__,                  \
                           helpstring__, helparg__)                       \
  DEFINE_var(varname__, dashes__, shortname__, ,                          \
             "", helpstring__, helparg__, false, options::Dir_list,	  \
             const options::Dir_list&, options::parse_dirlist)            \
  void                                                                    \
  add_to_##varname__(const char* new_value)                               \
  { options::parse_dirlist(NULL, new_value, &this->varname__##_.value); } \
  void                                                                    \
  add_search_directory_to_##varname__(const Search_directory& dir)        \
  { this->varname__##_.value.push_back(dir); }

// This is like DEFINE_string, but we store a set of strings.
#define DEFINE_set(varname__, dashes__, shortname__,                      \
                   helpstring__, helparg__)                               \
  DEFINE_var(varname__, dashes__, shortname__, ,                          \
             "", helpstring__, helparg__, false, options::String_set,     \
             const options::String_set&, options::parse_set)              \
 public:                                                                  \
  bool                                                                    \
  any_##varname__() const                                                 \
  { return !this->varname__##_.value.empty(); }                           \
									  \
  bool                                                                    \
  is_##varname__(const char* symbol) const                                \
  {                                                                       \
    return (!this->varname__##_.value.empty()                             \
            && (this->varname__##_.value.find(std::string(symbol))        \
                != this->varname__##_.value.end()));                      \
  }									  \
									  \
  options::String_set::const_iterator					  \
  varname__##_begin() const						  \
  { return this->varname__##_.value.begin(); }				  \
									  \
  options::String_set::const_iterator					  \
  varname__##_end() const						  \
  { return this->varname__##_.value.end(); }

// When you have a list of possible values (expressed as string)
// After helparg__ should come an initializer list, like
//   {"foo", "bar", "baz"}
#define DEFINE_enum(varname__, dashes__, shortname__, default_value__,   \
                    helpstring__, helparg__, ...)                        \
  DEFINE_var(varname__, dashes__, shortname__, default_value__,          \
             default_value__, helpstring__, helparg__, false,		 \
             const char*, const char*, parse_choices_##varname__)        \
 private:                                                                \
  static void parse_choices_##varname__(const char* option_name,         \
                                        const char* arg,                 \
                                        const char** retval) {           \
    const char* choices[] = __VA_ARGS__;                                 \
    options::parse_choices(option_name, arg, retval,                     \
                           choices, sizeof(choices) / sizeof(*choices)); \
  }

// This is like DEFINE_bool, but VARNAME is the name of a different
// option.  This option becomes an alias for that one.  INVERT is true
// if this option is an inversion of the other one.
#define DEFINE_bool_alias(option__, varname__, dashes__, shortname__,	\
			  helpstring__, no_helpstring__, invert__)	\
 private:								\
  struct Struct_##option__ : public options::Struct_var			\
  {									\
    Struct_##option__()							\
      : option(#option__, dashes__, shortname__, "", helpstring__,	\
	       NULL, false, this)					\
    { }									\
									\
    void								\
    parse_to_value(const char*, const char*,				\
		   Command_line*, General_options* options)		\
    {									\
      options->set_##varname__(!invert__);				\
      options->set_user_set_##varname__();				\
    }									\
									\
    options::One_option option;						\
  };									\
  Struct_##option__ option__##_;					\
									\
  struct Struct_no_##option__ : public options::Struct_var		\
  {									\
    Struct_no_##option__()						\
      : option((dashes__ == options::DASH_Z				\
		? "no" #option__					\
		: "no-" #option__),					\
	       dashes__, '\0', "", no_helpstring__,			\
	       NULL, false, this)					\
    { }									\
									\
    void								\
    parse_to_value(const char*, const char*,				\
		   Command_line*, General_options* options)		\
    {									\
      options->set_##varname__(invert__);				\
      options->set_user_set_##varname__();				\
    }									\
									\
    options::One_option option;						\
  };									\
  Struct_no_##option__ no_##option__##_initializer_

// This is used for non-standard flags.  It defines no functions; it
// just calls General_options::parse_VARNAME whenever the flag is
// seen.  We declare parse_VARNAME as a static member of
// General_options; you are responsible for defining it there.
// helparg__ should be NULL iff this special-option is a boolean.
#define DEFINE_special(varname__, dashes__, shortname__,                \
                       helpstring__, helparg__)                         \
 private:                                                               \
  void parse_##varname__(const char* option, const char* arg,           \
                         Command_line* inputs);                         \
  struct Struct_##varname__ : public options::Struct_special            \
  {                                                                     \
    Struct_##varname__()                                                \
      : options::Struct_special(#varname__, dashes__, shortname__,      \
                                &General_options::parse_##varname__,    \
                                helpstring__, helparg__)                \
    { }                                                                 \
  };                                                                    \
  Struct_##varname__ varname__##_initializer_

// An option that takes an optional string argument.  If the option is
// used with no argument, the value will be the default, and
// user_set_via_option will be true.
#define DEFINE_optional_string(varname__, dashes__, shortname__,	\
			       default_value__,				\
			       helpstring__, helparg__)			\
  DEFINE_var(varname__, dashes__, shortname__, default_value__,		\
             default_value__, helpstring__, helparg__, true,		\
             const char*, const char*, options::parse_optional_string)

// A directory to search.  For each directory we record whether it is
// in the sysroot.  We need to know this so that, if a linker script
// is found within the sysroot, we will apply the sysroot to any files
// named by that script.

class Search_directory
{
 public:
  // We need a default constructor because we put this in a
  // std::vector.
  Search_directory()
    : name_(NULL), put_in_sysroot_(false), is_in_sysroot_(false)
  { }

  // This is the usual constructor.
  Search_directory(const char* name, bool put_in_sysroot)
    : name_(name), put_in_sysroot_(put_in_sysroot), is_in_sysroot_(false)
  {
    if (this->name_.empty())
      this->name_ = ".";
  }

  // This is called if we have a sysroot.  The sysroot is prefixed to
  // any entries for which put_in_sysroot_ is true.  is_in_sysroot_ is
  // set to true for any enries which are in the sysroot (this will
  // naturally include any entries for which put_in_sysroot_ is true).
  // SYSROOT is the sysroot, CANONICAL_SYSROOT is the result of
  // passing SYSROOT to lrealpath.
  void
  add_sysroot(const char* sysroot, const char* canonical_sysroot);

  // Get the directory name.
  const std::string&
  name() const
  { return this->name_; }

  // Return whether this directory is in the sysroot.
  bool
  is_in_sysroot() const
  { return this->is_in_sysroot_; }

 private:
  std::string name_;
  bool put_in_sysroot_;
  bool is_in_sysroot_;
};

class General_options
{
 private:
  // NOTE: For every option that you add here, also consider if you
  // should add it to Position_dependent_options.
  DEFINE_special(help, options::TWO_DASHES, '\0',
                 N_("Report usage information"), NULL);
  DEFINE_special(version, options::TWO_DASHES, 'v',
                 N_("Report version information"), NULL);
  DEFINE_special(V, options::EXACTLY_ONE_DASH, '\0',
                 N_("Report version and target information"), NULL);

  // These options are sorted approximately so that for each letter in
  // the alphabet, we show the option whose shortname is that letter
  // (if any) and then every longname that starts with that letter (in
  // alphabetical order).  For both, lowercase sorts before uppercase.
  // The -z options come last.

  DEFINE_bool(allow_shlib_undefined, options::TWO_DASHES, '\0', false,
              N_("Allow unresolved references in shared libraries"),
              N_("Do not allow unresolved references in shared libraries"));

  DEFINE_bool(as_needed, options::TWO_DASHES, '\0', false,
              N_("Only set DT_NEEDED for dynamic libs if used"),
              N_("Always DT_NEEDED for dynamic libs"));

  // This should really be an "enum", but it's too easy for folks to
  // forget to update the list as they add new targets.  So we just
  // accept any string.  We'll fail later (when the string is parsed),
  // if the target isn't actually supported.
  DEFINE_string(format, options::TWO_DASHES, 'b', "elf",
                N_("Set input format"), ("[elf,binary]"));

  DEFINE_bool(Bdynamic, options::ONE_DASH, '\0', true,
              N_("-l searches for shared libraries"), NULL);
  DEFINE_bool_alias(Bstatic, Bdynamic, options::ONE_DASH, '\0',
		    N_("-l does not search for shared libraries"), NULL,
		    true);

  DEFINE_bool(Bsymbolic, options::ONE_DASH, '\0', false,
              N_("Bind defined symbols locally"), NULL);

  DEFINE_bool(Bsymbolic_functions, options::ONE_DASH, '\0', false,
	      N_("Bind defined function symbols locally"), NULL);

  DEFINE_optional_string(build_id, options::TWO_DASHES, '\0', "sha1",
			 N_("Generate build ID note"),
			 N_("[=STYLE]"));

  DEFINE_bool(check_sections, options::TWO_DASHES, '\0', true,
	      N_("Check segment addresses for overlaps (default)"),
	      N_("Do not check segment addresses for overlaps"));

#ifdef HAVE_ZLIB_H
  DEFINE_enum(compress_debug_sections, options::TWO_DASHES, '\0', "none",
              N_("Compress .debug_* sections in the output file"),
              ("[none,zlib]"),
              {"none", "zlib"});
#else
  DEFINE_enum(compress_debug_sections, options::TWO_DASHES, '\0', "none",
              N_("Compress .debug_* sections in the output file"),
              N_("[none]"),
              {"none"});
#endif

  DEFINE_bool(define_common, options::TWO_DASHES, 'd', false,
              N_("Define common symbols"),
              N_("Do not define common symbols"));
  DEFINE_bool(dc, options::ONE_DASH, '\0', false,
              N_("Alias for -d"), NULL);
  DEFINE_bool(dp, options::ONE_DASH, '\0', false,
              N_("Alias for -d"), NULL);

  DEFINE_string(debug, options::TWO_DASHES, '\0', "",
                N_("Turn on debugging"),
                N_("[all,files,script,task][,...]"));

  DEFINE_special(defsym, options::TWO_DASHES, '\0',
                 N_("Define a symbol"), N_("SYMBOL=EXPRESSION"));

  DEFINE_optional_string(demangle, options::TWO_DASHES, '\0', NULL,
			 N_("Demangle C++ symbols in log messages"),
			 N_("[=STYLE]"));

  DEFINE_bool(no_demangle, options::TWO_DASHES, '\0', false,
	      N_("Do not demangle C++ symbols in log messages"),
	      NULL);

  DEFINE_bool(detect_odr_violations, options::TWO_DASHES, '\0', false,
              N_("Try to detect violations of the One Definition Rule"),
              NULL);

  DEFINE_string(entry, options::TWO_DASHES, 'e', NULL,
                N_("Set program start address"), N_("ADDRESS"));

  DEFINE_bool(export_dynamic, options::TWO_DASHES, 'E', false,
              N_("Export all dynamic symbols"), NULL);

  DEFINE_bool(eh_frame_hdr, options::TWO_DASHES, '\0', false,
              N_("Create exception frame header"), NULL);

  DEFINE_bool(fatal_warnings, options::TWO_DASHES, '\0', false,
	      N_("Treat warnings as errors"),
	      N_("Do not treat warnings as errors"));

  DEFINE_string(soname, options::ONE_DASH, 'h', NULL,
                N_("Set shared library name"), N_("FILENAME"));

  DEFINE_double(hash_bucket_empty_fraction, options::TWO_DASHES, '\0', 0.0,
		N_("Min fraction of empty buckets in dynamic hash"),
		N_("FRACTION"));

  DEFINE_enum(hash_style, options::TWO_DASHES, '\0', "sysv",
	      N_("Dynamic hash style"), N_("[sysv,gnu,both]"),
	      {"sysv", "gnu", "both"});

  DEFINE_string(dynamic_linker, options::TWO_DASHES, 'I', NULL,
                N_("Set dynamic linker path"), N_("PROGRAM"));

  DEFINE_special(just_symbols, options::TWO_DASHES, '\0',
                 N_("Read only symbol values from FILE"), N_("FILE"));

  DEFINE_special(library, options::TWO_DASHES, 'l',
                 N_("Search for library LIBNAME"), N_("LIBNAME"));

  DEFINE_dirlist(library_path, options::TWO_DASHES, 'L',
                 N_("Add directory to search path"), N_("DIR"));

  DEFINE_string(m, options::EXACTLY_ONE_DASH, 'm', "",
                N_("Ignored for compatibility"), N_("EMULATION"));

  DEFINE_bool(print_map, options::TWO_DASHES, 'M', false,
	      N_("Write map file on standard output"), NULL);
  DEFINE_string(Map, options::ONE_DASH, '\0', NULL, N_("Write map file"),
		N_("MAPFILENAME"));

  DEFINE_bool(nmagic, options::TWO_DASHES, 'n', false,
	      N_("Do not page align data"), NULL);
  DEFINE_bool(omagic, options::EXACTLY_TWO_DASHES, 'N', false,
	      N_("Do not page align data, do not make text readonly"),
	      N_("Page align data, make text readonly"));

  DEFINE_enable(new_dtags, options::EXACTLY_TWO_DASHES, '\0', false,
		N_("Enable use of DT_RUNPATH and DT_FLAGS"),
		N_("Disable use of DT_RUNPATH and DT_FLAGS"));

  DEFINE_bool(noinhibit_exec, options::TWO_DASHES, '\0', false,
	      N_("Create an output file even if errors occur"), NULL);

  DEFINE_bool_alias(no_undefined, defs, options::TWO_DASHES, '\0',
		    N_("Report undefined symbols (even with --shared)"),
		    NULL, false);

  DEFINE_string(output, options::TWO_DASHES, 'o', "a.out",
                N_("Set output file name"), N_("FILE"));

  DEFINE_uint(optimize, options::EXACTLY_ONE_DASH, 'O', 0,
              N_("Optimize output file size"), N_("LEVEL"));

  DEFINE_string(oformat, options::EXACTLY_TWO_DASHES, '\0', "elf",
		N_("Set output format"), N_("[binary]"));

  DEFINE_bool(preread_archive_symbols, options::TWO_DASHES, '\0', false,
              N_("Preread archive symbols when multi-threaded"), NULL);
  DEFINE_string(print_symbol_counts, options::TWO_DASHES, '\0', NULL,
		N_("Print symbols defined and used for each input"),
		N_("FILENAME"));

  DEFINE_bool(Qy, options::EXACTLY_ONE_DASH, '\0', false,
	      N_("Ignored for SVR4 compatibility"), NULL);

  DEFINE_bool(emit_relocs, options::TWO_DASHES, 'q', false,
              N_("Generate relocations in output"), NULL);

  DEFINE_bool(relocatable, options::EXACTLY_ONE_DASH, 'r', false,
              N_("Generate relocatable output"), NULL);

  DEFINE_bool(relax, options::TWO_DASHES, '\0', false,
	      N_("Relax branches on certain targets"), NULL);

  // -R really means -rpath, but can mean --just-symbols for
  // compatibility with GNU ld.  -rpath is always -rpath, so we list
  // it separately.
  DEFINE_special(R, options::EXACTLY_ONE_DASH, 'R',
                 N_("Add DIR to runtime search path"), N_("DIR"));

  DEFINE_dirlist(rpath, options::ONE_DASH, '\0',
                 N_("Add DIR to runtime search path"), N_("DIR"));

  DEFINE_dirlist(rpath_link, options::TWO_DASHES, '\0',
                 N_("Add DIR to link time shared library search path"),
                 N_("DIR"));

  DEFINE_bool(strip_all, options::TWO_DASHES, 's', false,
              N_("Strip all symbols"), NULL);
  DEFINE_bool(strip_debug, options::TWO_DASHES, 'S', false,
              N_("Strip debugging information"), NULL);
  DEFINE_bool(strip_debug_non_line, options::TWO_DASHES, '\0', false,
              N_("Emit only debug line number information"), NULL);
  DEFINE_bool(strip_debug_gdb, options::TWO_DASHES, '\0', false,
              N_("Strip debug symbols that are unused by gdb "
                 "(at least versions <= 6.7)"), NULL);

  DEFINE_bool(shared, options::ONE_DASH, '\0', false,
              N_("Generate shared library"), NULL);

  // This is not actually special in any way, but I need to give it
  // a non-standard accessor-function name because 'static' is a keyword.
  DEFINE_special(static, options::ONE_DASH, '\0',
                 N_("Do not link against shared libraries"), NULL);

  DEFINE_bool(stats, options::TWO_DASHES, '\0', false,
              N_("Print resource usage statistics"), NULL);

  DEFINE_string(sysroot, options::TWO_DASHES, '\0', "",
                N_("Set target system root directory"), N_("DIR"));

  DEFINE_bool(trace, options::TWO_DASHES, 't', false,
              N_("Print the name of each input file"), NULL);

  DEFINE_special(script, options::TWO_DASHES, 'T',
                 N_("Read linker script"), N_("FILE"));

  DEFINE_bool(threads, options::TWO_DASHES, '\0', false,
              N_("Run the linker multi-threaded"),
              N_("Do not run the linker multi-threaded"));
  DEFINE_uint(thread_count, options::TWO_DASHES, '\0', 0,
              N_("Number of threads to use"), N_("COUNT"));
  DEFINE_uint(thread_count_initial, options::TWO_DASHES, '\0', 0,
              N_("Number of threads to use in initial pass"), N_("COUNT"));
  DEFINE_uint(thread_count_middle, options::TWO_DASHES, '\0', 0,
              N_("Number of threads to use in middle pass"), N_("COUNT"));
  DEFINE_uint(thread_count_final, options::TWO_DASHES, '\0', 0,
              N_("Number of threads to use in final pass"), N_("COUNT"));

  DEFINE_uint64(Tbss, options::ONE_DASH, '\0', -1U,
                N_("Set the address of the bss segment"), N_("ADDRESS"));
  DEFINE_uint64(Tdata, options::ONE_DASH, '\0', -1U,
                N_("Set the address of the data segment"), N_("ADDRESS"));
  DEFINE_uint64(Ttext, options::ONE_DASH, '\0', -1U,
                N_("Set the address of the text segment"), N_("ADDRESS"));

  DEFINE_set(undefined, options::TWO_DASHES, 'u',
	     N_("Create undefined reference to SYMBOL"), N_("SYMBOL"));

  DEFINE_bool(verbose, options::TWO_DASHES, '\0', false,
              N_("Synonym for --debug=files"), NULL);

  DEFINE_special(version_script, options::TWO_DASHES, '\0',
                 N_("Read version script"), N_("FILE"));

  DEFINE_bool(whole_archive, options::TWO_DASHES, '\0', false,
              N_("Include all archive contents"),
              N_("Include only needed archive contents"));

  DEFINE_set(wrap, options::TWO_DASHES, '\0',
	     N_("Use wrapper functions for SYMBOL"), N_("SYMBOL"));

  DEFINE_set(trace_symbol, options::TWO_DASHES, 'y',
             N_("Trace references to symbol"), N_("SYMBOL"));

  DEFINE_string(Y, options::EXACTLY_ONE_DASH, 'Y', "",
		N_("Default search path for Solaris compatibility"),
		N_("PATH"));

  DEFINE_special(start_group, options::TWO_DASHES, '(',
                 N_("Start a library search group"), NULL);
  DEFINE_special(end_group, options::TWO_DASHES, ')',
                 N_("End a library search group"), NULL);

  // The -z options.

  DEFINE_bool(combreloc, options::DASH_Z, '\0', true,
	      N_("Sort dynamic relocs"),
	      N_("Do not sort dynamic relocs"));
  DEFINE_uint64(common_page_size, options::DASH_Z, '\0', 0,
                N_("Set common page size to SIZE"), N_("SIZE"));
  DEFINE_bool(defs, options::DASH_Z, '\0', false,
              N_("Report undefined symbols (even with --shared)"),
              NULL);
  DEFINE_bool(execstack, options::DASH_Z, '\0', false,
              N_("Mark output as requiring executable stack"), NULL);
  DEFINE_uint64(max_page_size, options::DASH_Z, '\0', 0,
                N_("Set maximum page size to SIZE"), N_("SIZE"));
  DEFINE_bool(noexecstack, options::DASH_Z, '\0', false,
              N_("Mark output as not requiring executable stack"), NULL);
  DEFINE_bool(initfirst, options::DASH_Z, '\0', false,
	      N_("Mark DSO to be initialized first at runtime"),
	      NULL);
  DEFINE_bool(interpose, options::DASH_Z, '\0', false,
	      N_("Mark object to interpose all DSOs but executable"),
	      NULL);
  DEFINE_bool(loadfltr, options::DASH_Z, '\0', false,
	      N_("Mark object requiring immediate process"),
	      NULL);
  DEFINE_bool(nodefaultlib, options::DASH_Z, '\0', false,
	      N_("Mark object not to use default search paths"),
	      NULL);
  DEFINE_bool(nodelete, options::DASH_Z, '\0', false,
	      N_("Mark DSO non-deletable at runtime"),
	      NULL);
  DEFINE_bool(nodlopen, options::DASH_Z, '\0', false,
	      N_("Mark DSO not available to dlopen"),
	      NULL);
  DEFINE_bool(nodump, options::DASH_Z, '\0', false,
	      N_("Mark DSO not available to dldump"),
	      NULL);
  DEFINE_bool(relro, options::DASH_Z, '\0', false,
	      N_("Where possible mark variables read-only after relocation"),
	      N_("Don't mark variables read-only after relocation"));

 public:
  typedef options::Dir_list Dir_list;

  General_options();

  // Does post-processing on flags, making sure they all have
  // non-conflicting values.  Also converts some flags from their
  // "standard" types (string, etc), to another type (enum, DirList),
  // which can be accessed via a separate method.  Dies if it notices
  // any problems.
  void finalize();

  // The macro defines output() (based on --output), but that's a
  // generic name.  Provide this alternative name, which is clearer.
  const char*
  output_file_name() const
  { return this->output(); }

  // This is not defined via a flag, but combines flags to say whether
  // the output is position-independent or not.
  bool
  output_is_position_independent() const
  { return this->shared(); }

  // This would normally be static(), and defined automatically, but
  // since static is a keyword, we need to come up with our own name.
  bool
  is_static() const
  { return static_; }

  // In addition to getting the input and output formats as a string
  // (via format() and oformat()), we also give access as an enum.
  enum Object_format
  {
    // Ordinary ELF.
    OBJECT_FORMAT_ELF,
    // Straight binary format.
    OBJECT_FORMAT_BINARY
  };

  // Note: these functions are not very fast.
  Object_format format_enum() const;
  Object_format oformat_enum() const;

  // These are the best way to get access to the execstack state,
  // not execstack() and noexecstack() which are hard to use properly.
  bool
  is_execstack_set() const
  { return this->execstack_status_ != EXECSTACK_FROM_INPUT; }

  bool
  is_stack_executable() const
  { return this->execstack_status_ == EXECSTACK_YES; }

  // The --demangle option takes an optional string, and there is also
  // a --no-demangle option.  This is the best way to decide whether
  // to demangle or not.
  bool
  do_demangle() const
  { return this->do_demangle_; }

 private:
  // Don't copy this structure.
  General_options(const General_options&);
  General_options& operator=(const General_options&);

  // Whether to mark the stack as executable.
  enum Execstack
  {
    // Not set on command line.
    EXECSTACK_FROM_INPUT,
    // Mark the stack as executable (-z execstack).
    EXECSTACK_YES,
    // Mark the stack as not executable (-z noexecstack).
    EXECSTACK_NO
  };

  void
  set_execstack_status(Execstack value)
  { this->execstack_status_ = value; }

  void
  set_do_demangle(bool value)
  { this->do_demangle_ = value; }

  void
  set_static(bool value)
  { static_ = value; }

  // These are called by finalize() to set up the search-path correctly.
  void
  add_to_library_path_with_sysroot(const char* arg)
  { this->add_search_directory_to_library_path(Search_directory(arg, true)); }

  // Apply any sysroot to the directory lists.
  void
  add_sysroot();

  // Whether to mark the stack as executable.
  Execstack execstack_status_;
  // Whether to do a static link.
  bool static_;
  // Whether to do demangling.
  bool do_demangle_;
};

// The position-dependent options.  We use this to store the state of
// the commandline at a particular point in parsing for later
// reference.  For instance, if we see "ld --whole-archive foo.a
// --no-whole-archive," we want to store the whole-archive option with
// foo.a, so when the time comes to parse foo.a we know we should do
// it in whole-archive mode.  We could store all of General_options,
// but that's big, so we just pick the subset of flags that actually
// change in a position-dependent way.

#define DEFINE_posdep(varname__, type__)        \
 public:                                        \
  type__                                        \
  varname__() const                             \
  { return this->varname__##_; }                \
                                                \
  void                                          \
  set_##varname__(type__ value)                 \
  { this->varname__##_ = value; }               \
 private:                                       \
  type__ varname__##_

class Position_dependent_options
{
 public:
  Position_dependent_options(const General_options& options
                             = Position_dependent_options::default_options_)
  { copy_from_options(options); }

  void copy_from_options(const General_options& options)
  {
    this->set_as_needed(options.as_needed());
    this->set_Bdynamic(options.Bdynamic());
    this->set_format_enum(options.format_enum());
    this->set_whole_archive(options.whole_archive());
  }

  DEFINE_posdep(as_needed, bool);
  DEFINE_posdep(Bdynamic, bool);
  DEFINE_posdep(format_enum, General_options::Object_format);
  DEFINE_posdep(whole_archive, bool);

 private:
  // This is a General_options with everything set to its default
  // value.  A Position_dependent_options created with no argument
  // will take its values from here.
  static General_options default_options_;
};


// A single file or library argument from the command line.

class Input_file_argument
{
 public:
  // name: file name or library name
  // is_lib: true if name is a library name: that is, emits the leading
  //         "lib" and trailing ".so"/".a" from the name
  // extra_search_path: an extra directory to look for the file, prior
  //         to checking the normal library search path.  If this is "",
  //         then no extra directory is added.
  // just_symbols: whether this file only defines symbols.
  // options: The position dependent options at this point in the
  //         command line, such as --whole-archive.
  Input_file_argument()
    : name_(), is_lib_(false), extra_search_path_(""), just_symbols_(false),
      options_()
  { }

  Input_file_argument(const char* name, bool is_lib,
                      const char* extra_search_path,
                      bool just_symbols,
                      const Position_dependent_options& options)
    : name_(name), is_lib_(is_lib), extra_search_path_(extra_search_path),
      just_symbols_(just_symbols), options_(options)
  { }

  // You can also pass in a General_options instance instead of a
  // Position_dependent_options.  In that case, we extract the
  // position-independent vars from the General_options and only store
  // those.
  Input_file_argument(const char* name, bool is_lib,
                      const char* extra_search_path,
                      bool just_symbols,
                      const General_options& options)
    : name_(name), is_lib_(is_lib), extra_search_path_(extra_search_path),
      just_symbols_(just_symbols), options_(options)
  { }

  const char*
  name() const
  { return this->name_.c_str(); }

  const Position_dependent_options&
  options() const
  { return this->options_; }

  bool
  is_lib() const
  { return this->is_lib_; }

  const char*
  extra_search_path() const
  {
    return (this->extra_search_path_.empty()
            ? NULL
            : this->extra_search_path_.c_str());
  }

  // Return whether we should only read symbols from this file.
  bool
  just_symbols() const
  { return this->just_symbols_; }

  // Return whether this file may require a search using the -L
  // options.
  bool
  may_need_search() const
  { return this->is_lib_ || !this->extra_search_path_.empty(); }

 private:
  // We use std::string, not const char*, here for convenience when
  // using script files, so that we do not have to preserve the string
  // in that case.
  std::string name_;
  bool is_lib_;
  std::string extra_search_path_;
  bool just_symbols_;
  Position_dependent_options options_;
};

// A file or library, or a group, from the command line.

class Input_argument
{
 public:
  // Create a file or library argument.
  explicit Input_argument(Input_file_argument file)
    : is_file_(true), file_(file), group_(NULL)
  { }

  // Create a group argument.
  explicit Input_argument(Input_file_group* group)
    : is_file_(false), group_(group)
  { }

  // Return whether this is a file.
  bool
  is_file() const
  { return this->is_file_; }

  // Return whether this is a group.
  bool
  is_group() const
  { return !this->is_file_; }

  // Return the information about the file.
  const Input_file_argument&
  file() const
  {
    gold_assert(this->is_file_);
    return this->file_;
  }

  // Return the information about the group.
  const Input_file_group*
  group() const
  {
    gold_assert(!this->is_file_);
    return this->group_;
  }

  Input_file_group*
  group()
  {
    gold_assert(!this->is_file_);
    return this->group_;
  }

 private:
  bool is_file_;
  Input_file_argument file_;
  Input_file_group* group_;
};

// A group from the command line.  This is a set of arguments within
// --start-group ... --end-group.

class Input_file_group
{
 public:
  typedef std::vector<Input_argument> Files;
  typedef Files::const_iterator const_iterator;

  Input_file_group()
    : files_()
  { }

  // Add a file to the end of the group.
  void
  add_file(const Input_file_argument& arg)
  { this->files_.push_back(Input_argument(arg)); }

  // Iterators to iterate over the group contents.

  const_iterator
  begin() const
  { return this->files_.begin(); }

  const_iterator
  end() const
  { return this->files_.end(); }

 private:
  Files files_;
};

// A list of files from the command line or a script.

class Input_arguments
{
 public:
  typedef std::vector<Input_argument> Input_argument_list;
  typedef Input_argument_list::const_iterator const_iterator;

  Input_arguments()
    : input_argument_list_(), in_group_(false)
  { }

  // Add a file.
  void
  add_file(const Input_file_argument& arg);

  // Start a group (the --start-group option).
  void
  start_group();

  // End a group (the --end-group option).
  void
  end_group();

  // Return whether we are currently in a group.
  bool
  in_group() const
  { return this->in_group_; }

  // The number of entries in the list.
  int
  size() const
  { return this->input_argument_list_.size(); }

  // Iterators to iterate over the list of input files.

  const_iterator
  begin() const
  { return this->input_argument_list_.begin(); }

  const_iterator
  end() const
  { return this->input_argument_list_.end(); }

  // Return whether the list is empty.
  bool
  empty() const
  { return this->input_argument_list_.empty(); }

 private:
  Input_argument_list input_argument_list_;
  bool in_group_;
};


// All the information read from the command line.  These are held in
// three separate structs: one to hold the options (--foo), one to
// hold the filenames listed on the commandline, and one to hold
// linker script information.  This third is not a subset of the other
// two because linker scripts can be specified either as options (via
// -T) or as a file.

class Command_line
{
 public:
  typedef Input_arguments::const_iterator const_iterator;

  Command_line();

  // Process the command line options.  This will exit with an
  // appropriate error message if an unrecognized option is seen.
  void
  process(int argc, const char** argv);

  // Process one command-line option.  This takes the index of argv to
  // process, and returns the index for the next option.  no_more_options
  // is set to true if argv[i] is "--".
  int
  process_one_option(int argc, const char** argv, int i,
                     bool* no_more_options);

  // Get the general options.
  const General_options&
  options() const
  { return this->options_; }

  // Get the position dependent options.
  const Position_dependent_options&
  position_dependent_options() const
  { return this->position_options_; }

  // Get the linker-script options.
  Script_options&
  script_options()
  { return this->script_options_; }

  // Get the version-script options: a convenience routine.
  const Version_script_info&
  version_script() const
  { return *this->script_options_.version_script_info(); }

  // Get the input files.
  Input_arguments&
  inputs()
  { return this->inputs_; }

  // The number of input files.
  int
  number_of_input_files() const
  { return this->inputs_.size(); }

  // Iterators to iterate over the list of input files.

  const_iterator
  begin() const
  { return this->inputs_.begin(); }

  const_iterator
  end() const
  { return this->inputs_.end(); }

 private:
  Command_line(const Command_line&);
  Command_line& operator=(const Command_line&);

  General_options options_;
  Position_dependent_options position_options_;
  Script_options script_options_;
  Input_arguments inputs_;
};

} // End namespace gold.

#endif // !defined(GOLD_OPTIONS_H)
