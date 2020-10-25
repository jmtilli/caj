#include "caj.h"
#include <stdio.h>
#include <string.h>

struct caj_ctx ctx;

static void caj_dump(void)
{
	//printf("keystacksz %d\n", (int)ctx.keystacksz);
}

static int my_start_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("start dict %s\n", key);
	return 0;
}
static int my_end_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("end dict %s\n", key);
	return 0;
}
static int my_start_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("start array %s\n", key);
	return 0;
}
static int my_end_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("end array %s\n", key);
	return 0;
}
static int my_handle_null(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("%s: null\n", key);
	return 0;
}
static int my_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz)
{
	caj_dump();
	printf("%s: %s\n", key, val);
	return 0;
}
static int my_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d)
{
	caj_dump();
	printf("%s: %g\n", key, d);
	return 0;
}
static int my_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b)
{
	caj_dump();
	printf("%s: %s\n", key, b ? "true" : "false");
	return 0;
}

struct caj_handler_vtable myhandler_vtable = {
	.start_dict = my_start_dict,
	.end_dict = my_end_dict,
	.start_array = my_start_array,
	.end_array = my_end_array,
	.handle_null = my_handle_null,
	.handle_string = my_handle_string,
	.handle_number = my_handle_number,
	.handle_boolean = my_handle_boolean,
};

struct caj_handler myhandler = {
	.vtable = &myhandler_vtable,
	.userdata = NULL,
};

int main(int argc, char **argv)
{
	char *data = " {   \"foo\": [1, 2, 3], \"bar\": 4, \"baz\": {}, \"barf\": []   , \"quux\": [true, false, null]  }";
	int ret;
	caj_init(&ctx, &myhandler);
#if 1
	ret = caj_feed(&ctx, data, strlen(data), 1);
	if (ret != 0)
	{
		printf("ret %d\n", ret);
		return 1;
	}
#else
	for (int i = 0; i < (int)strlen(data); i++)
	{
		ret = caj_feed(&ctx, data+i, 1, (i == (int)strlen(data) - 1));
		if (ret != 0)
		{
			printf("ret %d\n", ret);
			return 1;
		}
	}
#endif
	return 0;
}
