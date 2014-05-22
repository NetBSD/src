// jprint.java test program.
//
// Copyright 2004-2013 Free Software Foundation, Inc.
//
// Written by Jeff Johnston <jjohnstn@redhat.com> 
// Contributed by Red Hat
//
// This file is part of GDB.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

import java.util.Properties;

class jvclass {
  public static int k;
  static {
    k = 77;
  }
  public static int addprint (int x, int y, int z) {
    int sum = x + y + z;
    System.out.println ("sum is " + sum);
    return sum;
  }

  public int addk (int x) {
    int sum = x + k;
    System.out.println ("adding k gives " + sum);
    return sum;
  }
}
    
public class jprint extends jvclass {
  public static Properties props = new Properties ();
  public static String hi = "hi maude";

  public int dothat (int x) {
    int y = x + 3;
    System.out.println ("new value is " + y);
    return y + 4;
  }
  public static int print (int x) {
    System.out.println("x is " + x);
    return x;
  }
  public static int print (int x, int y) {
    System.out.println("y is " + y);
    return y;
  }
  public static void main(String[] args) {
    jprint x = new jprint ();
    x.dothat (44);
    print (k, 33);
    print (x.addk(0), 33);
  }
}


