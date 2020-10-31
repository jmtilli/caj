#include "caj_out.h"
#include <stdio.h>
#include <string.h>

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

int main(int argc, char **argv)
{
	struct caj_out_ctx ctx;
	caj_out_init(&ctx, 0, 4, datasink, NULL);
	caj_out_add_start_dict(&ctx);
	caj_out_put_start_array(&ctx, "foo", 3);
	caj_out_add_number(&ctx, 1);
	caj_out_add_number(&ctx, 2);
	caj_out_add_number(&ctx, 3);
	caj_out_add_number(&ctx, 4.1);
	caj_out_add_number(&ctx, 4.2);
	caj_out_add_number(&ctx, 4.3);
	caj_out_add_number(&ctx, 4.4);
	caj_out_add_number(&ctx, 4.5);
	caj_out_add_number(&ctx, 4.6);
	caj_out_add_number(&ctx, 4.7);
	caj_out_add_number(&ctx, 4.8);
	caj_out_add_number(&ctx, 4.9);
	caj_out_end_array(&ctx);
	caj_out_put_number(&ctx, "bar", 3, 4);
	caj_out_put_start_dict(&ctx, "baz", 3);
	caj_out_end_dict(&ctx);
	caj_out_put_start_array(&ctx, "barf", 4);
	caj_out_end_array(&ctx);
	caj_out_put_start_array(&ctx, "quux", 4);
	caj_out_add_boolean(&ctx, 1);
	caj_out_add_boolean(&ctx, 0);
	caj_out_add_null(&ctx);
	caj_out_end_array(&ctx);
	caj_out_end_dict(&ctx);
	printf("\n");
	return 0;
}
