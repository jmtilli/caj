#ifndef _CAJ_LIKELY_H_
#define _CAJ_LIKELY_H_
#define caj_likely(x)       __builtin_expect((x),1)
#define caj_unlikely(x)     __builtin_expect((x),0)
#endif
