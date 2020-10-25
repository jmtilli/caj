#include "caj_out.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
struct caj_out_ctx {
	int (*datasink)(struct caj_out_ctx *ctx, const char *data, size_t sz); // FIXME handle ret
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

static void caj_out_indent(struct caj_out_ctx *ctx, int comma)
{
	size_t toindent = ctx->curindentlevel * ctx->indentamount;
	int first = 1;
	const char *indentchars = ctx->commanlindentchars;
	size_t off = 2;
	if (!comma)
	{
		indentchars++;
		off--;
	}
	if (toindent == 0)
	{
		ctx->datasink(ctx, "\n", 1);
		return;
	}
	while (toindent > 0)
	{
		size_t thisround = toindent;
		if (thisround > ctx->indentcharsz)
		{
			thisround = ctx->indentcharsz;
		}
		ctx->datasink(ctx, first ? indentchars : (indentchars + off),
		              first ? (thisround + off) : thisround);
		toindent -= thisround;
		first = 0;
	}
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
	ctx->datasink(ctx, "\"", 1);
	while (i < sz)
	{
		if (caj_needs_escape(s[i]))
		{
			if (s[i] == '"')
			{
				ctx->datasink(ctx, "\\\"", 2);
			}
			else if (s[i] == '\\')
			{
				ctx->datasink(ctx, "\\\"", 2);
			}
			else
			{
				ctx->datasink(ctx, caj_escape[(int)s[i]].buf, caj_escape[(int)s[i]].sz);
			}
			i++;
			continue;
		}
		seglen = caj_first_to_escape(s+i, sz-i);
		ctx->datasink(ctx, s+i, seglen);
		i += seglen;
	}
	ctx->datasink(ctx, "\"", 1);
	return 0;
}

static int caj_internal_put_i64(struct caj_out_ctx *ctx, int64_t i64)
{
	long long ll = i64;
	char buf128[128];
	snprintf(buf128, sizeof(buf128)-1, "%lld", ll);
	ctx->datasink(ctx, buf128, strlen(buf128));
	return 0;
}

static inline double myintpow10(int exponent)
{
	double a = 1.0;
	double b = 10.0;
	if (exponent < 0)
	{
		return 1.0 / myintpow10(-exponent);
	}
	// invariant: a * b^exponent stays the same
	while (exponent > 0)
	{
		if ((exponent % 2) == 0)
		{
			exponent /= 2;
			b *= b;
		}
		else
		{
			exponent -= 1;
			a *= b;
		}
	}
	return a;
}

static int caj_internal_put_number(struct caj_out_ctx *ctx, double d)
{
	int64_t i64 = (int64_t)d;
	char buf128[128];
	if (d < 0.0001 && d > -0.0001)
	{
		snprintf(buf128, sizeof(buf128)-1, "%.20e", d);
		ctx->datasink(ctx, buf128, strlen(buf128));
		return 0;
	}
	if ((double)i64 == d && i64 <= (1LL<<53) && i64 >= -(1LL<<53))
	{
		return caj_internal_put_i64(ctx, i64);
	}
	snprintf(buf128, sizeof(buf128)-5, "%.20g", d);
	size_t len = strlen(buf128);
	if (strchr(buf128, '.') == NULL)
	{
		// Let's not make a floating point value look like an integer
		buf128[len] = '.';
		buf128[len+1] = '0';
		buf128[len+2] = '\0';
	}
	ctx->datasink(ctx, buf128, strlen(buf128));
	return 0;
}

int caj_out_put_start_dict(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	ctx->datasink(ctx, ": {", 3);
	ctx->first = 1;
	ctx->curindentlevel++;
	return 0;
}
int caj_out_put_start_array(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	ctx->datasink(ctx, ": [", 3);
	ctx->first = 1;
	ctx->curindentlevel++;
	return 0;
}
int caj_out_add_start_dict(struct caj_out_ctx *ctx)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	ctx->datasink(ctx, "{", 1);
	ctx->first = 1;
	ctx->curindentlevel++;
	return 0;
}
int caj_out_add_start_array(struct caj_out_ctx *ctx)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	ctx->datasink(ctx, "{", 1);
	ctx->first = 1;
	ctx->curindentlevel++;
	return 0;
}
int caj_out_end_dict(struct caj_out_ctx *ctx)
{
	if (ctx->curindentlevel == 0)
	{
		abort();
	}
	ctx->curindentlevel--;
	if (!ctx->first)
	{
		caj_out_indent(ctx, 0);
	}
	ctx->first = 0;
	ctx->datasink(ctx, "}", 1);
	return 0;
}
int caj_out_end_array(struct caj_out_ctx *ctx)
{
	if (ctx->curindentlevel == 0)
	{
		abort();
	}
	ctx->curindentlevel--;
	if (!ctx->first)
	{
		caj_out_indent(ctx, 0);
	}
	ctx->first = 0;
	ctx->datasink(ctx, "]", 1);
	return 0;
}
int caj_out_put_string(struct caj_out_ctx *ctx, const char *key, size_t keysz, const char *val, size_t valsz)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	ctx->datasink(ctx, ": ", 2);
	caj_internal_put_string(ctx, val, valsz);
	ctx->first = 0;
	return 0;
}
int caj_out_add_string(struct caj_out_ctx *ctx, const char *val, size_t valsz)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	caj_internal_put_string(ctx, val, valsz);
	ctx->first = 0;
	return 0;
}
int caj_out_put_number(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	ctx->datasink(ctx, ": ", 2);
	caj_internal_put_number(ctx, d);
	ctx->first = 0;
	return 0;
}
int caj_out_add_number(struct caj_out_ctx *ctx, double d)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	caj_internal_put_number(ctx, d);
	ctx->first = 0;
	return 0;
}
int caj_out_put_boolean(struct caj_out_ctx *ctx, const char *key, size_t keysz, int b)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	if (b)
	{
		ctx->datasink(ctx, ": true", 6);
	}
	else
	{
		ctx->datasink(ctx, ": false", 7);
	}
	ctx->first = 0;
	return 0;
}
int caj_out_add_boolean(struct caj_out_ctx *ctx, int b)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	if (b)
	{
		ctx->datasink(ctx, "true", 4);
	}
	else
	{
		ctx->datasink(ctx, "false", 5);
	}
	ctx->first = 0;
	return 0;
}
int caj_out_put_null(struct caj_out_ctx *ctx, const char *key, size_t keysz)
{
	if (ctx->veryfirst)
	{
		abort();
	}
	caj_out_indent(ctx, !ctx->first);
	caj_internal_put_string(ctx, key, keysz);
	ctx->datasink(ctx, ": null", 6);
	ctx->first = 0;
	return 0;
}
int caj_out_add_null(struct caj_out_ctx *ctx)
{
	if (!ctx->veryfirst)
	{
		caj_out_indent(ctx, !ctx->first);
	}
	ctx->veryfirst = 0;
	ctx->datasink(ctx, "null", 4);
	ctx->first = 0;
	return 0;
}
