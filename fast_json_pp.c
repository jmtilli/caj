#include "caj.h"
#include "caj_out.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

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
static int my_handle_comment(struct caj_handler *cajh, int comma_seen, const char *comment, size_t commentsz)
{
	struct caj_out_ctx *octx = (struct caj_out_ctx*)cajh->userdata;
	return caj_out_comment(octx, comma_seen, comment, commentsz);
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
	.handle_comment = my_handle_comment,
};

struct caj_ctx inctx;
struct caj_out_ctx outctx;

struct caj_handler myhandler = {
	.vtable = &myhandler_vtable,
	.userdata = &outctx,
};

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-t] [-n] [-C [-C]] [-c count]\n", argv0);
	exit(1);
}

int main(int argc, char **argv)
{
	char buf[32*1024];
	size_t numbytes;
	size_t i;
	int err;
	int opt;
	int comments = 0;
	int trailingcomma = 0;
	int tab = 0;
	int nopretty = 0;
	int indentamount = -1;
	caj_init(&inctx, &myhandler);
	while ((opt = getopt(argc, argv, "tnc:hCT") ) != -1)
	{
		switch (opt)
		{
			case 't':
				tab = 1;
				break;
			case 'n':
				nopretty = 1;
				break;
			case 'C':
				comments++;
				break;
			case 'T':
				trailingcomma = 1;
				break;
			case 'c':
			{
				unsigned long ul;
				char *endptr;
				ul = strtoul(optarg, &endptr, 10);
				if (!*optarg || *endptr)
				{
					usage(argv[0]);
				}
				if (ul > INT_MAX)
				{
					usage(argv[0]);
				}
				indentamount = (int)ul;
				break;
			}
			case 'h':
				usage(argv[0]);
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
	if (optind != argc)
	{
		usage(argv[0]);
	}
	if (indentamount < 0)
	{
		if (tab)
		{
			indentamount = 1;
		}
		else
		{
			indentamount = 4;
		}
	}
	caj_out_init(&outctx, !!tab, nopretty ? SIZE_MAX : (size_t)indentamount, datasink, NULL);
	if (comments >= 1)
	{
		caj_allow_comments(&inctx);
	}
	if (trailingcomma)
	{
		caj_allow_trailing_comma(&inctx);
	}
	if (comments < 2)
	{
		myhandler_vtable.handle_comment = NULL;
	}
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
			if (feof(stdin))
			{
				caj_free(&inctx);
				putchar('\n');
				return 0;
			}
			for (;;)
			{
				numbytes = fread(buf, 1, sizeof(buf), stdin);
				for (i = 0; i < numbytes; i++)
				{
					if ((buf[i] != ' ' && buf[i] != '\n' &&
					     buf[i] != '\r' && buf[i] != '\t'))
					{
						fprintf(stderr, "Junk at end\n");
						return 1;
					}
				}
				if (numbytes == 0)
				{
					break;
				}
			}
			if (!feof(stdin))
			{
				fprintf(stderr, "Not EOF at end\n");
				return 1;
			}
			caj_free(&inctx);
			putchar('\n');
			return 0;
		}
		if (feof(stdin))
		{
			err = caj_feed(&inctx, NULL, 0, 1);
			if (err != 0)
			{
				fprintf(stderr, "Parse error at end: %d\n", err);
				return 1;
			}
			caj_free(&inctx);
			putchar('\n');
			return 0;
		}
		if (err != -EINPROGRESS && err != 0)
		{
			fprintf(stderr, "Parse error: %d\n", err);
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
	putchar('\n');
	return 0;
}
