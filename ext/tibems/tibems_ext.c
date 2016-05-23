#include <tibems_ext.h>

VALUE mTibEMS, cTibEMSError;

/* Ruby Extension initializer */
void Init_tibemsadmin() {
  mTibEMS      = rb_define_module("TibEMS");
  cTibEMSError = rb_const_get(mTibEMS, rb_intern("Error"));

  init_tibems_admin();
/*
  init_tibems_queue();
*/
}
