#include "cajun.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv)
{
	struct caj_ctx caj;
	struct cajun_ctx cajun;
	struct caj_handler myhandler = {
		.userdata = &cajun,
		.vtable = &cajun_vtable,
	};
	char *data = " {   \"foo\": [1, 2, 3], \"bar\": 4, \"baz\": {}, \"barf\": []   , \"quux\": [true, false, null]  }";
	int ret;
	uint8_t u8;
	struct cajun_node *n;
	struct cajun_node *ar1, *ar2, *ar3, *dict;

	// FIXME freeing on parse error
	caj_init(&caj, &myhandler);
	cajun_ctx_init(&cajun);
	ret = caj_feed(&caj, data, strlen(data), 1);
	if (ret != 0)
	{
		printf("ret %d\n", ret);
		return 1;
	}

	n = cajun.n;

	ar1 = cajun_dict_get_array_not_null(n, "foo", strlen("foo"));
	u8 = cajun_dict_get_uint8_not_null(n, "bar", strlen("bar"));
	dict = cajun_dict_get_dict_not_null(n, "baz", strlen("baz"));
	if (dict == NULL)
	{
		abort();
	}
	ar2 = cajun_dict_get_array_not_null(n, "barf", strlen("barf"));
	ar3 = cajun_dict_get_array_not_null(n, "quux", strlen("quux"));
	if (cajun_array_size(ar1) != 3)
	{
		abort();
	}
	if (cajun_array_size(ar2) != 0)
	{
		abort();
	}
	if (cajun_array_size(ar3) != 3)
	{
		abort();
	}
	u8 = cajun_array_get_uint8_not_null(ar1, 0);
	if (u8 != 1)
	{
		abort();
	}
	u8 = cajun_array_get_uint8_not_null(ar1, 1);
	if (u8 != 2)
	{
		abort();
	}
	u8 = cajun_array_get_uint8_not_null(ar1, 2);
	if (u8 != 3)
	{
		abort();
	}

	if (!cajun_array_get_boolean_not_null(ar3, 0))
	{
		abort();
	}
	if (cajun_array_get_boolean_not_null(ar3, 1))
	{
		abort();
	}
	if (!cajun_array_is_null(ar3, 2, CAJUN_FORBID_MISSING))
	{
		abort();
	}

	cajun_node_free(cajun.n);
	free(cajun.n);
	cajun.n = NULL;
	cajun_ctx_free(&cajun);
	caj_free(&caj);
	return 0;
}
