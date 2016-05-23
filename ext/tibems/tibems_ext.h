#ifndef TIBEMS_EXT
#define TIBEMS_EXT

void Init_tibems(void);

/* tell rbx not to use it's caching compat layer
   by doing this we're making a promise to RBX that
   we'll never modify the pointers we get back from RSTRING_PTR */
#define RSTRING_NOT_MODIFIED
#include <ruby.h>

#ifdef __MVS__  /* TIBEMSOS_ZOS  */
#include <tibems.h>
#include <emsadmin.h>
#else
#include <tibems/tibems.h>
#include <tibems/emsadmin.h>
#endif

#ifdef HAVE_RUBY_ENCODING_H
#include <ruby/encoding.h>
#endif
#ifdef HAVE_RUBY_THREAD_H
#include <ruby/thread.h>
#endif

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define RB_TIBEMS_NORETURN __attribute__ ((noreturn))
#define RB_TIBEMS_UNUSED __attribute__ ((unused))
#else
#define RB_TIBEMS_NORETURN
#define RB_TIBEMS_UNUSED
#endif

#include "admin.h"
/*
#include <queue.h>
*/

#endif

