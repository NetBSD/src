with Pck; use Pck;

procedure P is
   Small : Data_Small;
   Large : Data_Large;
begin
   Small := Create_Small;
   Large := Create_Large;
   Small (1) := Large (1);
end P;
