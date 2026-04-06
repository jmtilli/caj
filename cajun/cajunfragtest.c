#include "cajunfrag.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int my_start_dict(struct cajunfrag_ctx *ctx, const char *key, size_t keysz)
{
	if (cajunfrag_ctx_is1(ctx, 2, "customers", NULL))
	{
		return cajunfrag_start_fragment_collection(ctx);
	}
	return 0;
}
static int my_end_dict(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, struct cajun_node *n)
{
	if (cajunfrag_ctx_is1(ctx, 2, "customers", NULL))
	{
		printf("id %d\n", cajun_dict_get_int_not_null(n, "id"));
		printf("account count %d\n", cajun_dict_get_int_not_null(n, "accountCount"));
		printf("total balance %lg\n", cajun_dict_get_double_not_null(n, "totalBalance"));
		printf("name %s\n", cajun_dict_get_string_not_null(n, "name", NULL));
	}
	return 0;
}

const struct cajunfrag_handler_vtable my_vtable = {
	.start_dict = my_start_dict,
	.end_dict = my_end_dict,
};

int main(int argc, char **argv)
{
	char *data = 
"{\n"
"        \"customers\": [\n"
"                {\n"
"                        \"id\": 1e+000,\n"
"                        \"name\": \"Clark Henson\",\n"
"                        \"accountCount\": 1,\n"
"                        \"totalBalance\": 5085.96\n"
"                },\n"
"                {\n"
"                        \"id\": 20.00e-1,\n"
"                        \"name\": \"Elnora Ericson\",\n"
"                        \"accountCount\": 3,\n"
"                        \"totalBalance\": 3910.11\n"
"                }\n"
"        ]\n"
"}\n";
	int ret;

	ret = cajunfrag_parse(data, strlen(data), &my_vtable, NULL);
	if (ret != 0)
	{
		printf("Parser error\n");
		return 1;
	}
	return 0;
}
