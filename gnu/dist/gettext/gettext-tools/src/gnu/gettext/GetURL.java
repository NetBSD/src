/* Fetch an URL's contents.
 * Copyright (C) 2001 Free Software Foundation, Inc.
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

package gnu.gettext;

import java.io.*;
import java.net.*;

/**
 * @author Bruno Haible
 */
public class GetURL {
  // Use a separate thread to signal a timeout error if the URL cannot
  // be accessed and completely read within a given amount of time.
  private static long timeout = 30*1000; // 30 seconds
  private boolean done;
  private Thread timeoutThread;
  public void fetch (String s) {
    URL url;
    try {
      url = new URL(s);
    } catch (MalformedURLException e) {
      System.exit(1);
      return;
    }
    // We always print something on stderr because the user should know
    // why we are trying to establish an internet connection.
    System.err.print("Retrieving "+s+"...");
    System.err.flush();
    done = false;
    timeoutThread =
      new Thread() {
        public void run () {
          try {
            sleep(timeout);
            if (!done) {
              System.err.println(" timed out.");
              System.exit(1);
            }
          } catch (InterruptedException e) {
          }
        }
      };
    timeoutThread.start();
    try {
      InputStream istream = new BufferedInputStream(url.openStream());
      OutputStream ostream = new BufferedOutputStream(System.out);
      for (;;) {
        int b = istream.read();
        if (b < 0) break;
        ostream.write(b);
      }
      ostream.close();
      System.out.flush();
      istream.close();
    } catch (IOException e) {
      //e.printStackTrace();
      System.err.println(" failed.");
      System.exit(1);
    }
    done = true;
    System.err.println(" done.");
  }
  public static void main (String[] args) {
    if (args.length != 1)
      System.exit(1);
    (new GetURL()).fetch(args[0]);
    System.exit(0);
  }
}
