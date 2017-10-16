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
import org.jruby.anno.JRubyClass;
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
import com.tibco.tibjms.admin.StatData;
import com.tibco.tibjms.admin.TibjmsAdmin;
import com.tibco.tibjms.admin.TibjmsAdminException;

import java.util.Collection;
import java.util.logging.Level;
import java.util.logging.Logger;

// The Java class that backs the Ruby class Faye::WebSocketMask. Its methods
// annotated with @JRubyMethod become exposed as instance methods on the Ruby
// class through the call to defineAnnotatedMethods() above.
@JRubyClass(name="TibEMS::Admin")
public class Admin extends RubyObject {
  public static final int DESTINATION_TYPE_TOPIC = 0;
  public static final int DESTINATION_TYPE_QUEUE = 1;

  private static IRubyObject Qnil;

  private TibjmsAdmin admin = null;

  public Admin(final Ruby runtime, RubyClass rubyClass) {
    super(runtime, rubyClass);

    Qnil = runtime.getNil();
  }

  @JRubyMethod(name = "create")
  protected IRubyObject create(ThreadContext context, RubyString url, RubyString user, RubyString pass) throws TibjmsAdminException {
    try {
      this.admin = new TibjmsAdmin(url.asJavaString(),user.asJavaString(),pass.asJavaString());
    } catch (TibjmsAdminException exp) {
      throw exp;
    }

    return this;
  }

  @JRubyMethod(name = "close")
  public IRubyObject close(ThreadContext context) throws TibjmsAdminException {
    try {
      this.admin.close();
      this.admin = null;
    } catch (TibjmsAdminException exp) {
      throw exp;
    }
    return Qnil;
  }

  private RubyArray destinationsHash( Ruby runtime, DestinationInfo[] destInfos, int type ) {
    RubyArray info = RubyArray.newArray( runtime, destInfos.length );

    for (DestinationInfo destInfo : destInfos) {
      RubyHash dest = RubyHash.newHash( runtime );
      RubyHash inbound = RubyHash.newHash( runtime );
      RubyHash outbound = RubyHash.newHash( runtime );

      dest.put("name",destInfo.getName());

      dest.put("consumerCount", destInfo.getConsumerCount());

      dest.put("flowControlMaxBytes", destInfo.getFlowControlMaxBytes());

      dest.put("maxBytes", destInfo.getMaxBytes());
      dest.put("maxMsgs", destInfo.getMaxMsgs());

      dest.put("pendingMessageCount", destInfo.getPendingMessageCount());
      dest.put("pendingMessageSize", destInfo.getPendingMessageSize());

      dest.put("pendingPersistentMessageCount", destInfo.getPendingPersistentMessageCount());
      dest.put("pendingPersistentMessageSize", destInfo.getPendingPersistentMessageSize());

      if (type == DESTINATION_TYPE_TOPIC) {
        TopicInfo topicInfo = (TopicInfo)destInfo;
        dest.put("activeDurableCount", topicInfo.getActiveDurableCount());
        dest.put("durableCount", topicInfo.getDurableCount());
        dest.put("subscriberCount", topicInfo.getSubscriberCount());
      } else {
        QueueInfo queueInfo = (QueueInfo)destInfo;
        dest.put("deliveredMessageCount", queueInfo.getDeliveredMessageCount());
        dest.put("inTransitMessageCount", queueInfo.getInTransitMessageCount());
        dest.put("maxRedelivery", queueInfo.getMaxRedelivery());
        dest.put("receiverCount", queueInfo.getReceiverCount());
      }

      StatData inboundStats = destInfo.getInboundStatistics();
      inbound.put("totalMessages", inboundStats.getTotalMessages());
      inbound.put("messageRate", inboundStats.getMessageRate());
      inbound.put("totalBytes", inboundStats.getTotalBytes());
      inbound.put("byteRate", inboundStats.getByteRate());

      dest.put("inbound", inbound);

      StatData outboundStats = destInfo.getOutboundStatistics();
      outbound.put("totalMessages", outboundStats.getTotalMessages());
      outbound.put("messageRate", outboundStats.getMessageRate());
      outbound.put("totalBytes", outboundStats.getTotalBytes());
      outbound.put("byteRate", outboundStats.getByteRate());

      dest.put("outbound", outbound);

      info.add(dest);
    }

    return info;
  }

  @JRubyMethod(name = "get_info")
  public IRubyObject get_info(ThreadContext context) throws TibjmsAdminException {
    Ruby runtime = context.runtime;
    RubyHash info = RubyHash.newHash( runtime );

    try {
      ServerInfo serverInfo = this.admin.getInfo();

      info.put("asyncDBSize", serverInfo.getAsyncDBSize());
      info.put("connectionCount", serverInfo.getConnectionCount());
      info.put("consumerCount", serverInfo.getConsumerCount());
      info.put("diskReadOperationsRate", serverInfo.getDiskReadOperationsRate());
      info.put("diskReadRate", serverInfo.getDiskReadRate());
      info.put("diskWriteOperationsRate", serverInfo.getDiskWriteOperationsRate());
      info.put("diskWriteRate", serverInfo.getDiskWriteRate());
      info.put("durableCount", serverInfo.getDurableCount());
      info.put("inboundBytesRate", serverInfo.getInboundBytesRate());
      info.put("inboundMessageCount", serverInfo.getInboundMessageCount());
      info.put("inboundMessageRate", serverInfo.getInboundMessageRate());
      info.put("maxConnections", serverInfo.getMaxConnections());
      info.put("maxMsgMemory", serverInfo.getMaxMsgMemory());
      info.put("msgMem", serverInfo.getMsgMem());
      info.put("msgMemPooled", serverInfo.getMsgMemPooled());
      info.put("outboundBytesRate", serverInfo.getOutboundBytesRate());
      info.put("outboundMessageCount", serverInfo.getOutboundMessageCount());
      info.put("outboundMessageRate", serverInfo.getOutboundMessageRate());
      info.put("pendingMessageCount", serverInfo.getPendingMessageCount());
      info.put("pendingMessageSize", serverInfo.getPendingMessageSize());
      info.put("producerCount", serverInfo.getProducerCount());
      info.put("queueCount", serverInfo.getQueueCount());
      info.put("reserveMemory", serverInfo.getReserveMemory());
      info.put("sessionCount", serverInfo.getSessionCount());
      info.put("syncDBSize", serverInfo.getSyncDBSize());
      info.put("topicCount", serverInfo.getTopicCount());

      // TODO: pass pattern
      QueueInfo[] queueInfos = this.admin.getQueuesStatistics();

      if (queueInfos.length > 0) {
        RubyArray queues = destinationsHash( runtime, queueInfos, DESTINATION_TYPE_QUEUE );

        info.put("queues", queues);
      }

      // TODO: pass pattern
      TopicInfo[] topicInfos = this.admin.getTopicsStatistics();

      if (topicInfos.length > 0) {
        RubyArray topics = destinationsHash( runtime, topicInfos, DESTINATION_TYPE_TOPIC );

        info.put("topics", topics);
      }

    } catch (TibjmsAdminException exp) {
      throw exp;
    }

    return info;
  }
}
