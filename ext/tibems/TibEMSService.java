package org.jalonsoa.tibems;

import java.lang.Long;
import java.io.IOException;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyClass;
import org.jruby.RubyFixnum;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.load.BasicLibraryService;

import org.jalonsoa.tibems.Admin;

public class TibEMSService implements BasicLibraryService {
  private Ruby runtime;

  // Initial setup function. Takes a reference to the current JRuby runtime and
  // sets up our modules. For JRuby, we will define mask() as an instance method
  // on a specially created class, Faye::WebSocketMask.
  public boolean basicLoad(Ruby runtime) throws IOException {
    this.runtime = runtime;
    RubyModule tibems = runtime.defineModule("TibEMS");

    // Create the WebSocketMask class. defineClassUnder() takes a name, a
    // reference to the superclass -- runtime.getObject() gets you the Object
    // class for the current runtime -- and an allocator function that says
    // which Java object to constuct when you call new() on the class.
    RubyClass admin = tibems.defineClassUnder("Admin", runtime.getObject(), new ObjectAllocator() {
      public IRubyObject allocate(Ruby runtime, RubyClass rubyClass) {
        return new Admin(runtime, rubyClass);
      }
    });

    admin.defineAnnotatedMethods(Admin.class);
    return true;
  }
}
