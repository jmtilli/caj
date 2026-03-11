#ifndef _CAJ_CONTAINEROF_H_
#define _CAJ_CONTAINEROF_H_

#include <stddef.h>

#define CAJ_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - offsetof(type, member)))

#endif
