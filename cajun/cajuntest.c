#include "cajun.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

int main(int argc, char **argv)
{
	struct caj_out_ctx outctx;
	char *data = " {   \"foo\": [1, 2, 3], \"bar\": 4, \"baz\": {}, \"barf\": []   , \"quux\": [true, false, null]  }";
	uint8_t u8;
	struct cajun_node *n;
	struct cajun_node *ar1, *ar2, *ar3, *dict;

	// FIXME freeing on parse error
	caj_out_init(&outctx, 0, 4, datasink, NULL);

	n = cajun_node_parse(data, strlen(data));
	if (n == NULL)
	{
		printf("Parser error\n");
		return 1;
	}

	ar1 = cajun_dict_get_array_not_null(n, "foo");
	u8 = cajun_dict_get_uint8_not_null(n, "bar");
	dict = cajun_dict_get_dict_not_null(n, "baz");
	if (dict == NULL)
	{
		abort();
	}
	ar2 = cajun_dict_get_array_not_null(n, "barf");
	ar3 = cajun_dict_get_array_not_null(n, "quux");
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

	cajun_node_out(&outctx, n);
	printf("\n");

	cajun_node_delete(n);
	return 0;
}
