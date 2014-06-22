--  Copyright 2004-2014 Free Software Foundation, Inc.
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <http://www.gnu.org/licenses/>.

with System;

procedure Fixed_Points is

   ------------
   -- Test 1 --
   ------------

   --  Fixed point subtypes

   type Base_Fixed_Point_Type is
     delta 1.0 / 16.0
       range (System.Min_Int / 2) * 1.0 / 16.0 ..
       (System.Max_Int / 2) * 1.0 / 16.0;

   subtype Fixed_Point_Subtype is
     Base_Fixed_Point_Type range -50.0 .. 50.0;

   type New_Fixed_Point_Type is
     new Base_Fixed_Point_Type range -50.0 .. 50.0;

   Base_Object            : Base_Fixed_Point_Type := -50.0;
   Subtype_Object         : Fixed_Point_Subtype := -50.0;
   New_Type_Object        : New_Fixed_Point_Type := -50.0;


   ------------
   -- Test 2 --
   ------------

   --  Overprecise delta

   Overprecise_Delta : constant := 0.135791357913579;
   --  delta whose significant figures cannot be stored into a long.

   type Overprecise_Fixed_Point is
     delta Overprecise_Delta range 0.0 .. 200.0;
   for Overprecise_Fixed_Point'Small use Overprecise_Delta;

   Overprecise_Object : Overprecise_Fixed_Point :=
     Overprecise_Fixed_Point'Small;

begin
   Base_Object := 1.0/16.0;   -- Set breakpoint here
   Subtype_Object := 1.0/16.0;
   New_Type_Object := 1.0/16.0;
   Overprecise_Object := Overprecise_Fixed_Point'Small * 2;
end Fixed_Points;
