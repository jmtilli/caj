#include "caj.h"
#include "caj_out.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

static int my_start_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (key == NULL)
	{
		return caj_out_add_start_dict(octx);
	}
	else
	{
		return caj_out_put2_start_dict(octx, key, keysz);
	}
}
static int my_end_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	return caj_out_end_dict(octx);
}
static int my_start_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (key == NULL)
	{
		return caj_out_add_start_array(octx);
	}
	else
	{
		return caj_out_put2_start_array(octx, key, keysz);
	}
}
static int my_end_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	return caj_out_end_array(octx);
}
static int my_handle_null(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (key == NULL)
	{
		return caj_out_add_null(octx);
	}
	else
	{
		return caj_out_put2_null(octx, key, keysz);
	}
}
static int my_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (key == NULL)
	{
		return caj_out_add2_string(octx, val, valsz);
	}
	else
	{
		return caj_out_put22_string(octx, key, keysz, val, valsz);
	}
}
static int my_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d, int is_integer)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (is_integer)
	{
		if (key == NULL)
		{
			return caj_out_add_i64(octx, (int64_t)d);
		}
		else
		{
			return caj_out_put2_i64(octx, key, keysz, (int64_t)d);
		}
	}
	else
	{
		if (key == NULL)
		{
			return caj_out_add_flop(octx, d);
		}
		else
		{
			return caj_out_put2_flop(octx, key, keysz, d);
		}
	}
}
static int my_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	if (key == NULL)
	{
		return caj_out_add_boolean(octx, b);
	}
	else
	{
		return caj_out_put2_boolean(octx, key, keysz, b);
	}
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

struct caj_ctx inctx;
struct caj_out_ctx outctx = {};

struct caj_handler myhandler = {
	.vtable = &myhandler_vtable,
	.userdata = &outctx,
};

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

int main(int argc, char **argv)
{
	char buf[2048];
	size_t numbytes;
	int err;
	caj_init(&inctx, &myhandler);
	caj_out_init(&outctx, 1, 1, datasink, NULL);
	for (;;)
	{
		numbytes = fread(buf, 1, sizeof(buf), stdin);
		if (numbytes == 0)
		{
			break;
		}
		err = caj_feed(&inctx, buf, numbytes, 0);
		if (err == 0)
		{
			numbytes = fread(buf, 1, sizeof(buf), stdin);
			if (numbytes != 0 || !feof(stdin))
			{
				fprintf(stderr, "Junk at end\n");
				return 1;
			}
			caj_free(&inctx);
			return 0;
		}
		if (err != -EINPROGRESS)
		{
			fprintf(stderr, "Parse error\n");
			return 1;
		}
	}
	err = caj_feed(&inctx, NULL, 0, 1);
	if (err != 0)
	{
		fprintf(stderr, "Parse error at end: %d\n", err);
		return 1;
	}
	caj_free(&inctx);
	return 0;
}
