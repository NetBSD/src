// gold.cc -- main linker functions

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

#include "gold.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include "libiberty.h"

#include "options.h"
#include "debug.h"
#include "workqueue.h"
#include "dirsearch.h"
#include "readsyms.h"
#include "symtab.h"
#include "common.h"
#include "object.h"
#include "layout.h"
#include "reloc.h"
#include "defstd.h"

namespace gold
{

const char* program_name;

void
gold_exit(bool status)
{
  if (!status && parameters != NULL && parameters->options_valid())
    unlink_if_ordinary(parameters->options().output_file_name());
  exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
}

void
gold_nomem()
{
  // We are out of memory, so try hard to print a reasonable message.
  // Note that we don't try to translate this message, since the
  // translation process itself will require memory.

  // LEN only exists to avoid a pointless warning when write is
  // declared with warn_use_result, as when compiling with
  // -D_USE_FORTIFY on GNU/Linux.  Casting to void does not appear to
  // work, at least not with gcc 4.3.0.

  ssize_t len = write(2, program_name, strlen(program_name));
  if (len >= 0)
    {
      const char* const s = ": out of memory\n";
      len = write(2, s, strlen(s));
    }
  gold_exit(false);
}

// Handle an unreachable case.

void
do_gold_unreachable(const char* filename, int lineno, const char* function)
{
  fprintf(stderr, _("%s: internal error in %s, at %s:%d\n"),
	  program_name, function, filename, lineno);
  gold_exit(false);
}

// This class arranges to run the functions done in the middle of the
// link.  It is just a closure.

class Middle_runner : public Task_function_runner
{
 public:
  Middle_runner(const General_options& options,
		const Input_objects* input_objects,
		Symbol_table* symtab,
		Layout* layout, Mapfile* mapfile)
    : options_(options), input_objects_(input_objects), symtab_(symtab),
      layout_(layout), mapfile_(mapfile)
  { }

  void
  run(Workqueue*, const Task*);

 private:
  const General_options& options_;
  const Input_objects* input_objects_;
  Symbol_table* symtab_;
  Layout* layout_;
  Mapfile* mapfile_;
};

void
Middle_runner::run(Workqueue* workqueue, const Task* task)
{
  queue_middle_tasks(this->options_, task, this->input_objects_, this->symtab_,
		     this->layout_, workqueue, this->mapfile_);
}

// Queue up the initial set of tasks for this link job.

void
queue_initial_tasks(const General_options& options,
		    Dirsearch& search_path,
		    const Command_line& cmdline,
		    Workqueue* workqueue, Input_objects* input_objects,
		    Symbol_table* symtab, Layout* layout, Mapfile* mapfile)
{
  if (cmdline.begin() == cmdline.end())
    gold_fatal(_("no input files"));

  int thread_count = options.thread_count_initial();
  if (thread_count == 0)
    thread_count = cmdline.number_of_input_files();
  workqueue->set_thread_count(thread_count);

  // Read the input files.  We have to add the symbols to the symbol
  // table in order.  We do this by creating a separate blocker for
  // each input file.  We associate the blocker with the following
  // input file, to give us a convenient place to delete it.
  Task_token* this_blocker = NULL;
  for (Command_line::const_iterator p = cmdline.begin();
       p != cmdline.end();
       ++p)
    {
      Task_token* next_blocker = new Task_token(true);
      next_blocker->add_blocker();
      workqueue->queue(new Read_symbols(options, input_objects, symtab, layout,
					&search_path, mapfile, &*p, NULL,
					this_blocker, next_blocker));
      this_blocker = next_blocker;
    }

  workqueue->queue(new Task_function(new Middle_runner(options,
						       input_objects,
						       symtab,
						       layout,
						       mapfile),
				     this_blocker,
				     "Task_function Middle_runner"));
}

// Queue up the middle set of tasks.  These are the tasks which run
// after all the input objects have been found and all the symbols
// have been read, but before we lay out the output file.

void
queue_middle_tasks(const General_options& options,
		   const Task* task,
		   const Input_objects* input_objects,
		   Symbol_table* symtab,
		   Layout* layout,
		   Workqueue* workqueue,
		   Mapfile* mapfile)
{
  // We have to support the case of not seeing any input objects, and
  // generate an empty file.  Existing builds depend on being able to
  // pass an empty archive to the linker and get an empty object file
  // out.  In order to do this we need to use a default target.
  if (input_objects->number_of_input_objects() == 0)
    set_parameters_target(&parameters->default_target());

  int thread_count = options.thread_count_middle();
  if (thread_count == 0)
    thread_count = std::max(2, input_objects->number_of_input_objects());
  workqueue->set_thread_count(thread_count);

  // Now we have seen all the input files.
  const bool doing_static_link = (!input_objects->any_dynamic()
				  && !parameters->options().shared());
  set_parameters_doing_static_link(doing_static_link);
  if (!doing_static_link && options.is_static())
    {
      // We print out just the first .so we see; there may be others.
      gold_error(_("cannot mix -static with dynamic object %s"),
		 (*input_objects->dynobj_begin())->name().c_str());
    }
  if (!doing_static_link && parameters->options().relocatable())
    gold_error(_("cannot mix -r with dynamic object %s"),
	       (*input_objects->dynobj_begin())->name().c_str());
  if (!doing_static_link
      && options.oformat_enum() != General_options::OBJECT_FORMAT_ELF)
    gold_fatal(_("cannot use non-ELF output format with dynamic object %s"),
	       (*input_objects->dynobj_begin())->name().c_str());

  if (is_debugging_enabled(DEBUG_SCRIPT))
    layout->script_options()->print(stderr);

  // For each dynamic object, record whether we've seen all the
  // dynamic objects that it depends upon.
  input_objects->check_dynamic_dependencies();

  // See if any of the input definitions violate the One Definition Rule.
  // TODO: if this is too slow, do this as a task, rather than inline.
  symtab->detect_odr_violations(task, options.output_file_name());

  // Create any output sections required by any linker script.
  layout->create_script_sections();

  // Define some sections and symbols needed for a dynamic link.  This
  // handles some cases we want to see before we read the relocs.
  layout->create_initial_dynamic_sections(symtab);

  // Define symbols from any linker scripts.
  layout->define_script_symbols(symtab);

  // Add any symbols named with -u options to the symbol table.
  symtab->add_undefined_symbols_from_command_line();

  // Attach sections to segments.
  layout->attach_sections_to_segments();

  if (!parameters->options().relocatable())
    {
      // Predefine standard symbols.
      define_standard_symbols(symtab, layout);

      // Define __start and __stop symbols for output sections where
      // appropriate.
      layout->define_section_symbols(symtab);
    }

  // Make sure we have symbols for any required group signatures.
  layout->define_group_signatures(symtab);

  // Read the relocations of the input files.  We do this to find
  // which symbols are used by relocations which require a GOT and/or
  // a PLT entry, or a COPY reloc.  When we implement garbage
  // collection we will do it here by reading the relocations in a
  // breadth first search by references.
  //
  // We could also read the relocations during the first pass, and
  // mark symbols at that time.  That is how the old GNU linker works.
  // Doing that is more complex, since we may later decide to discard
  // some of the sections, and thus change our minds about the types
  // of references made to the symbols.
  Task_token* blocker = new Task_token(true);
  Task_token* symtab_lock = new Task_token(false);
  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      // We can read and process the relocations in any order.  But we
      // only want one task to write to the symbol table at a time.
      // So we queue up a task for each object to read the
      // relocations.  That task will in turn queue a task to wait
      // until it can write to the symbol table.
      blocker->add_blocker();
      workqueue->queue(new Read_relocs(options, symtab, layout, *p,
				       symtab_lock, blocker));
    }

  // Allocate common symbols.  This requires write access to the
  // symbol table, but is independent of the relocation processing.
  if (parameters->options().define_common())
    {
      blocker->add_blocker();
      workqueue->queue(new Allocate_commons_task(symtab, layout, mapfile,
						 symtab_lock, blocker));
    }

  // When all those tasks are complete, we can start laying out the
  // output file.
  // TODO(csilvers): figure out a more principled way to get the target
  Target* target = const_cast<Target*>(&parameters->target());
  workqueue->queue(new Task_function(new Layout_task_runner(options,
							    input_objects,
							    symtab,
                                                            target,
							    layout,
							    mapfile),
				     blocker,
				     "Task_function Layout_task_runner"));
}

// Queue up the final set of tasks.  This is called at the end of
// Layout_task.

void
queue_final_tasks(const General_options& options,
		  const Input_objects* input_objects,
		  const Symbol_table* symtab,
		  Layout* layout,
		  Workqueue* workqueue,
		  Output_file* of)
{
  int thread_count = options.thread_count_final();
  if (thread_count == 0)
    thread_count = std::max(2, input_objects->number_of_input_objects());
  workqueue->set_thread_count(thread_count);

  bool any_postprocessing_sections = layout->any_postprocessing_sections();

  // Use a blocker to wait until all the input sections have been
  // written out.
  Task_token* input_sections_blocker = NULL;
  if (!any_postprocessing_sections)
    input_sections_blocker = new Task_token(true);

  // Use a blocker to block any objects which have to wait for the
  // output sections to complete before they can apply relocations.
  Task_token* output_sections_blocker = new Task_token(true);

  // Use a blocker to block the final cleanup task.
  Task_token* final_blocker = new Task_token(true);

  // Queue a task to write out the symbol table.
  final_blocker->add_blocker();
  workqueue->queue(new Write_symbols_task(layout,
					  symtab,
					  input_objects,
					  layout->sympool(),
					  layout->dynpool(),
					  of,
					  final_blocker));

  // Queue a task to write out the output sections.
  output_sections_blocker->add_blocker();
  final_blocker->add_blocker();
  workqueue->queue(new Write_sections_task(layout, of, output_sections_blocker,
					   final_blocker));

  // Queue a task to write out everything else.
  final_blocker->add_blocker();
  workqueue->queue(new Write_data_task(layout, symtab, of, final_blocker));

  // Queue a task for each input object to relocate the sections and
  // write out the local symbols.
  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      if (input_sections_blocker != NULL)
	input_sections_blocker->add_blocker();
      final_blocker->add_blocker();
      workqueue->queue(new Relocate_task(options, symtab, layout, *p, of,
					 input_sections_blocker,
					 output_sections_blocker,
					 final_blocker));
    }

  // Queue a task to write out the output sections which depend on
  // input sections.  If there are any sections which require
  // postprocessing, then we need to do this last, since it may resize
  // the output file.
  if (!any_postprocessing_sections)
    {
      final_blocker->add_blocker();
      Task* t = new Write_after_input_sections_task(layout, of,
						    input_sections_blocker,
						    final_blocker);
      workqueue->queue(t);
    }
  else
    {
      Task_token *new_final_blocker = new Task_token(true);
      new_final_blocker->add_blocker();
      Task* t = new Write_after_input_sections_task(layout, of,
						    final_blocker,
						    new_final_blocker);
      workqueue->queue(t);
      final_blocker = new_final_blocker;
    }

  // Queue a task to close the output file.  This will be blocked by
  // FINAL_BLOCKER.
  workqueue->queue(new Task_function(new Close_task_runner(&options, layout,
							   of),
				     final_blocker,
				     "Task_function Close_task_runner"));
}

} // End namespace gold.
