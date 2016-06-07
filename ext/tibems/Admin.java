package org.jalonsoa.tibems;

import java.lang.Long;
import java.io.IOException;

import org.jruby.Ruby;
import org.jruby.RubyArray;
import org.jruby.RubyClass;
import org.jruby.RubyFixnum;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.RubyString;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.load.BasicLibraryService;

import com.tibco.tibjms.admin.ConsumerInfo;
import com.tibco.tibjms.admin.DestinationInfo;
import com.tibco.tibjms.admin.QueueInfo;
import com.tibco.tibjms.admin.TopicInfo;
import com.tibco.tibjms.admin.ServerInfo;
import com.tibco.tibjms.admin.TibjmsAdmin;
import com.tibco.tibjms.admin.TibjmsAdminException;

import java.util.Collection;
import java.util.logging.Level;
import java.util.logging.Logger;

// The Java class that backs the Ruby class Faye::WebSocketMask. Its methods
// annotated with @JRubyMethod become exposed as instance methods on the Ruby
// class through the call to defineAnnotatedMethods() above.
public class Admin extends RubyObject {
  private static TibjmsAdmin admin = null;

  private static IRubyObject Qnil;

  public Admin(final Ruby runtime, RubyClass rubyClass) {
    super(runtime, rubyClass);

    Qnil = runtime.getNil();
  }

  @JRubyMethod
  public static IRubyObject create(ThreadContext context, IRubyObject self, RubyString url, RubyString user, RubyString pass) {
    try {
      admin = new TibjmsAdmin(url.asJavaString(),user.asJavaString(),pass.asJavaString());
    } catch (TibjmsAdminException exp) {
    }

    return self;
  }

  @JRubyMethod
  public IRubyObject close(ThreadContext context) {
    try {
      admin.close();
    } catch (TibjmsAdminException exp) {
    }
    return Qnil;
  }

  @JRubyMethod
  public IRubyObject get_info(ThreadContext context) {
    Ruby runtime = context.runtime;
    RubyHash info = RubyHash.newHash( runtime );

    try {
      ServerInfo serverInfo = admin.getInfo();

      info.put("diskReadRate", serverInfo.getDiskReadRate());
      info.put("diskWriteRate", serverInfo.getDiskWriteRate());
      info.put("syncDBSize", serverInfo.getSyncDBSize());
      info.put("asyncDBSize", serverInfo.getAsyncDBSize());
      info.put("msgMem", serverInfo.getMsgMem());
      info.put("msgMemPooled", serverInfo.getMsgMemPooled());
      info.put("maxMsgMemory", serverInfo.getMaxMsgMemory());
      info.put("msgMem", serverInfo.getMsgMem());
      info.put("connectionCount", serverInfo.getConnectionCount());
      info.put("maxConnections", serverInfo.getMaxConnections());
      info.put("sessionCount", serverInfo.getSessionCount());
      info.put("producerCount", serverInfo.getProducerCount());
      info.put("consumerCount", serverInfo.getConsumerCount());
      info.put("durableCount", serverInfo.getDurableCount());
      info.put("topicCount", serverInfo.getTopicCount());
      info.put("queueCount", serverInfo.getQueueCount());
      info.put("pendingMessageCount", serverInfo.getPendingMessageCount());
      info.put("pendingMessageSize", serverInfo.getPendingMessageSize());
      info.put("inboundMessageCount", serverInfo.getInboundMessageCount());
      info.put("inboundMessageRate", serverInfo.getInboundMessageRate());
      info.put("inboundBytesRate", serverInfo.getInboundBytesRate());
      info.put("outboundMessageCount", serverInfo.getOutboundMessageCount());
      info.put("outboundMessageRate", serverInfo.getOutboundMessageRate());
      info.put("outboundBytesRate", serverInfo.getOutboundBytesRate());
      info.put("logFileMaxSize", serverInfo.getLogFileMaxSize());
      info.put("logFileSize", serverInfo.getLogFileSize());

      // TODO: pass pattern
      QueueInfo[] queueInfos = admin.getQueuesStatistics();

      // TODO: pass pattern
      TopicInfo[] topicInfos = admin.getTopicsStatistics();

    } catch (TibjmsAdminException exp) {
    }

    return info;
  }
}
