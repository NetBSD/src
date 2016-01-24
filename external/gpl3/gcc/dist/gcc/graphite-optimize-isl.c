/* A scheduling optimizer for Graphite
   Copyright (C) 2012-2015 Free Software Foundation, Inc.
   Contributed by Tobias Grosser <tobias@grosser.es>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"

#ifdef HAVE_isl
#include <isl/constraint.h>
#include <isl/set.h>
#include <isl/union_set.h>
#include <isl/map.h>
#include <isl/union_map.h>
#include <isl/schedule.h>
#include <isl/band.h>
#include <isl/aff.h>
#include <isl/options.h>
#endif

#include "system.h"
#include "coretypes.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "options.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "fold-const.h"
#include "predict.h"
#include "tm.h"
#include "hard-reg-set.h"
#include "input.h"
#include "function.h"
#include "dominance.h"
#include "cfg.h"
#include "basic-block.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "tree-ssa-loop.h"
#include "dumpfile.h"
#include "cfgloop.h"
#include "tree-chrec.h"
#include "tree-data-ref.h"
#include "tree-scalar-evolution.h"
#include "sese.h"

#ifdef HAVE_isl
#include "graphite-poly.h"

static isl_union_set *
scop_get_domains (scop_p scop ATTRIBUTE_UNUSED)
{
  int i;
  poly_bb_p pbb;
  isl_space *space = isl_set_get_space (scop->context);
  isl_union_set *res = isl_union_set_empty (space);

  FOR_EACH_VEC_ELT (scop->bbs, i, pbb)
    res = isl_union_set_add_set (res, isl_set_copy (pbb->domain));

  return res;
}

/* getTileMap - Create a map that describes a n-dimensonal tiling.
  
   getTileMap creates a map from a n-dimensional scattering space into an
   2*n-dimensional scattering space. The map describes a rectangular tiling.
  
   Example:
     scheduleDimensions = 2, parameterDimensions = 1, tileSize = 32
 
    tileMap := [p0] -> {[s0, s1] -> [t0, t1, s0, s1]:
                         t0 % 32 = 0 and t0 <= s0 < t0 + 32 and
                         t1 % 32 = 0 and t1 <= s1 < t1 + 32}
 
   Before tiling:
 
   for (i = 0; i < N; i++)
     for (j = 0; j < M; j++)
 	S(i,j)
 
   After tiling:
 
   for (t_i = 0; t_i < N; i+=32)
     for (t_j = 0; t_j < M; j+=32)
 	for (i = t_i; i < min(t_i + 32, N); i++)  | Unknown that N % 32 = 0
 	  for (j = t_j; j < t_j + 32; j++)        |   Known that M % 32 = 0
 	    S(i,j)
   */
 
static isl_basic_map *
getTileMap (isl_ctx *ctx, int scheduleDimensions, int tileSize)
{
  int x;
  /* We construct

     tileMap := [p0] -> {[s0, s1] -> [t0, t1, p0, p1, a0, a1]:
    	                s0 = a0 * 32 and s0 = p0 and t0 <= p0 < t0 + 32 and
    	                s1 = a1 * 32 and s1 = p1 and t1 <= p1 < t1 + 32}

     and project out the auxilary dimensions a0 and a1.  */
  isl_space *Space = isl_space_alloc (ctx, 0, scheduleDimensions,
				      scheduleDimensions * 3);
  isl_basic_map *tileMap = isl_basic_map_universe (isl_space_copy (Space));

  isl_local_space *LocalSpace = isl_local_space_from_space (Space);

  for (x = 0; x < scheduleDimensions; x++)
    {
      int sX = x;
      int tX = x;
      int pX = scheduleDimensions + x;
      int aX = 2 * scheduleDimensions + x;

      isl_constraint *c;

      /* sX = aX * tileSize; */
      c = isl_equality_alloc (isl_local_space_copy (LocalSpace));
      isl_constraint_set_coefficient_si (c, isl_dim_out, sX, 1);
      isl_constraint_set_coefficient_si (c, isl_dim_out, aX, -tileSize);
      tileMap = isl_basic_map_add_constraint (tileMap, c);

      /* pX = sX; */
      c = isl_equality_alloc (isl_local_space_copy (LocalSpace));
      isl_constraint_set_coefficient_si (c, isl_dim_out, pX, 1);
      isl_constraint_set_coefficient_si (c, isl_dim_in, sX, -1);
      tileMap = isl_basic_map_add_constraint (tileMap, c);

      /* tX <= pX */
      c = isl_inequality_alloc (isl_local_space_copy (LocalSpace));
      isl_constraint_set_coefficient_si (c, isl_dim_out, pX, 1);
      isl_constraint_set_coefficient_si (c, isl_dim_out, tX, -1);
      tileMap = isl_basic_map_add_constraint (tileMap, c);

      /* pX <= tX + (tileSize - 1) */
      c = isl_inequality_alloc (isl_local_space_copy (LocalSpace));
      isl_constraint_set_coefficient_si (c, isl_dim_out, tX, 1);
      isl_constraint_set_coefficient_si (c, isl_dim_out, pX, -1);
      isl_constraint_set_constant_si (c, tileSize - 1);
      tileMap = isl_basic_map_add_constraint (tileMap, c);
    }

  /* Project out auxiliary dimensions.

     The auxiliary dimensions are transformed into existentially quantified ones.
     This reduces the number of visible scattering dimensions and allows Cloog
     to produces better code.  */
  tileMap = isl_basic_map_project_out (tileMap, isl_dim_out,
				       2 * scheduleDimensions,
				       scheduleDimensions);
  isl_local_space_free (LocalSpace);
  return tileMap;
}

/* getScheduleForBand - Get the schedule for this band.
  
   Polly applies transformations like tiling on top of the isl calculated value.
   This can influence the number of scheduling dimension. The number of
   schedule dimensions is returned in the parameter 'Dimension'.  */
static bool DisableTiling = false;

static isl_union_map *
getScheduleForBand (isl_band *Band, int *Dimensions)
{
  isl_union_map *PartialSchedule;
  isl_ctx *ctx;
  isl_space *Space;
  isl_basic_map *TileMap;
  isl_union_map *TileUMap;

  PartialSchedule = isl_band_get_partial_schedule (Band);
  *Dimensions = isl_band_n_member (Band);

  if (DisableTiling || flag_loop_unroll_jam)
    return PartialSchedule;

  /* It does not make any sense to tile a band with just one dimension.  */
  if (*Dimensions == 1)
    return PartialSchedule;

  ctx = isl_union_map_get_ctx (PartialSchedule);
  Space = isl_union_map_get_space (PartialSchedule);

  TileMap = getTileMap (ctx, *Dimensions, 32);
  TileUMap = isl_union_map_from_map (isl_map_from_basic_map (TileMap));
  TileUMap = isl_union_map_align_params (TileUMap, Space);
  *Dimensions = 2 * *Dimensions;

  return isl_union_map_apply_range (PartialSchedule, TileUMap);
}

/* Create a map that pre-vectorizes one scheduling dimension.
  
   getPrevectorMap creates a map that maps each input dimension to the same
   output dimension, except for the dimension DimToVectorize. DimToVectorize is
   strip mined by 'VectorWidth' and the newly created point loop of
   DimToVectorize is moved to the innermost level.
  
   Example (DimToVectorize=0, ScheduleDimensions=2, VectorWidth=4):
  
   | Before transformation
   |
   | A[i,j] -> [i,j]
   |
   | for (i = 0; i < 128; i++)
   |    for (j = 0; j < 128; j++)
   |      A(i,j);
  
     Prevector map:
     [i,j] -> [it,j,ip] : it % 4 = 0 and it <= ip <= it + 3 and i = ip
  
   | After transformation:
   |
   | A[i,j] -> [it,j,ip] : it % 4 = 0 and it <= ip <= it + 3 and i = ip
   |
   | for (it = 0; it < 128; it+=4)
   |    for (j = 0; j < 128; j++)
   |      for (ip = max(0,it); ip < min(128, it + 3); ip++)
   |        A(ip,j);
  
   The goal of this transformation is to create a trivially vectorizable loop.
   This means a parallel loop at the innermost level that has a constant number
   of iterations corresponding to the target vector width.
  
   This transformation creates a loop at the innermost level. The loop has a
   constant number of iterations, if the number of loop iterations at
   DimToVectorize can be devided by VectorWidth. The default VectorWidth is
   currently constant and not yet target specific. This function does not reason
   about parallelism.

  */
static isl_map *
getPrevectorMap (isl_ctx *ctx, int DimToVectorize,
		 int ScheduleDimensions,
		 int VectorWidth)
{
  isl_space *Space;
  isl_local_space *LocalSpace, *LocalSpaceRange;
  isl_set *Modulo;
  isl_map *TilingMap;
  isl_constraint *c;
  isl_aff *Aff;
  int PointDimension; /* ip */
  int TileDimension;  /* it */
  isl_val *VectorWidthMP;
  int i;

  /* assert (0 <= DimToVectorize && DimToVectorize < ScheduleDimensions);*/

  Space = isl_space_alloc (ctx, 0, ScheduleDimensions, ScheduleDimensions + 1);
  TilingMap = isl_map_universe (isl_space_copy (Space));
  LocalSpace = isl_local_space_from_space (Space);
  PointDimension = ScheduleDimensions;
  TileDimension = DimToVectorize;

  /* Create an identity map for everything except DimToVectorize and map
     DimToVectorize to the point loop at the innermost dimension.  */
  for (i = 0; i < ScheduleDimensions; i++)
    {
      c = isl_equality_alloc (isl_local_space_copy (LocalSpace));
      isl_constraint_set_coefficient_si (c, isl_dim_in, i, -1);

      if (i == DimToVectorize)
	isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, 1);
      else
	isl_constraint_set_coefficient_si (c, isl_dim_out, i, 1);

      TilingMap = isl_map_add_constraint (TilingMap, c);
    }

  /* it % 'VectorWidth' = 0  */
  LocalSpaceRange = isl_local_space_range (isl_local_space_copy (LocalSpace));
  Aff = isl_aff_zero_on_domain (LocalSpaceRange);
  Aff = isl_aff_set_constant_si (Aff, VectorWidth);
  Aff = isl_aff_set_coefficient_si (Aff, isl_dim_in, TileDimension, 1);

  VectorWidthMP = isl_val_int_from_si (ctx, VectorWidth);
  Aff = isl_aff_mod_val (Aff, VectorWidthMP);
  Modulo = isl_pw_aff_zero_set (isl_pw_aff_from_aff (Aff));
  TilingMap = isl_map_intersect_range (TilingMap, Modulo);

  /* it <= ip */
  c = isl_inequality_alloc (isl_local_space_copy (LocalSpace));
  isl_constraint_set_coefficient_si (c, isl_dim_out, TileDimension, -1);
  isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, 1);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  /* ip <= it + ('VectorWidth' - 1) */
  c = isl_inequality_alloc (LocalSpace);
  isl_constraint_set_coefficient_si (c, isl_dim_out, TileDimension, 1);
  isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, -1);
  isl_constraint_set_constant_si (c, VectorWidth - 1);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  return TilingMap;
}

/* Compute an auxiliary map to getPrevectorMap, for computing the separating 
   class defined by full tiles.  Used in graphite_isl_ast_to_gimple.c to set the 
   corresponding option for AST build.

   The map (for VectorWidth=4):

   [i,j] -> [it,j,ip] : it % 4 = 0 and it <= ip <= it + 3 and it + 3 = i and
                        ip >= 0

   The image of this map is the separation class. The range of this map includes
   all the i multiple of 4 in the domain such as i + 3 is in the domain too.
    
 */ 
static isl_map *
getPrevectorMap_full (isl_ctx *ctx, int DimToVectorize,
		 int ScheduleDimensions,
		 int VectorWidth)
{
  isl_space *Space;
  isl_local_space *LocalSpace, *LocalSpaceRange;
  isl_set *Modulo;
  isl_map *TilingMap;
  isl_constraint *c;
  isl_aff *Aff;
  int PointDimension; /* ip */
  int TileDimension;  /* it */
  isl_val *VectorWidthMP;
  int i;

  /* assert (0 <= DimToVectorize && DimToVectorize < ScheduleDimensions);*/

  Space = isl_space_alloc (ctx, 0, ScheduleDimensions, ScheduleDimensions + 1);
  TilingMap = isl_map_universe (isl_space_copy (Space));
  LocalSpace = isl_local_space_from_space (Space);
  PointDimension = ScheduleDimensions;
  TileDimension = DimToVectorize;

  /* Create an identity map for everything except DimToVectorize and the 
     point loop. */
  for (i = 0; i < ScheduleDimensions; i++)
    {
      if (i == DimToVectorize)
        continue;

      c = isl_equality_alloc (isl_local_space_copy (LocalSpace));

      isl_constraint_set_coefficient_si (c, isl_dim_in, i, -1);
      isl_constraint_set_coefficient_si (c, isl_dim_out, i, 1);

      TilingMap = isl_map_add_constraint (TilingMap, c);
    }

  /* it % 'VectorWidth' = 0  */
  LocalSpaceRange = isl_local_space_range (isl_local_space_copy (LocalSpace));
  Aff = isl_aff_zero_on_domain (LocalSpaceRange);
  Aff = isl_aff_set_constant_si (Aff, VectorWidth);
  Aff = isl_aff_set_coefficient_si (Aff, isl_dim_in, TileDimension, 1);

  VectorWidthMP = isl_val_int_from_si (ctx, VectorWidth);
  Aff = isl_aff_mod_val (Aff, VectorWidthMP);
  Modulo = isl_pw_aff_zero_set (isl_pw_aff_from_aff (Aff));
  TilingMap = isl_map_intersect_range (TilingMap, Modulo);

  /* it + ('VectorWidth' - 1) = i0  */
  c = isl_equality_alloc (isl_local_space_copy(LocalSpace));
  isl_constraint_set_coefficient_si (c, isl_dim_out, TileDimension,-1);
  isl_constraint_set_coefficient_si (c, isl_dim_in, TileDimension, 1);
  isl_constraint_set_constant_si (c, -VectorWidth + 1);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  /* ip >= 0 */
  c = isl_inequality_alloc (isl_local_space_copy (LocalSpace));
  isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, 1);
  isl_constraint_set_constant_si (c, 0);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  /* it <= ip */
  c = isl_inequality_alloc (isl_local_space_copy (LocalSpace));
  isl_constraint_set_coefficient_si (c, isl_dim_out, TileDimension, -1);
  isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, 1);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  /* ip <= it + ('VectorWidth' - 1) */
  c = isl_inequality_alloc (LocalSpace);
  isl_constraint_set_coefficient_si (c, isl_dim_out, TileDimension, 1);
  isl_constraint_set_coefficient_si (c, isl_dim_out, PointDimension, -1);
  isl_constraint_set_constant_si (c, VectorWidth - 1);
  TilingMap = isl_map_add_constraint (TilingMap, c);

  return TilingMap;
}

static bool EnablePollyVector = false;

/* getScheduleForBandList - Get the scheduling map for a list of bands.
    
   We walk recursively the forest of bands to combine the schedules of the
   individual bands to the overall schedule. In case tiling is requested,
   the individual bands are tiled.
   For unroll and jam the map the schedule for full tiles of the unrolled
   dimnesion is computed.  */
static isl_union_map *
getScheduleForBandList (isl_band_list *BandList, isl_union_map **map_sepcl)
{
  int NumBands, i;
  isl_union_map *Schedule;
  isl_ctx *ctx;

  ctx = isl_band_list_get_ctx (BandList);
  NumBands = isl_band_list_n_band (BandList);
  Schedule = isl_union_map_empty (isl_space_params_alloc (ctx, 0));

  for (i = 0; i < NumBands; i++)
    {
      isl_band *Band;
      isl_union_map *PartialSchedule;
      int ScheduleDimensions;
      isl_space *Space;

      isl_union_map *PartialSchedule_f;

      Band = isl_band_list_get_band (BandList, i);
      PartialSchedule = getScheduleForBand (Band, &ScheduleDimensions);
      Space = isl_union_map_get_space (PartialSchedule);

      PartialSchedule_f = NULL;

      if (isl_band_has_children (Band))
	{
	  isl_band_list *Children;
	  isl_union_map *SuffixSchedule;

	  Children = isl_band_get_children (Band);
	  SuffixSchedule = getScheduleForBandList (Children, map_sepcl);
	  PartialSchedule = isl_union_map_flat_range_product (PartialSchedule,
							      SuffixSchedule);
	  isl_band_list_free (Children);
	}
      else if (EnablePollyVector || flag_loop_unroll_jam)
	{
	  int i;
	  int depth;
 
 	  depth = PARAM_VALUE (PARAM_LOOP_UNROLL_JAM_DEPTH);
  
	  for (i = ScheduleDimensions - 1 ;  i >= 0 ; i--)
	    {
	      if (flag_loop_unroll_jam && (i != (ScheduleDimensions - depth)))
		continue;

#ifdef HAVE_ISL_SCHED_CONSTRAINTS_COMPUTE_SCHEDULE
	      if (isl_band_member_is_coincident (Band, i))
#else
	      if (isl_band_member_is_zero_distance (Band, i))
#endif
		{
		  isl_map *TileMap;
		  isl_union_map *TileUMap;
		  int stride;

                  stride = PARAM_VALUE (PARAM_LOOP_UNROLL_JAM_SIZE);    

		  TileMap = getPrevectorMap_full (ctx, i, ScheduleDimensions, 
						  stride); 
 		  TileUMap = isl_union_map_from_map (TileMap);
		  TileUMap = isl_union_map_align_params
		    (TileUMap, isl_space_copy (Space));
		  PartialSchedule_f = isl_union_map_apply_range
		    (isl_union_map_copy (PartialSchedule), TileUMap);

		  TileMap = getPrevectorMap (ctx, i, ScheduleDimensions, stride);
		  TileUMap = isl_union_map_from_map (TileMap);
		  TileUMap = isl_union_map_align_params
		    (TileUMap, isl_space_copy (Space));
		  PartialSchedule = isl_union_map_apply_range
		    (PartialSchedule, TileUMap);
		  break;
		}	
	    }
	}
      Schedule = isl_union_map_union (Schedule, 
                                      isl_union_map_copy(PartialSchedule));

      isl_band_free (Band);
      isl_space_free (Space);

      if (!flag_loop_unroll_jam)
	{
          isl_union_map_free (PartialSchedule);
          continue;
	}

      if (PartialSchedule_f)
	{
	  *map_sepcl = isl_union_map_union (*map_sepcl, PartialSchedule_f);
          isl_union_map_free (PartialSchedule);
	}
      else
        *map_sepcl = isl_union_map_union (*map_sepcl, PartialSchedule);
    }

  return Schedule;
}

static isl_union_map *
getScheduleMap (isl_schedule *Schedule, isl_union_map **map_sepcl)
{
  isl_band_list *BandList = isl_schedule_get_band_forest (Schedule);
  isl_union_map *ScheduleMap = getScheduleForBandList (BandList, map_sepcl);
  isl_band_list_free (BandList);
  return ScheduleMap;
}

static isl_stat
getSingleMap (__isl_take isl_map *map, void *user)
{
  isl_map **singleMap = (isl_map **) user;
  *singleMap = map;

  return isl_stat_ok;
}

static void
apply_schedule_map_to_scop (scop_p scop, isl_union_map *schedule_map, bool sepcl)
{
  int i;
  poly_bb_p pbb;

  FOR_EACH_VEC_ELT (scop->bbs, i, pbb)
    {
      isl_set *domain = isl_set_copy (pbb->domain);
      isl_union_map *stmtBand;
      isl_map *stmtSchedule;

      stmtBand = isl_union_map_intersect_domain
	(isl_union_map_copy (schedule_map),
	 isl_union_set_from_set (domain));
      isl_union_map_foreach_map (stmtBand, getSingleMap, &stmtSchedule);

      if (!sepcl)
	{
	  isl_map_free (pbb->transformed);
	  pbb->transformed = stmtSchedule;
	}
      else
	  pbb->map_sepclass = stmtSchedule;

      isl_union_map_free (stmtBand);
    }
}

static const int CONSTANT_BOUND = 20;

bool
optimize_isl (scop_p scop)
{

  isl_schedule *schedule;
#ifdef HAVE_ISL_SCHED_CONSTRAINTS_COMPUTE_SCHEDULE
  isl_schedule_constraints *schedule_constraints;
#endif
  isl_union_set *domain;
  isl_union_map *validity, *proximity, *dependences;
  isl_union_map *schedule_map;
  isl_union_map *schedule_map_f;

  domain = scop_get_domains (scop);
  dependences = scop_get_dependences (scop);
  dependences = isl_union_map_gist_domain (dependences,
					   isl_union_set_copy (domain));
  dependences = isl_union_map_gist_range (dependences,
					  isl_union_set_copy (domain));
  validity = dependences;

  proximity = isl_union_map_copy (validity);

#ifdef HAVE_ISL_SCHED_CONSTRAINTS_COMPUTE_SCHEDULE
  schedule_constraints = isl_schedule_constraints_on_domain (domain);
  schedule_constraints
	= isl_schedule_constraints_set_proximity (schedule_constraints,
						  proximity);
  schedule_constraints
	= isl_schedule_constraints_set_validity (schedule_constraints,
						 isl_union_map_copy (validity));
  schedule_constraints
	= isl_schedule_constraints_set_coincidence (schedule_constraints,
						    validity);
#endif

  isl_options_set_schedule_max_constant_term (scop->ctx, CONSTANT_BOUND);
  isl_options_set_schedule_maximize_band_depth (scop->ctx, 1);
#ifdef HAVE_ISL_OPTIONS_SET_SCHEDULE_SERIALIZE_SCCS
  isl_options_set_schedule_serialize_sccs (scop->ctx, 1);
#else
  isl_options_set_schedule_fuse (scop->ctx, ISL_SCHEDULE_FUSE_MIN);
#endif
  isl_options_set_on_error (scop->ctx, ISL_ON_ERROR_CONTINUE);

#ifdef HAVE_ISL_SCHED_CONSTRAINTS_COMPUTE_SCHEDULE
  schedule = isl_schedule_constraints_compute_schedule(schedule_constraints);
#else
  schedule = isl_union_set_compute_schedule (domain, validity, proximity);
#endif

  isl_options_set_on_error (scop->ctx, ISL_ON_ERROR_ABORT);

  if (!schedule)
    return false;

  schedule_map_f = isl_union_map_empty (isl_space_params_alloc (scop->ctx, 0));
  schedule_map = getScheduleMap (schedule, &schedule_map_f);

  apply_schedule_map_to_scop (scop, schedule_map, false);
  if (!isl_union_map_is_empty (schedule_map_f))
    apply_schedule_map_to_scop (scop, schedule_map_f, true);
  isl_union_map_free (schedule_map_f);

  isl_schedule_free (schedule);
  isl_union_map_free (schedule_map);

  return true;
}

#endif
