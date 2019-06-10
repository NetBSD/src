/* Python interface to finish breakpoints

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */



#include "defs.h"
#include "python-internal.h"
#include "breakpoint.h"
#include "frame.h"
#include "gdbthread.h"
#include "arch-utils.h"
#include "language.h"
#include "observer.h"
#include "inferior.h"
#include "block.h"
#include "location.h"
#include "py-ref.h"

/* Function that is called when a Python finish bp is found out of scope.  */
static const char outofscope_func[] = "out_of_scope";

/* struct implementing the gdb.FinishBreakpoint object by extending
   the gdb.Breakpoint class.  */
struct finish_breakpoint_object
{
  /* gdb.Breakpoint base class.  */
  gdbpy_breakpoint_object py_bp;
  /* gdb.Type object of the value return by the breakpointed function.
     May be NULL if no debug information was available or return type
     was VOID.  */
  PyObject *return_type;
  /* gdb.Value object of the function finished by this breakpoint.  Will be
     NULL if return_type is NULL.  */
  PyObject *function_value;
  /* When stopped at this FinishBreakpoint, gdb.Value object returned by
     the function; Py_None if the value is not computable; NULL if GDB is
     not stopped at a FinishBreakpoint.  */
  PyObject *return_value;
};

extern PyTypeObject finish_breakpoint_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("finish_breakpoint_object");

/* Python function to get the 'return_value' attribute of
   FinishBreakpoint.  */

static PyObject *
bpfinishpy_get_returnvalue (PyObject *self, void *closure)
{
  struct finish_breakpoint_object *self_finishbp =
      (struct finish_breakpoint_object *) self;

  if (!self_finishbp->return_value)
    Py_RETURN_NONE;

  Py_INCREF (self_finishbp->return_value);
  return self_finishbp->return_value;
}

/* Deallocate FinishBreakpoint object.  */

static void
bpfinishpy_dealloc (PyObject *self)
{
  struct finish_breakpoint_object *self_bpfinish =
        (struct finish_breakpoint_object *) self;

  Py_XDECREF (self_bpfinish->function_value);
  Py_XDECREF (self_bpfinish->return_type);
  Py_XDECREF (self_bpfinish->return_value);
}

/* Triggered when gdbpy_should_stop is about to execute the `stop' callback
   of the gdb.FinishBreakpoint object BP_OBJ.  Will compute and cache the
   `return_value', if possible.  */

void
bpfinishpy_pre_stop_hook (struct gdbpy_breakpoint_object *bp_obj)
{
  struct finish_breakpoint_object *self_finishbp =
        (struct finish_breakpoint_object *) bp_obj;

  /* Can compute return_value only once.  */
  gdb_assert (!self_finishbp->return_value);

  if (!self_finishbp->return_type)
    return;

  TRY
    {
      struct value *function =
        value_object_to_value (self_finishbp->function_value);
      struct type *value_type =
        type_object_to_type (self_finishbp->return_type);
      struct value *ret = get_return_value (function, value_type);

      if (ret)
        {
          self_finishbp->return_value = value_to_value_object (ret);
          if (!self_finishbp->return_value)
              gdbpy_print_stack ();
        }
      else
        {
          Py_INCREF (Py_None);
          self_finishbp->return_value = Py_None;
        }
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      gdbpy_print_stack ();
    }
  END_CATCH
}

/* Triggered when gdbpy_should_stop has triggered the `stop' callback
   of the gdb.FinishBreakpoint object BP_OBJ.  */

void
bpfinishpy_post_stop_hook (struct gdbpy_breakpoint_object *bp_obj)
{

  TRY
    {
      /* Can't delete it here, but it will be removed at the next stop.  */
      disable_breakpoint (bp_obj->bp);
      gdb_assert (bp_obj->bp->disposition == disp_del);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      gdbpy_print_stack ();
    }
  END_CATCH
}

/* Python function to create a new breakpoint.  */

static int
bpfinishpy_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "frame", "internal", NULL };
  struct finish_breakpoint_object *self_bpfinish =
      (struct finish_breakpoint_object *) self;
  PyObject *frame_obj = NULL;
  int thread;
  struct frame_info *frame = NULL; /* init for gcc -Wall */
  struct frame_info *prev_frame = NULL;
  struct frame_id frame_id;
  PyObject *internal = NULL;
  int internal_bp = 0;
  CORE_ADDR pc;
  struct symbol *function;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "|OO", keywords,
					&frame_obj, &internal))
    return -1;

  TRY
    {
      /* Default frame to newest frame if necessary.  */
      if (frame_obj == NULL)
	frame = get_current_frame ();
      else
	frame = frame_object_to_frame_info (frame_obj);

      if (frame == NULL)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Invalid ID for the `frame' object."));
	}
      else
	{
	  prev_frame = get_prev_frame (frame);
	  if (prev_frame == 0)
	    {
	      PyErr_SetString (PyExc_ValueError,
			       _("\"FinishBreakpoint\" not "
				 "meaningful in the outermost "
				 "frame."));
	    }
	  else if (get_frame_type (prev_frame) == DUMMY_FRAME)
	    {
	      PyErr_SetString (PyExc_ValueError,
			       _("\"FinishBreakpoint\" cannot "
				 "be set on a dummy frame."));
	    }
	  else
	    {
	      frame_id = get_frame_id (prev_frame);
	      if (frame_id_eq (frame_id, null_frame_id))
		PyErr_SetString (PyExc_ValueError,
				 _("Invalid ID for the `frame' object."));
	    }
	}
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      return -1;
    }
  END_CATCH

  if (PyErr_Occurred ())
    return -1;

  thread = ptid_to_global_thread_id (inferior_ptid);
  if (thread == 0)
    {
      PyErr_SetString (PyExc_ValueError,
                       _("No thread currently selected."));
      return -1;
    }

  if (internal)
    {
      internal_bp = PyObject_IsTrue (internal);
      if (internal_bp == -1)
        {
          PyErr_SetString (PyExc_ValueError,
                           _("The value of `internal' must be a boolean."));
          return -1;
        }
    }

  /* Find the function we will return from.  */
  self_bpfinish->return_type = NULL;
  self_bpfinish->function_value = NULL;

  TRY
    {
      if (get_frame_pc_if_available (frame, &pc))
        {
          function = find_pc_function (pc);
          if (function != NULL)
            {
              struct type *ret_type =
                  TYPE_TARGET_TYPE (SYMBOL_TYPE (function));

              /* Remember only non-void return types.  */
              if (TYPE_CODE (ret_type) != TYPE_CODE_VOID)
                {
                  struct value *func_value;

                  /* Ignore Python errors at this stage.  */
                  self_bpfinish->return_type = type_to_type_object (ret_type);
                  PyErr_Clear ();
                  func_value = read_var_value (function, NULL, frame);
                  self_bpfinish->function_value =
                      value_to_value_object (func_value);
                  PyErr_Clear ();
                }
            }
        }
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      /* Just swallow.  Either the return type or the function value
	 remain NULL.  */
    }
  END_CATCH

  if (self_bpfinish->return_type == NULL || self_bpfinish->function_value == NULL)
    {
      /* Won't be able to compute return value.  */
      Py_XDECREF (self_bpfinish->return_type);
      Py_XDECREF (self_bpfinish->function_value);

      self_bpfinish->return_type = NULL;
      self_bpfinish->function_value = NULL;
    }

  bppy_pending_object = &self_bpfinish->py_bp;
  bppy_pending_object->number = -1;
  bppy_pending_object->bp = NULL;

  TRY
    {
      /* Set a breakpoint on the return address.  */
      event_location_up location
	= new_address_location (get_frame_pc (prev_frame), NULL, 0);
      create_breakpoint (python_gdbarch,
                         location.get (), NULL, thread, NULL,
                         0,
                         1 /*temp_flag*/,
                         bp_breakpoint,
                         0,
                         AUTO_BOOLEAN_TRUE,
                         &bkpt_breakpoint_ops,
                         0, 1, internal_bp, 0);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_SET_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  self_bpfinish->py_bp.bp->frame_id = frame_id;
  self_bpfinish->py_bp.is_finish_bp = 1;

  /* Bind the breakpoint with the current program space.  */
  self_bpfinish->py_bp.bp->pspace = current_program_space;

  return 0;
}

/* Called when GDB notices that the finish breakpoint BP_OBJ is out of
   the current callstack.  Triggers the method OUT_OF_SCOPE if implemented,
   then delete the breakpoint.  */

static void
bpfinishpy_out_of_scope (struct finish_breakpoint_object *bpfinish_obj)
{
  gdbpy_breakpoint_object *bp_obj = (gdbpy_breakpoint_object *) bpfinish_obj;
  PyObject *py_obj = (PyObject *) bp_obj;

  if (bpfinish_obj->py_bp.bp->enable_state == bp_enabled
      && PyObject_HasAttrString (py_obj, outofscope_func))
    {
      gdbpy_ref<> meth_result (PyObject_CallMethod (py_obj, outofscope_func,
						    NULL));
      if (meth_result == NULL)
	gdbpy_print_stack ();
    }

  delete_breakpoint (bpfinish_obj->py_bp.bp);
}

/* Callback for `bpfinishpy_detect_out_scope'.  Triggers Python's
   `B->out_of_scope' function if B is a FinishBreakpoint out of its scope.  */

static int
bpfinishpy_detect_out_scope_cb (struct breakpoint *b, void *args)
{
  struct breakpoint *bp_stopped = (struct breakpoint *) args;
  PyObject *py_bp = (PyObject *) b->py_bp_object;

  /* Trigger out_of_scope if this is a FinishBreakpoint and its frame is
     not anymore in the current callstack.  */
  if (py_bp != NULL && b->py_bp_object->is_finish_bp)
    {
      struct finish_breakpoint_object *finish_bp =
          (struct finish_breakpoint_object *) py_bp;

      /* Check scope if not currently stopped at the FinishBreakpoint.  */
      if (b != bp_stopped)
        {
          TRY
            {
              if (b->pspace == current_inferior ()->pspace
                  && (!target_has_registers
                      || frame_find_by_id (b->frame_id) == NULL))
                bpfinishpy_out_of_scope (finish_bp);
            }
          CATCH (except, RETURN_MASK_ALL)
            {
              gdbpy_convert_exception (except);
              gdbpy_print_stack ();
            }
	  END_CATCH
        }
    }

  return 0;
}

/* Attached to `stop' notifications, check if the execution has run
   out of the scope of any FinishBreakpoint before it has been hit.  */

static void
bpfinishpy_handle_stop (struct bpstats *bs, int print_frame)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  iterate_over_breakpoints (bpfinishpy_detect_out_scope_cb,
                            bs == NULL ? NULL : bs->breakpoint_at);
}

/* Attached to `exit' notifications, triggers all the necessary out of
   scope notifications.  */

static void
bpfinishpy_handle_exit (struct inferior *inf)
{
  gdbpy_enter enter_py (target_gdbarch (), current_language);

  iterate_over_breakpoints (bpfinishpy_detect_out_scope_cb, NULL);
}

/* Initialize the Python finish breakpoint code.  */

int
gdbpy_initialize_finishbreakpoints (void)
{
  if (PyType_Ready (&finish_breakpoint_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "FinishBreakpoint",
			      (PyObject *) &finish_breakpoint_object_type) < 0)
    return -1;

  observer_attach_normal_stop (bpfinishpy_handle_stop);
  observer_attach_inferior_exit (bpfinishpy_handle_exit);

  return 0;
}

static gdb_PyGetSetDef finish_breakpoint_object_getset[] = {
  { "return_value", bpfinishpy_get_returnvalue, NULL,
  "gdb.Value object representing the return value, if any. \
None otherwise.", NULL },
    { NULL }  /* Sentinel.  */
};

PyTypeObject finish_breakpoint_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.FinishBreakpoint",         /*tp_name*/
  sizeof (struct finish_breakpoint_object),  /*tp_basicsize*/
  0,                              /*tp_itemsize*/
  bpfinishpy_dealloc,             /*tp_dealloc*/
  0,                              /*tp_print*/
  0,                              /*tp_getattr*/
  0,                              /*tp_setattr*/
  0,                              /*tp_compare*/
  0,                              /*tp_repr*/
  0,                              /*tp_as_number*/
  0,                              /*tp_as_sequence*/
  0,                              /*tp_as_mapping*/
  0,                              /*tp_hash */
  0,                              /*tp_call*/
  0,                              /*tp_str*/
  0,                              /*tp_getattro*/
  0,                              /*tp_setattro */
  0,                              /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
  "GDB finish breakpoint object", /* tp_doc */
  0,                              /* tp_traverse */
  0,                              /* tp_clear */
  0,                              /* tp_richcompare */
  0,                              /* tp_weaklistoffset */
  0,                              /* tp_iter */
  0,                              /* tp_iternext */
  0,                              /* tp_methods */
  0,                              /* tp_members */
  finish_breakpoint_object_getset,/* tp_getset */
  &breakpoint_object_type,        /* tp_base */
  0,                              /* tp_dict */
  0,                              /* tp_descr_get */
  0,                              /* tp_descr_set */
  0,                              /* tp_dictoffset */
  bpfinishpy_init,                /* tp_init */
  0,                              /* tp_alloc */
  0                               /* tp_new */
};
