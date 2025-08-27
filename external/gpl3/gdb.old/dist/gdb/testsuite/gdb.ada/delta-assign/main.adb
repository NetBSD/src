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
   Local : Record_Type := (A => 1, B => 2);
   A : Array_Type := (1, 3, 5);
begin
   Do_Nothing (Local'Address);  -- STOP
   Do_Nothing (A'Address);
end Main;
