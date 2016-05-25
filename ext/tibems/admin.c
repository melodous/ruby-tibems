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
static VALUE sym_queue, sym_topic, sym_producer, sym_consumer;
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
  tibemsAdmin admin;
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

  status = tibemsAdmin_Create(&(args->admin), args->url,
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

    xfree(&(wrapper->admin));
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
  args.admin       = wrapper->admin;

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

static VALUE rb_tibems_admin_get_info(VALUE self) {
  tibemsServerInfo serverInfo = TIBEMS_INVALID_ADMIN_ID;
  tibems_status status = TIBEMS_OK;
  tibems_int    queue_count, topic_count, producer_count, consumer_count;
  VALUE info;

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
    rb_hash_aset(info, sym_producer, LONG2FIX(topic_count));
    rb_hash_aset(info, sym_topic, LONG2FIX(producer_count));
    rb_hash_aset(info, sym_consumer, LONG2FIX(consumer_count));
    tibemsServerInfo_Destroy(serverInfo);
  } else {
    tibemsServerInfo_Destroy(serverInfo);
    rb_raise_tibems_admin_error(wrapper);
  }

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
  mTibEMS      = rb_define_module("Mysql2"); Teach RDoc about Mysql2 constant.
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

  intern_brackets = rb_intern("[]");
  intern_merge = rb_intern("merge");
  intern_merge_bang = rb_intern("merge!");
  intern_new_with_args = rb_intern("new_with_args");

}

