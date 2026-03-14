--  Copyright 2008-2025 Free Software Foundation, Inc.
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

procedure P is
   --  Make this large enough to force it into memory with gnat-llvm.
   type Table is array (1 .. 15) of Integer;

   function Create (I : Integer) return Table is
   begin
      return (4 + I, 8 * I, 7 * I + 4, others => 72);
   end Create;

   A : Table := Create (7);
   C : Integer;
begin
   C := A (1) + A (2);  -- STOP
end P;

