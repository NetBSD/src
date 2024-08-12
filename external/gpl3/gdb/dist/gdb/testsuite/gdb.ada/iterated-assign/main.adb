--  Copyright 2024 Free Software Foundation, Inc.
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

with pck; use pck;

procedure Main is
   A1 : Other_Array_Type := (2, 4, 6, 8);
   A2 : MD_Array_Type := ((1, 2), (3, 4));
begin
   Do_Nothing (A1'Address);  -- STOP
   Do_Nothing (A2'Address);
end Main;
