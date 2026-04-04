#include "caj.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct pullcaj_ctx ctx;

int main(int argc, char **argv)
{
	char *data = " {   \"foo\": [1, 2, 3], \"bar\": 4, \"baz\": {}, \"barf\": []   , \"quux\": [true, false, null]  }";
	int ret = -EINPROGRESS;
	struct pullcaj_event_info ev;
	size_t i = 0;
	int eof = 0;
	pullcaj_init(&ctx);
	pullcaj_set_buf(&ctx, data, 1, 0);
	i++;
	for (;;)
	{
		while ((ret = pullcaj_get_event(&ctx, &ev)) > 0) {
			switch (ev.ev) {
				case CAJ_EV_START_DICT:
					printf("Start dict %s\n", ev.key);
					break;
				case CAJ_EV_END_DICT:
					printf("End dict %s\n", ev.key);
					break;
				case CAJ_EV_START_ARRAY:
					printf("Start array %s\n", ev.key);
					break;
				case CAJ_EV_END_ARRAY:
					printf("End array %s\n", ev.key);
					break;
				case CAJ_EV_NULL:
					printf("Null %s\n", ev.key);
					break;
				case CAJ_EV_STR:
					printf("Str %s -> %s\n", ev.key, ev.u.str.val);
					break;
				case CAJ_EV_NUM:
					printf("Num %s -> %g (%d)\n", ev.key, ev.u.num.d, ev.u.num.is_integer);
					break;
				case CAJ_EV_BOOL:
					printf("Bool %s -> %d\n", ev.key, ev.u.b.b);
					break;
			}
		}
		if (ret == 0) {
			printf("Breaking 1\n");
			break;
		}
		if (ret == -EINPROGRESS) {
			if (data[i] == '\0') {
				if (eof) {
					printf("Breaking 2\n");
					break;
				}
				eof = 1;
				if (pullcaj_set_buf(&ctx, data+i, 0, 1) != 0)
				{
					abort();
				}
			} else {
				if (pullcaj_set_buf(&ctx, data+i, 1, 0) != 0)
				{
					abort();
				}
				i++;
			}
		}
	}
	pullcaj_free(&ctx);
	return 0;
}
