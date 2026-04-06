#include "cajun.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

static void outtest2(void)
{
	struct caj_out_ctx outctx;
	struct cajun_node *n;
	struct cajun_node *n2;
	struct cajun_node *n3;
	caj_out_init(&outctx, 0, 4, datasink, NULL);
	n = cajun_dict_new();
	if (n == NULL)
	{
		printf("Out of memory\n");
		return;
	}
	n2 = cajun_array_new();
	if (n2 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_dict_add(n, "foo", strlen("foo"), n2) != 0)
	{
		printf("Out of memory or duplicate key\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	n3 = cajun_number_new(1, 1);
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	n3 = cajun_number_new(2, 1);
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	n3 = cajun_number_new(3, 1);
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	n2 = cajun_number_new(4, 1);
	if (n2 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_dict_add(n, "bar", strlen("bar"), n2) != 0)
	{
		printf("Out of memory or duplicate key\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	n2 = cajun_dict_new();
	if (n2 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_dict_add(n, "baz", strlen("baz"), n2) != 0)
	{
		printf("Out of memory or duplicate key\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	n2 = cajun_array_new();
	if (n2 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_dict_add(n, "barf", strlen("barf"), n2) != 0)
	{
		printf("Out of memory or duplicate key\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	n2 = cajun_array_new();
	if (n2 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_dict_add(n, "quux", strlen("quux"), n2) != 0)
	{
		printf("Out of memory or duplicate key\n");
		cajun_node_delete(n);
		cajun_node_delete(n2);
		return;
	}
	n3 = cajun_boolean_new(1);
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	n3 = cajun_boolean_new(0);
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	n3 = cajun_null_new();
	if (n3 == NULL)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		return;
	}
	if (cajun_array_add(n2, n3) != 0)
	{
		printf("Out of memory\n");
		cajun_node_delete(n);
		cajun_node_delete(n3);
		return;
	}
	cajun_node_out(&outctx, n);
	printf("\n");
	cajun_node_delete(n);
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
	cajun_node_delete(n);
	printf("\n");
	printf("\n");

	outtest2();

	return 0;
}
