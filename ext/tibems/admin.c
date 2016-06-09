#include <tibems_ext.h>

#include <time.h>
#include <errno.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>

VALUE cTibEMSAdmin;
extern VALUE mTibEMS, cTibEMSError;
static VALUE sym_queue, sym_topic, sym_producer, sym_consumer, sym_queues, sym_topics;
static VALUE sym_dest_name, sym_dest_deliveredMessageCount, sym_dest_flowControlMaxBytes,
  sym_dest_maxBytes, sym_dest_maxMsgs, sym_dest_pendingMessageCount, sym_dest_pendingMessageSize,
  sym_dest_pendingPersistentMessageCount, sym_dest_activeDurableCount, sym_dest_durableCount,
  sym_dest_receiverCount, sym_dest_subscriberCount, sym_dest_byteRate, sym_dest_messageRate,
  sym_dest_totalBytes, sym_dest_totalMessages, sym_inbound, sym_outbound;

static ID intern_brackets, intern_merge, intern_merge_bang, intern_new_with_args;

#ifndef HAVE_RB_HASH_DUP
VALUE rb_hash_dup(VALUE other) {
  return rb_funcall(rb_cHash, intern_brackets, 1, other);
}
#endif

#define REQUIRE_INITIALIZED(wrapper) \
  if (!wrapper->initialized) { \
    rb_raise(cTibEMSError, "tibems admin is not initialized"); \
  }

#define REQUIRE_NOT_CONNECTED(wrapper) \
  REQUIRE_INITIALIZED(wrapper) \
  if (wrapper->connected) { \
    rb_raise(cTibEMSError, "tibems admin connection is already open"); \
  }

struct nogvl_create_args {
  tibemsAdmin *admin;
  const char *url;
  const char *user;
  const char *passwd;
/* TODO
  tibemsSSLParams ssl;
*/
};

/*
 * used to pass all arguments to mysql_send_query while inside
 * rb_thread_call_without_gvl
 */
struct nogvl_get_queue_info_args {
  tibemsAdmin *admin;
  VALUE info;
  char *queue;
};

static void rb_tibems_admin_mark(void * wrapper) {
  tibems_admin_wrapper * w = wrapper;
  if (w) {
    rb_gc_mark(w->encoding);
    rb_gc_mark(w->active_thread);
  }
}

static VALUE rb_raise_tibems_admin_error(tibems_admin_wrapper *wrapper) {
  tibems_status status = TIBEMS_OK;
  const char* err_str   = NULL;
  VALUE e, rb_error_msg, rb_stack_trace;

  status = tibemsErrorContext_GetLastErrorString(wrapper->errorctx, &err_str);

  if (status != TIBEMS_OK) {
    const char *no_error_msg = "Unable to get error message";
    rb_error_msg = rb_str_new2(no_error_msg);
  } else {
    rb_error_msg = rb_str_new2(err_str);
  }

  status = tibemsErrorContext_GetLastErrorStackTrace(wrapper->errorctx, &err_str);

  if (status != TIBEMS_OK) {
      const char *no_stack_trace = "Unable to get stack trace error";
      rb_stack_trace = rb_tainted_str_new2(no_stack_trace);
  } else {
      rb_stack_trace = rb_tainted_str_new2(err_str);
  }

#ifdef HAVE_RUBY_ENCODING_H
  rb_enc_associate(rb_error_msg, rb_utf8_encoding());
  rb_enc_associate(rb_stack_trace, rb_usascii_encoding());
#endif

  e = rb_funcall(cTibEMSError, intern_new_with_args, 4,
                 rb_error_msg,
                 LONG2FIX(wrapper->server_version),
                 UINT2NUM(wrapper->last_status),
                 rb_stack_trace);

  rb_exc_raise(e);
}

static void *nogvl_init(void *ptr) {
  tibems_admin_wrapper *wrapper = ptr;
  tibems_status status = TIBEMS_OK;

  /* init error context */
  status = tibemsErrorContext_Create(&(wrapper->errorctx));

  if (status != TIBEMS_OK) {
    rb_raise(cTibEMSError, "unable to create tibems error context"); \
  }

  return (void*)((status == TIBEMS_OK) ? Qtrue : Qfalse);
}

static void *nogvl_create(void *ptr) {
  struct nogvl_create_args *args = ptr;
  tibems_status status = TIBEMS_OK;

  status = tibemsAdmin_Create(args->admin, args->url,
                              args->user, args->passwd, 0);

  return (void *)((status==TIBEMS_OK) ? Qtrue : Qfalse);
}

static void *nogvl_close(void *ptr) {
  tibems_admin_wrapper *wrapper = ptr;
  tibems_status status = TIBEMS_OK;

  if (wrapper->admin != TIBEMS_INVALID_ADMIN_ID) {
    status = tibemsAdmin_Close(wrapper->admin);

    if (status != TIBEMS_OK) {
      /* TODO: raise error */
    }

    wrapper->admin = TIBEMS_INVALID_ADMIN_ID;
    wrapper->connected = 0;
    wrapper->active_thread = Qnil;
  }

  return NULL;
}

/* this is called during GC */
static void rb_tibems_admin_free(void *ptr) {
  tibems_admin_wrapper *wrapper = ptr;
  decr_tibems_admin(wrapper);
}

void decr_tibems_admin(tibems_admin_wrapper *wrapper)
{
  wrapper->refcount--;

  if (wrapper->refcount == 0) {
    nogvl_close(wrapper);
    xfree(wrapper);
  }
}

static VALUE allocate(VALUE klass) {
  VALUE obj;
  tibems_admin_wrapper * wrapper;
  obj = Data_Make_Struct(klass, tibems_admin_wrapper, rb_tibems_admin_mark, rb_tibems_admin_free, wrapper);
  wrapper->encoding = Qnil;
  wrapper->active_thread = Qnil;
  wrapper->automatic_close = 1;
  wrapper->server_version = 0;
  wrapper->reconnect_enabled = 0;
  wrapper->connect_timeout = 0;
  wrapper->connected = 0; /* means that a database connection is open */
  wrapper->initialized = 0; /* means that that the wrapper is initialized */
  wrapper->refcount = 1;
  wrapper->admin = TIBEMS_INVALID_ADMIN_ID;
  wrapper->errorctx = NULL;
  wrapper->last_status = TIBEMS_OK;

  return obj;
}

static VALUE rb_create(VALUE self, VALUE url, VALUE user, VALUE pass) {
  struct nogvl_create_args args;
  time_t start_time, end_time, elapsed_time, connect_timeout;
  tibems_status status = TIBEMS_OK;
  VALUE rv;
  GET_ADMIN(self);

  args.url         = NIL_P(url )     ? NULL : StringValueCStr(url);
  args.user        = NIL_P(user)     ? NULL : StringValueCStr(user);
  args.passwd      = NIL_P(pass)     ? NULL : StringValueCStr(pass);
  args.admin       = &(wrapper->admin);

  if (wrapper->connect_timeout)
    time(&start_time);
  rv = (VALUE) rb_thread_call_without_gvl(nogvl_create, &args, RUBY_UBF_IO, 0);
  if (rv == Qfalse) {
    while (rv == Qfalse && errno == EINTR) {
      if (wrapper->connect_timeout) {
        time(&end_time);
        /* avoid long connect timeout from system time changes */
        if (end_time < start_time)
          start_time = end_time;
        elapsed_time = end_time - start_time;
        /* avoid an early timeout due to time truncating milliseconds off the start time */
        if (elapsed_time > 0)
          elapsed_time--;
        if (elapsed_time >= (time_t)wrapper->connect_timeout)
          break;
        connect_timeout = wrapper->connect_timeout - elapsed_time;
        status = tibemsAdmin_SetCommandTimeout(wrapper->admin, connect_timeout * 10000);
      }
      errno = 0;
      rv = (VALUE) rb_thread_call_without_gvl(nogvl_create, &args, RUBY_UBF_IO, 0);
    }
    /* restore the connect timeout for reconnecting */
    if (wrapper->connect_timeout)
      status = tibemsAdmin_SetCommandTimeout(wrapper->admin, wrapper->connect_timeout * 10000);
    if (rv == Qfalse || status != TIBEMS_OK)
      rb_raise_tibems_admin_error(wrapper);
  }

/* TODO: get server version as long
  wrapper->server_version = mysql_get_server_version(wrapper->client);
*/
  wrapper->connected = 1;
  return self;
}

void rb_tibems_admin_set_active_thread(VALUE self) {
  VALUE thread_current = rb_thread_current();
  GET_ADMIN(self);

  // see if this connection is still waiting on a result from a previous query
  if (NIL_P(wrapper->active_thread)) {
    // mark this connection active
    wrapper->active_thread = thread_current;
  } else if (wrapper->active_thread == thread_current) {
    rb_raise(cTibEMSError, "This connection is still waiting for a result, try again once you have the result");
  } else {
    VALUE inspect = rb_inspect(wrapper->active_thread);
    const char *thr = StringValueCStr(inspect);

    rb_raise(cTibEMSError, "This connection is in use by: %s", thr);
  }
}

static VALUE rb_tibems_admin_get_queue_stats(VALUE self, tibemsQueueInfo  queueInfo) {
  tibems_status status = TIBEMS_OK;
  tibems_long dest_deliveredMessageCount, dest_flowControlMaxBytes, dest_maxBytes,
              dest_maxMsgs, dest_pendingMessageCount, dest_pendingMessageSize,
              dest_pendingPersistentMessageCount;
  tibems_int dest_receiverCount;
  tibemsStatData statData;
  VALUE info, stats;
  tibems_long dest_byteRate, dest_messageRate, dest_totalBytes, dest_totalMessages;
  char nameBuf[1024];

  GET_ADMIN(self);

  REQUIRE_CONNECTED(wrapper);

  status = tibemsQueueInfo_GetName(queueInfo, nameBuf, sizeof(nameBuf));
  if (status != TIBEMS_OK) {
    rb_raise_tibems_admin_error(wrapper);
  }

  if ((TIBEMS_OK == tibemsQueueInfo_GetDeliveredMessageCount(queueInfo, &dest_deliveredMessageCount))
      && (TIBEMS_OK == tibemsQueueInfo_GetFlowControlMaxBytes(queueInfo, &dest_flowControlMaxBytes))
      && (TIBEMS_OK == tibemsQueueInfo_GetMaxBytes(queueInfo, &dest_maxBytes))
      && (TIBEMS_OK == tibemsQueueInfo_GetMaxMsgs(queueInfo, &dest_maxMsgs))
      && (TIBEMS_OK == tibemsQueueInfo_GetPendingMessageCount(queueInfo, &dest_pendingMessageCount))
      && (TIBEMS_OK == tibemsQueueInfo_GetPendingMessageSize(queueInfo, &dest_pendingMessageSize))
      && (TIBEMS_OK == tibemsQueueInfo_GetPendingPersistentMessageCount(queueInfo, &dest_pendingPersistentMessageCount))
      && (TIBEMS_OK == tibemsQueueInfo_GetReceiverCount(queueInfo, &dest_receiverCount))) {
    info = rb_hash_new();
    rb_hash_aset(info, sym_dest_name, rb_str_new2(nameBuf));
    rb_hash_aset(info, sym_dest_deliveredMessageCount, LONG2FIX(dest_deliveredMessageCount));
    rb_hash_aset(info, sym_dest_flowControlMaxBytes, LONG2FIX(dest_flowControlMaxBytes));
    rb_hash_aset(info, sym_dest_maxBytes, LONG2FIX(dest_maxBytes));
    rb_hash_aset(info, sym_dest_maxMsgs, LONG2FIX(dest_maxMsgs));
    rb_hash_aset(info, sym_dest_pendingMessageCount, LONG2FIX(dest_pendingMessageCount));
    rb_hash_aset(info, sym_dest_pendingMessageSize, LONG2FIX(dest_pendingMessageSize));
    rb_hash_aset(info, sym_dest_pendingPersistentMessageCount, LONG2FIX(dest_pendingPersistentMessageCount));
    rb_hash_aset(info, sym_dest_receiverCount, LONG2FIX(dest_receiverCount));

    if ((TIBEMS_OK == tibemsQueueInfo_GetOutboundStatistics(queueInfo, &statData))
        && (TIBEMS_OK == tibemsStatData_GetByteRate(statData, &dest_byteRate))
        && (TIBEMS_OK == tibemsStatData_GetMessageRate(statData, &dest_messageRate))
        && (TIBEMS_OK == tibemsStatData_GetTotalBytes(statData, &dest_totalBytes))
        && (TIBEMS_OK == tibemsStatData_GetTotalMessages(statData, &dest_totalMessages))) {

        stats = rb_hash_new();
        rb_hash_aset(stats, sym_dest_byteRate, LONG2FIX(dest_byteRate));
        rb_hash_aset(stats, sym_dest_messageRate, LONG2FIX(dest_messageRate));
        rb_hash_aset(stats, sym_dest_totalBytes, LONG2FIX(dest_totalBytes));
        rb_hash_aset(stats, sym_dest_totalMessages, LONG2FIX(dest_totalMessages));

        rb_hash_aset(info, sym_outbound, stats);
    } else {
      rb_raise_tibems_admin_error(wrapper);
    }

    if ((TIBEMS_OK == tibemsQueueInfo_GetInboundStatistics(queueInfo, &statData))
        && (TIBEMS_OK == tibemsStatData_GetByteRate(statData, &dest_byteRate))
        && (TIBEMS_OK == tibemsStatData_GetMessageRate(statData, &dest_messageRate))
        && (TIBEMS_OK == tibemsStatData_GetTotalBytes(statData, &dest_totalBytes))
        && (TIBEMS_OK == tibemsStatData_GetTotalMessages(statData, &dest_totalMessages))) {

        stats = rb_hash_new();
        rb_hash_aset(stats, sym_dest_byteRate, LONG2FIX(dest_byteRate));
        rb_hash_aset(stats, sym_dest_messageRate, LONG2FIX(dest_messageRate));
        rb_hash_aset(stats, sym_dest_totalBytes, LONG2FIX(dest_totalBytes));
        rb_hash_aset(stats, sym_dest_totalMessages, LONG2FIX(dest_totalMessages));

        rb_hash_aset(info, sym_inbound, stats);
    } else {
      rb_raise_tibems_admin_error(wrapper);
    }

    return info;
  } else {
    return Qnil;
  }
}

static VALUE rb_tibems_admin_get_topic_stats(VALUE self, tibemsTopicInfo topicInfo) {
  tibems_status status = TIBEMS_OK;
  tibems_int  dest_activeDurableCount, dest_durableCount, dest_subscriberCount;
  tibems_long dest_flowControlMaxBytes, dest_maxBytes,
              dest_maxMsgs, dest_pendingMessageCount, dest_pendingMessageSize,
              dest_pendingPersistentMessageCount;
  tibemsStatData statData;
  VALUE info, stats;
  tibems_long dest_byteRate, dest_messageRate, dest_totalBytes, dest_totalMessages;
  char nameBuf[1024];

  GET_ADMIN(self);

  REQUIRE_CONNECTED(wrapper);

  status = tibemsTopicInfo_GetName(topicInfo, nameBuf, sizeof(nameBuf));
  if (status != TIBEMS_OK) {
    rb_raise_tibems_admin_error(wrapper);
  }

  if ((TIBEMS_OK == tibemsTopicInfo_GetActiveDurableCount(topicInfo, &dest_activeDurableCount))
      && (TIBEMS_OK == tibemsTopicInfo_GetDurableCount(topicInfo, &dest_durableCount))
      && (TIBEMS_OK == tibemsTopicInfo_GetFlowControlMaxBytes(topicInfo, &dest_flowControlMaxBytes))
      && (TIBEMS_OK == tibemsTopicInfo_GetMaxBytes(topicInfo, &dest_maxBytes))
      && (TIBEMS_OK == tibemsTopicInfo_GetMaxMsgs(topicInfo, &dest_maxMsgs))
      && (TIBEMS_OK == tibemsTopicInfo_GetPendingMessageCount(topicInfo, &dest_pendingMessageCount))
      && (TIBEMS_OK == tibemsTopicInfo_GetPendingMessageSize(topicInfo, &dest_pendingMessageSize))
      && (TIBEMS_OK == tibemsTopicInfo_GetPendingPersistentMessageCount(topicInfo, &dest_pendingPersistentMessageCount))
      && (TIBEMS_OK == tibemsTopicInfo_GetSubscriberCount(topicInfo, &dest_subscriberCount))) {
    info = rb_hash_new();
    rb_hash_aset(info, sym_dest_name, rb_str_new2(nameBuf));
    rb_hash_aset(info, sym_dest_activeDurableCount, LONG2FIX(dest_activeDurableCount));
    rb_hash_aset(info, sym_dest_durableCount, LONG2FIX(dest_durableCount));
    rb_hash_aset(info, sym_dest_flowControlMaxBytes, LONG2FIX(dest_flowControlMaxBytes));
    rb_hash_aset(info, sym_dest_maxBytes, LONG2FIX(dest_maxBytes));
    rb_hash_aset(info, sym_dest_maxMsgs, LONG2FIX(dest_maxMsgs));
    rb_hash_aset(info, sym_dest_pendingMessageCount, LONG2FIX(dest_pendingMessageCount));
    rb_hash_aset(info, sym_dest_pendingMessageSize, LONG2FIX(dest_pendingMessageSize));
    rb_hash_aset(info, sym_dest_pendingPersistentMessageCount, LONG2FIX(dest_pendingPersistentMessageCount));
    rb_hash_aset(info, sym_dest_subscriberCount, LONG2FIX(dest_subscriberCount));

    if ((TIBEMS_OK == tibemsTopicInfo_GetOutboundStatistics(topicInfo, &statData))
        && (TIBEMS_OK == tibemsStatData_GetByteRate(statData, &dest_byteRate))
        && (TIBEMS_OK == tibemsStatData_GetMessageRate(statData, &dest_messageRate))
        && (TIBEMS_OK == tibemsStatData_GetTotalBytes(statData, &dest_totalBytes))
        && (TIBEMS_OK == tibemsStatData_GetTotalMessages(statData, &dest_totalMessages))) {

        stats = rb_hash_new();
        rb_hash_aset(stats, sym_dest_byteRate, LONG2FIX(dest_byteRate));
        rb_hash_aset(stats, sym_dest_messageRate, LONG2FIX(dest_messageRate));
        rb_hash_aset(stats, sym_dest_totalBytes, LONG2FIX(dest_totalBytes));
        rb_hash_aset(stats, sym_dest_totalMessages, LONG2FIX(dest_totalMessages));

        rb_hash_aset(info, sym_outbound, stats);
    } else {
      rb_raise_tibems_admin_error(wrapper);
    }

    if ((TIBEMS_OK == tibemsTopicInfo_GetInboundStatistics(topicInfo, &statData))
        && (TIBEMS_OK == tibemsStatData_GetByteRate(statData, &dest_byteRate))
        && (TIBEMS_OK == tibemsStatData_GetMessageRate(statData, &dest_messageRate))
        && (TIBEMS_OK == tibemsStatData_GetTotalBytes(statData, &dest_totalBytes))
        && (TIBEMS_OK == tibemsStatData_GetTotalMessages(statData, &dest_totalMessages))) {

        stats = rb_hash_new();
        rb_hash_aset(stats, sym_dest_byteRate, LONG2FIX(dest_byteRate));
        rb_hash_aset(stats, sym_dest_messageRate, LONG2FIX(dest_messageRate));
        rb_hash_aset(stats, sym_dest_totalBytes, LONG2FIX(dest_totalBytes));
        rb_hash_aset(stats, sym_dest_totalMessages, LONG2FIX(dest_totalMessages));

        rb_hash_aset(info, sym_inbound, stats);
    } else {
      rb_raise_tibems_admin_error(wrapper);
    }
    return info;
  } else {
    return Qnil;
  }
}

static VALUE rb_tibems_admin_get_info(VALUE self) {
  tibemsServerInfo serverInfo = TIBEMS_INVALID_ADMIN_ID;
  tibemsQueueInfo  queueInfo  = TIBEMS_INVALID_ADMIN_ID;
  tibemsTopicInfo  topicInfo  = TIBEMS_INVALID_ADMIN_ID;
  tibemsCollection destInfos;
  tibems_status status = TIBEMS_OK;
  tibems_int    queue_count, topic_count, producer_count, consumer_count;
  VALUE info, queues, topics, stats;

  GET_ADMIN(self);

  REQUIRE_CONNECTED(wrapper);

  status = tibemsAdmin_GetInfo(wrapper->admin, &serverInfo);

  if (status != TIBEMS_OK) {
    rb_raise_tibems_admin_error(wrapper);
    return Qnil;
  }

  info = Qnil;
  if ((TIBEMS_OK == tibemsServerInfo_GetQueueCount(serverInfo, &queue_count))
      && (TIBEMS_OK == tibemsServerInfo_GetTopicCount(serverInfo, &topic_count))
      && (TIBEMS_OK == tibemsServerInfo_GetProducerCount(serverInfo, &producer_count))
      && (TIBEMS_OK == tibemsServerInfo_GetConsumerCount(serverInfo, &consumer_count))) {
    info = rb_hash_new();
    rb_hash_aset(info, sym_queue, LONG2FIX(queue_count));
    rb_hash_aset(info, sym_topic, LONG2FIX(topic_count));
    rb_hash_aset(info, sym_producer, LONG2FIX(producer_count));
    rb_hash_aset(info, sym_consumer, LONG2FIX(consumer_count));
    tibemsServerInfo_Destroy(serverInfo);
  } else {
    tibemsServerInfo_Destroy(serverInfo);
    rb_raise_tibems_admin_error(wrapper);
  }

  // Get Queue Stats
  status = tibemsAdmin_GetQueues(wrapper->admin, &destInfos, ">", TIBEMS_DEST_GET_NOTEMP);

  if (status != TIBEMS_OK) {
    rb_raise_tibems_admin_error(wrapper);
    return Qnil;
  }

  status = tibemsCollection_GetFirst(destInfos, (&queueInfo));

  if (status != TIBEMS_OK) {
    tibemsCollection_Destroy(destInfos);
    rb_raise_tibems_admin_error(wrapper);
  }

  stats = rb_tibems_admin_get_queue_stats(self,queueInfo);

  if (stats == Qnil) {
    tibemsQueueInfo_Destroy(queueInfo);
    tibemsCollection_Destroy(destInfos);
    rb_raise_tibems_admin_error(wrapper);
  }

  queues = rb_ary_new();
  rb_ary_push(queues, stats);

  while (TIBEMS_NOT_FOUND != (status = tibemsCollection_GetNext(destInfos, &queueInfo))) {
    if (status != TIBEMS_OK) {
      tibemsCollection_Destroy(destInfos);
      rb_raise_tibems_admin_error(wrapper);
    }

    stats = rb_tibems_admin_get_queue_stats(self,queueInfo);

    if (stats == Qnil) {
      tibemsQueueInfo_Destroy(queueInfo);
      tibemsCollection_Destroy(destInfos);
      rb_raise_tibems_admin_error(wrapper);
    }

    rb_ary_push(queues, stats);

    tibemsQueueInfo_Destroy(queueInfo);
  }

  tibemsCollection_Destroy(destInfos);
  rb_hash_aset(info, sym_queues, queues);

  // Get Topic Stats
  status = tibemsAdmin_GetTopics(wrapper->admin, &destInfos, ">", TIBEMS_DEST_GET_NOTEMP);

  if (status != TIBEMS_OK) {
    rb_raise_tibems_admin_error(wrapper);
    return Qnil;
  }

  status = tibemsCollection_GetFirst(destInfos, (&topicInfo));

  if (status != TIBEMS_OK) {
    tibemsCollection_Destroy(destInfos);
    rb_raise_tibems_admin_error(wrapper);
  }

  stats = rb_tibems_admin_get_topic_stats(self,topicInfo);

  if (stats == Qnil) {
    tibemsTopicInfo_Destroy(topicInfo);
    tibemsCollection_Destroy(destInfos);
    rb_raise_tibems_admin_error(wrapper);
  }

  topics = rb_ary_new();
  rb_ary_push(topics, stats);

  while (TIBEMS_NOT_FOUND != (status = tibemsCollection_GetNext(destInfos, &topicInfo))) {
    if (status != TIBEMS_OK) {
      tibemsCollection_Destroy(destInfos);
      rb_raise_tibems_admin_error(wrapper);
    }

    stats = rb_tibems_admin_get_topic_stats(self,topicInfo);

    if (stats == Qnil) {
      tibemsQueueInfo_Destroy(topicInfo);
      tibemsCollection_Destroy(destInfos);
      rb_raise_tibems_admin_error(wrapper);
    }

    rb_ary_push(topics, stats);

    tibemsQueueInfo_Destroy(topicInfo);
  }

  tibemsCollection_Destroy(destInfos);
  rb_hash_aset(info, sym_topics, topics);

  return info;
}

/*
 * Immediately disconnect from the server; normally the garbage collector
 * will disconnect automatically when a connection is no longer needed.
 * Explicitly closing this will free up server resources sooner than waiting
 * for the garbage collector.
 *
 * @return [nil]
 */
static VALUE rb_tibems_admin_close(VALUE self) {
  GET_ADMIN(self);

  if (wrapper->connected) {
    rb_thread_call_without_gvl(nogvl_close, wrapper, RUBY_UBF_IO, 0);
  }

  return Qnil;
}

static VALUE initialize_ext(VALUE self) {
  GET_ADMIN(self);

  if ((VALUE)rb_thread_call_without_gvl(nogvl_init, wrapper, RUBY_UBF_IO, 0) == Qfalse) {
    /* TODO: warning - not enough memory? */
    rb_raise_tibems_admin_error(wrapper);
  }

  wrapper->initialized = 1;
  return self;
}

void init_tibems_admin() {
#if 0
  mTibEMS      = rb_define_module("TibEMS"); Teach RDoc about TibEMS constant.
#endif

  cTibEMSAdmin = rb_define_class_under(mTibEMS, "Admin", rb_cObject);

  rb_define_alloc_func(cTibEMSAdmin, allocate);

  rb_define_method(cTibEMSAdmin, "close", rb_tibems_admin_close, 0);
  rb_define_method(cTibEMSAdmin, "get_info", rb_tibems_admin_get_info, 0);

  rb_define_private_method(cTibEMSAdmin, "initialize_ext", initialize_ext, 0);
  rb_define_private_method(cTibEMSAdmin, "create", rb_create, 3);

  sym_queue           = ID2SYM(rb_intern("queue"));
  sym_topic           = ID2SYM(rb_intern("topic"));
  sym_producer        = ID2SYM(rb_intern("producer"));
  sym_consumer        = ID2SYM(rb_intern("consumer"));
  sym_queues          = ID2SYM(rb_intern("queues"));
  sym_topics          = ID2SYM(rb_intern("topics"));

  sym_dest_name                  = ID2SYM(rb_intern("name"));
  sym_dest_deliveredMessageCount = ID2SYM(rb_intern("deliveredMessageCount"));
  sym_dest_flowControlMaxBytes   = ID2SYM(rb_intern("flowControlMaxBytes"));
  sym_dest_maxBytes              = ID2SYM(rb_intern("maxBytes"));
  sym_dest_maxMsgs               = ID2SYM(rb_intern("maxMsgs"));
  sym_dest_pendingMessageCount   = ID2SYM(rb_intern("pendingMessageCount"));
  sym_dest_pendingMessageSize    = ID2SYM(rb_intern("pendingMessageSize"));
  sym_dest_activeDurableCount    = ID2SYM(rb_intern("activeDurableCount"));
  sym_dest_durableCount          = ID2SYM(rb_intern("durableCount"));
  sym_dest_pendingPersistentMessageCount = ID2SYM(rb_intern("pendingPersistentMessageCount"));
  sym_dest_receiverCount         = ID2SYM(rb_intern("receiverCount"));
  sym_dest_subscriberCount       = ID2SYM(rb_intern("subscriberCount"));
  sym_dest_byteRate              = ID2SYM(rb_intern("byteRate"));
  sym_dest_messageRate           = ID2SYM(rb_intern("messageRate"));
  sym_dest_totalBytes            = ID2SYM(rb_intern("totalBytes"));
  sym_dest_totalMessages         = ID2SYM(rb_intern("totalMessages"));

  sym_inbound                    = ID2SYM(rb_intern("inbound"));
  sym_outbound                   = ID2SYM(rb_intern("outbound"));

  intern_brackets = rb_intern("[]");
  intern_merge = rb_intern("merge");
  intern_merge_bang = rb_intern("merge!");
  intern_new_with_args = rb_intern("new_with_args");

}

