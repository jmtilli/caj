#ifndef _CAJ_CONTAINEROF_H_
#define _CAJ_CONTAINEROF_H_

#define CAJ_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

#endif
