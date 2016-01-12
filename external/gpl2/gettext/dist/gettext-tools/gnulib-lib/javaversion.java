/* Show the Java version.
 * Copyright (C) 2006 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
 * This program shows the Java version.
 *
 * This program _must_ be compiled with
 *   javac -d . -target 1.1 javaversion.java
 * since its purpose is to show the version of _any_ Java implementation.
 *
 * @author Bruno Haible
 */
public class javaversion {
  public static void main (String[] args) {
    System.out.println(System.getProperty("java.specification.version"));
  }
}
