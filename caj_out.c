#include "caj_out.h"
#include "prettyftoa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
struct caj_out_ctx {
	int (*datasink)(struct caj_out_ctx *ctx, const char *data, size_t sz);
	void *userdata;
	const char *commanlindentchars;
	size_t indentcharsz;
	size_t indentamount;
	size_t curindentlevel;
	unsigned first:1;
	unsigned veryfirst:1;
};
*/

struct caj_indent_struct {
	const char *buf;
	size_t sz;
};

const char caj_indent_tabs[] =
  ",\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

const char caj_indent_spaces[] =
  ",\n                                                                      ";

const struct caj_indent_struct caj_indent_struct[2] = {
	{.buf = caj_indent_tabs, .sz = sizeof(caj_indent_tabs)-3},
	{.buf = caj_indent_spaces, .sz = sizeof(caj_indent_spaces)-3},
};

void caj_out_init(struct caj_out_ctx *ctx, int tabs, size_t indentamount,
                  int (*datasink)(struct caj_out_ctx *ctx, const char *data, size_t sz),
		  void *userdata)
{
	ctx->commanlindentchars = caj_indent_struct[!tabs].buf;
	ctx->indentcharsz = caj_indent_struct[!tabs].sz;
	ctx->indentamount = indentamount;
	ctx->curindentlevel = 0;
	ctx->first = 1;
	ctx->veryfirst = 1;
	ctx->datasink = datasink;
	ctx->userdata = userdata;
}

static int caj_out_indent(struct caj_out_ctx *ctx, int comma)
{
	size_t toindent = ctx->curindentlevel * ctx->indentamount;
	int first = 1;
	const char *indentchars = ctx->commanlindentchars;
	size_t off = 2;
	int ret;
	if (!comma)
	{
		indentchars++;
		off--;
	}
	if (toindent == 0)
	{
		return ctx->datasink(ctx, "\n", 1);
	}
	while (toindent > 0)
	{
		size_t thisround = toindent;
		if (thisround > ctx->indentcharsz)
		{
			thisround = ctx->indentcharsz;
		}
		ret = ctx->datasink(ctx,
		                    first ? indentchars : (indentchars + off),
		                    first ? (thisround + off) : thisround);
		if (ret)
		{
			return ret;
		}
		toindent -= thisround;
		first = 0;
	}
	return 0;
}

static inline int caj_needs_escape(char ch)
{
	const unsigned char uch = (unsigned char)ch;
	if (uch < 0x20)
	{
		return 1;
	}
	if (uch == '\\' || uch == '"')
	{
		return 1;
	}
	return 0;
}

static inline size_t caj_first_to_escape(const char *s, size_t sz)
{
	size_t i;
	for (i = 0; i < sz; i++)
	{
		if (caj_needs_escape(s[i]))
		{
			return i;
		}
	}
	return sz;
}

struct caj_escape {
	const char *buf;
	size_t sz;
};
const struct caj_escape caj_escape[] = {
	{.buf = "\\u0000", .sz = 6},
	{.buf = "\\u0001", .sz = 6},
	{.buf = "\\u0002", .sz = 6},
	{.buf = "\\u0003", .sz = 6},
	{.buf = "\\u0004", .sz = 6},
	{.buf = "\\u0005", .sz = 6},
	{.buf = "\\u0006", .sz = 6},
	{.buf = "\\u0007", .sz = 6},
	{.buf = "\\b", .sz = 2},
	{.buf = "\\t", .sz = 2},
	{.buf = "\\n", .sz = 2},
	{.buf = "\\u000b", .sz = 6},
	{.buf = "\\f", .sz = 2},
	{.buf = "\\r", .sz = 2},
	{.buf = "\\u000e", .sz = 6},
	{.buf = "\\u000f", .sz = 6},
	{.buf = "\\u0010", .sz = 6},
	{.buf = "\\u0011", .sz = 6},
	{.buf = "\\u0012", .sz = 6},
	{.buf = "\\u0013", .sz = 6},
	{.buf = "\\u0014", .sz = 6},
	{.buf = "\\u0015", .sz = 6},
	{.buf = "\\u0016", .sz = 6},
	{.buf = "\\u0017", .sz = 6},
	{.buf = "\\u0018", .sz = 6},
	{.buf = "\\u0019", .sz = 6},
	{.buf = "\\u001a", .sz = 6},
	{.buf = "\\u001b", .sz = 6},
	{.buf = "\\u001c", .sz = 6},
	{.buf = "\\u001d", .sz = 6},
	{.buf = "\\u001e", .sz = 6},
	{.buf = "\\u001f", .sz = 6},
};

static int caj_internal_put_string(struct caj_out_ctx *ctx, const char *s, size_t sz)
{
	size_t i = 0;
	size_t seglen;
	int ret;
	ret = ctx->datasink(ctx, "\"", 1);
	if (ret)
	{
		return ret;
	}
	while (i < sz)
	{
		if (caj_needs_escape(s[i]))
		{
			if (s[i] == '"')
			{
				ret = ctx->datasink(ctx, "\\\"", 2);
				if (ret)
				{
					return ret;
				}
			}
			else if (s[i] == '\\')
			{
				ret = ctx->datasink(ctx, "\\\"", 2);
				if (ret)
				{
					return ret;
				}
			}
			else
			{
				ret = ctx->datasink(ctx, caj_escape[(int)s[i]].buf, caj_escape[(int)s[i]].sz);
				if (ret)
				{
					return ret;
				}
			}
			i++;
			continue;
		}
		seglen = caj_first_to_escape(s+i, sz-i);
		ret = ctx->datasink(ctx, s+i, seglen);
		if (ret)
		{
			return ret;
		}
		i += seglen;
	}
	return ctx->datasink(ctx, "\"", 1);
}

static int caj_internal_put_i64(struct caj_out_ctx *ctx, int64_t i64)
{
	long long ll = i64;
	char buf128[128];
	snprintf(buf128, sizeof(buf128)-1, "%lld", ll);
	return ctx->datasink(ctx, buf128, strlen(buf128));
}

static int caj_internal_put_flop(struct caj_out_ctx *ctx, double d)
{
	char buf128[128];
	if (!isfinite(d))
	{
		abort();
	}
	pretty_ftoa(buf128, sizeof(buf128), d);
	return ctx->datasink(ctx, buf128, strlen(buf128));
}
static int caj_internal_put_flop_ex(struct caj_out_ctx *ctx, double d)
{
	char buf128[128];
	if (!isfinite(d))
	{
		return ctx->datasink(ctx, "null", 4);
	}
	pretty_ftoa(buf128, sizeof(buf128), d);
	return ctx->datasink(ctx, buf128, strlen(buf128));
}
static int caj_internal_put_number(struct caj_out_ctx *ctx, double d)
{
	int64_t i64 = (int64_t)d;
	if ((double)i64 == d && i64 <= (1LL<<48) && i64 >= -(1LL<<48))
	{
		// Let's represent at most 48-bit integers accurately
		return caj_internal_put_i64(ctx, i64);
	}
	return caj_internal_put_flop(ctx, d);
}
static int caj_internal_put_number_ex(struct caj_out_ctx *ctx, double d)
{
	int64_t i64 = (int64_t)d;
	if ((double)i64 == d && i64 <= (1LL<<48) && i64 >= -(1LL<<48))
	{
		// Let's represent at most 48-bit integers accurately
		return caj_internal_put_i64(ctx, i64);
	}
	return caj_internal_put_flop_ex(ctx, d);
}

int caj_out_put2_start_dict(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ctx->first = 1;
	ctx->curindentlevel++;
	return ctx->datasink(ctx, ": {", 3);
}
int caj_out_put_start_dict(struct caj_out_ctx *ctx, const char *key)
{
	return caj_out_put2_start_dict(ctx, key, strlen(key));
}
int caj_out_put2_start_array(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ctx->first = 1;
	ctx->curindentlevel++;
	return ctx->datasink(ctx, ": [", 3);
}
int caj_out_put_start_array(struct caj_out_ctx *ctx, const char *key)
{
	return caj_out_put2_start_array(ctx, key, strlen(key));
}
int caj_out_add_start_dict(struct caj_out_ctx *ctx)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ctx->first = 1;
	ctx->curindentlevel++;
	return ctx->datasink(ctx, "{", 1);
}
int caj_out_add_start_array(struct caj_out_ctx *ctx)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ctx->first = 1;
	ctx->curindentlevel++;
	return ctx->datasink(ctx, "[", 1);
}
int caj_out_end_dict(struct caj_out_ctx *ctx)
{
	int ret;
	if (ctx->curindentlevel == 0)
	{
		abort();
	}
	ctx->curindentlevel--;
	if (!ctx->first)
	{
		ret = caj_out_indent(ctx, 0);
		if (ret)
		{
			return ret;
		}
	}
	ctx->first = 0;
	return ctx->datasink(ctx, "}", 1);
}
int caj_out_end_array(struct caj_out_ctx *ctx)
{
	int ret;
	if (ctx->curindentlevel == 0)
	{
		abort();
	}
	ctx->curindentlevel--;
	if (!ctx->first)
	{
		ret = caj_out_indent(ctx, 0);
		if (ret)
		{
			return ret;
		}
	}
	ctx->first = 0;
	return ctx->datasink(ctx, "]", 1);
}
int caj_out_put22_string(struct caj_out_ctx *ctx, const char *key, size_t keysz, const char *val, size_t valsz)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_string(ctx, val, valsz);
}
int caj_out_put_string(struct caj_out_ctx *ctx, const char *key, const char *val)
{
	return caj_out_put22_string(ctx, key, strlen(key), val, strlen(val));
}
int caj_out_add2_string(struct caj_out_ctx *ctx, const char *val, size_t valsz)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_string(ctx, val, valsz);
	ctx->first = 0;
	return ret;
}
int caj_out_add_string(struct caj_out_ctx *ctx, const char *val)
{
	return caj_out_add2_string(ctx, val, strlen(val));
}
int caj_out_put2_flop(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_flop(ctx, d);
}
int caj_out_put_flop(struct caj_out_ctx *ctx, const char *key, double d)
{
	return caj_out_put2_flop(ctx, key, strlen(key), d);
}
int caj_out_put2_flop_ex(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_flop_ex(ctx, d);
}
int caj_out_put_flop_ex(struct caj_out_ctx *ctx, const char *key, double d)
{
	return caj_out_put2_flop_ex(ctx, key, strlen(key), d);
}
int caj_out_put2_number(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_number(ctx, d);
}
int caj_out_put2_number_ex(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_number_ex(ctx, d);
}
int caj_out_put_number(struct caj_out_ctx *ctx, const char *key, double d)
{
	return caj_out_put2_number(ctx, key, strlen(key), d);
}
int caj_out_put_number_ex(struct caj_out_ctx *ctx, const char *key, double d)
{
	return caj_out_put2_number_ex(ctx, key, strlen(key), d);
}
int caj_out_put2_i64(struct caj_out_ctx *ctx, const char *key, size_t keysz, int64_t i)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ret = ctx->datasink(ctx, ": ", 2);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return caj_internal_put_i64(ctx, i);
}
int caj_out_put_i64(struct caj_out_ctx *ctx, const char *key, int64_t i)
{
	return caj_out_put2_i64(ctx, key, strlen(key), i);
}
int caj_out_add_flop(struct caj_out_ctx *ctx, double d)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_flop(ctx, d);
	ctx->first = 0;
	return ret;
}
int caj_out_add_flop_ex(struct caj_out_ctx *ctx, double d)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_flop_ex(ctx, d);
	ctx->first = 0;
	return ret;
}
int caj_out_add_number(struct caj_out_ctx *ctx, double d)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_number(ctx, d);
	ctx->first = 0;
	return ret;
}
int caj_out_add_number_ex(struct caj_out_ctx *ctx, double d)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_number_ex(ctx, d);
	ctx->first = 0;
	return ret;
}
int caj_out_add_i64(struct caj_out_ctx *ctx, int64_t i)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ret = caj_internal_put_i64(ctx, i);
	ctx->first = 0;
	return ret;
}
int caj_out_put2_boolean(struct caj_out_ctx *ctx, const char *key, size_t keysz, int b)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	if (b)
	{
		ret = ctx->datasink(ctx, ": true", 6);
	}
	else
	{
		ret = ctx->datasink(ctx, ": false", 7);
	}
	ctx->first = 0;
	return ret;
}
int caj_out_put_boolean(struct caj_out_ctx *ctx, const char *key, int b)
{
	return caj_out_put2_boolean(ctx, key, strlen(key), b);
}
int caj_out_add_boolean(struct caj_out_ctx *ctx, int b)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	if (b)
	{
		ret = ctx->datasink(ctx, "true", 4);
	}
	else
	{
		ret = ctx->datasink(ctx, "false", 5);
	}
	ctx->first = 0;
	return ret;
}
int caj_out_put2_null(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	int ret;
	if (ctx->veryfirst)
	{
		abort();
	}
	ret = caj_out_indent(ctx, !ctx->first);
	if (ret)
	{
		return ret;
	}
	ret = caj_internal_put_string(ctx, key, keysz);
	if (ret)
	{
		return ret;
	}
	ctx->first = 0;
	return ctx->datasink(ctx, ": null", 6);
}
int caj_out_put_null(struct caj_out_ctx *ctx, const char *key)
{
	return caj_out_put2_null(ctx, key, strlen(key));
}
int caj_out_add_null(struct caj_out_ctx *ctx)
{
	int ret;
	if (!ctx->veryfirst)
	{
		ret = caj_out_indent(ctx, !ctx->first);
		if (ret)
		{
			return ret;
		}
	}
	ctx->veryfirst = 0;
	ctx->first = 0;
	return ctx->datasink(ctx, "null", 4);
}
