#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajun.h"
#include "cajunfrag.h"
#include "../caj.h"
#include <stdlib.h>
#include <stdarg.h>

int cajunfrag_ctx_is2(struct cajunfrag_ctx *ctx, size_t cnt, ...)
{
	va_list ap;
	size_t i;
	if (ctx->keystacksz != cnt + 1)
	{
		return 0;
	}
	va_start(ap, cnt);
	for (i = 0; i < cnt; i++)
	{
		const char *argname;
		size_t argsz;
		argname = va_arg(ap, const char*);
		argsz = va_arg(ap, size_t);
		if (ctx->keystack[i+1].keysz != argsz)
		{
			va_end(ap);
			return 0;
		}
		if (ctx->keystack[i+1].key == NULL && argname == NULL)
		{
			continue;
		}
		if (ctx->keystack[i+1].key == NULL || argname == NULL)
		{
			va_end(ap);
			return 0;
		}
		if (memcmp(argname, ctx->keystack[i+1].key, argsz) != 0)
		{
			va_end(ap);
			return 0;
		}
	}
	va_end(ap);
	return 1;
}

int cajunfrag_ctx_is1(struct cajunfrag_ctx *ctx, size_t cnt, ...)
{
	va_list ap;
	size_t i;
	if (ctx->keystacksz != cnt + 1)
	{
		return 0;
	}
	va_start(ap, cnt);
	for (i = 0; i < cnt; i++)
	{
		const char *argname;
		size_t argsz;
		argname = va_arg(ap, const char*);
		argsz = argname == NULL ? 0 : strlen(argname);
		if (ctx->keystack[i+1].keysz != argsz)
		{
			va_end(ap);
			return 0;
		}
		if (ctx->keystack[i+1].key == NULL && argname == NULL)
		{
			continue;
		}
		if (ctx->keystack[i+1].key == NULL || argname == NULL)
		{
			va_end(ap);
			return 0;
		}
		if (memcmp(argname, ctx->keystack[i+1].key, argsz) != 0)
		{
			va_end(ap);
			return 0;
		}
	}
	va_end(ap);
	return 1;
}

int cajunfrag_start_fragment_collection(struct cajunfrag_ctx *ctx)
{
	if (!ctx->isdict && !ctx->isarray)
	{
		abort();
	}
	if (ctx->isdict && ctx->isarray)
	{
		abort();
	}
	if (ctx->n != NULL)
	{
		abort();
	}
	if (ctx->isdict)
	{
		ctx->n = cajun_node_new();
		if (ctx->n == NULL)
		{
			return -ENOMEM;
		}
		cajun_dict_init(ctx->n);
	}
	else
	{
		ctx->n = cajun_node_new();
		if (ctx->n == NULL)
		{
			return -ENOMEM;
		}
		cajun_array_init(ctx->n);
	}
	return 0;
}

static inline void *my_memdup(const void *block, size_t sz)
{
	void *newblock = malloc(sz);
	if (newblock == NULL)
	{
		return NULL;
	}
	memcpy(newblock, block, sz);
	return newblock;
}

int cajunfrag_start_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	int ret = 0;
	void *newblock;
	struct cajun_node *dict;
	if (ctx->n == NULL)
	{
		if (ctx->keystacksz >= ctx->keystackcap)
		{
			size_t newcap = 2*ctx->keystacksz + 16;
			struct cajunfrag_keystack_item *newstack;
			newstack = realloc(ctx->keystack, sizeof(*newstack) * newcap);
			if (newstack == NULL)
			{
				return -ENOMEM;
			}
			ctx->keystack = newstack;
			ctx->keystackcap = newcap;
		}
		newblock = NULL;
		ctx->keystack[ctx->keystacksz].key = NULL;
		ctx->keystack[ctx->keystacksz].keysz = keysz;
		if (key != NULL)
		{
			newblock = my_memdup(key, keysz);
			if (newblock == NULL)
			{
				return -ENOMEM;
			}
			ctx->keystack[ctx->keystacksz].key = newblock;
		}
		ctx->keystacksz++;
		ctx->isarray = 0;
		ctx->isdict = 1;
		ret = 0;
		if (ctx->handler->start_dict)
		{
			ret = ctx->handler->start_dict(ctx, key, keysz);
		}
		ctx->isdict = 0;
		return ret;
	}

	dict = cajun_node_new();

	if (dict == NULL)
	{
		return -ENOMEM;
	}
	cajun_dict_init(dict);
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, dict);
		}
		else
		{
			ret = cajun_array_add(ctx->n, dict);
		}
		ctx->n = dict;
	}
	if (ret != 0)
	{
		cajun_node_free(dict);
		free(dict);
		return ret;
	}
	return 0;
}
int cajunfrag_end_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	struct cajun_node *n;
	int ret;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->end_dict)
		{
			ret = ctx->handler->end_dict(ctx, key, keysz, NULL);
		}
		if (ctx->keystacksz == 0)
		{
			abort();
		}
		free(ctx->keystack[ctx->keystacksz-1].key);
		ctx->keystacksz--;
		return ret;
	}
	n = ctx->n;
	ctx->n = ctx->n->parent;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->end_dict)
		{
			ret = ctx->handler->end_dict(ctx, key, keysz, n);
		}
		if (ctx->keystacksz == 0)
		{
			abort();
		}
		free(ctx->keystack[ctx->keystacksz-1].key);
		ctx->keystacksz--;
		if (n != NULL)
		{
			cajun_node_free(n);
			free(n);
		}
		return ret;
	}
	return 0;
}
int cajunfrag_start_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	void *newblock;
	struct cajun_node *ar;
	int ret = 0;
	if (ctx->n == NULL)
	{
		if (ctx->keystacksz >= ctx->keystackcap)
		{
			size_t newcap = 2*ctx->keystacksz + 16;
			struct cajunfrag_keystack_item *newstack;
			newstack = realloc(ctx->keystack, sizeof(*newstack) * newcap);
			if (newstack == NULL)
			{
				return -ENOMEM;
			}
			ctx->keystack = newstack;
			ctx->keystackcap = newcap;
		}
		newblock = NULL;
		ctx->keystack[ctx->keystacksz].key = NULL;
		ctx->keystack[ctx->keystacksz].keysz = keysz;
		if (key != NULL)
		{
			newblock = my_memdup(key, keysz);
			if (newblock == NULL)
			{
				return -ENOMEM;
			}
			ctx->keystack[ctx->keystacksz].key = newblock;
		}
		ctx->keystacksz++;
		ctx->isdict = 0;
		ctx->isarray = 1;
		ret = 0;
		if (ctx->handler->start_array)
		{
			ret = ctx->handler->start_array(ctx, key, keysz);
		}
		ctx->isarray = 0;
		return ret;
	}

	ar = cajun_node_new();
	if (ar == NULL)
	{
		return -ENOMEM;
	}
	cajun_array_init(ar);
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, ar);
		}
		else
		{
			ret = cajun_array_add(ctx->n, ar);
		}
		ctx->n = ar;
	}
	if (ret != 0)
	{
		cajun_node_free(ar);
		free(ar);
		return ret;
	}
	return 0;
}
int cajunfrag_end_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	struct cajun_node *n;
	int ret;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->end_array)
		{
			ret = ctx->handler->end_array(ctx, key, keysz, NULL);
		}
		if (ctx->keystacksz == 0)
		{
			abort();
		}
		free(ctx->keystack[ctx->keystacksz-1].key);
		ctx->keystacksz--;
		return ret;
	}
	n = ctx->n;
	ctx->n = ctx->n->parent;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->end_array)
		{
			ret = ctx->handler->end_array(ctx, key, keysz, n);
		}
		if (ctx->keystacksz == 0)
		{
			abort();
		}
		free(ctx->keystack[ctx->keystacksz-1].key);
		ctx->keystacksz--;
		if (n != NULL)
		{
			cajun_node_free(n);
			free(n);
		}
		return ret;
	}
	return 0;
}
int cajunfrag_handle_null(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	int ret = 0;
	struct cajun_node *n;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->handle_null)
		{
			ret = ctx->handler->handle_null(ctx, key, keysz);
		}
		return ret;
	}
	n = cajun_node_new();
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_null_init(n);
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->n, n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	return 0;
}
int cajunfrag_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	int ret = 0;
	struct cajun_node *n;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->handle_string)
		{
			ret = ctx->handler->handle_string(ctx, key, keysz, val, valsz);
		}
		return ret;
	}
	n = cajun_node_new();
	if (n == NULL)
	{
		return -ENOMEM;
	}
	ret = cajun_string_init(n, val, valsz);
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->n, n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	return 0;
}
int cajunfrag_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d, int is_integer)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	int ret = 0;
	struct cajun_node *n;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->handle_number)
		{
			ret = ctx->handler->handle_number(ctx, key, keysz, d, is_integer);
		}
		return ret;
	}
	n = cajun_node_new();
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_number_init(n, d, is_integer);
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->n, n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	return 0;
}
int cajunfrag_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b)
{
	struct cajunfrag_ctx *ctx = cajh->userdata;
	int ret = 0;
	struct cajun_node *n;
	if (ctx->n == NULL)
	{
		ret = 0;
		if (ctx->handler->handle_boolean)
		{
			ret = ctx->handler->handle_boolean(ctx, key, keysz, b);
		}
		return ret;
	}
	n = cajun_node_new();
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_boolean_init(n, b);
	if (ctx->n != NULL)
	{
		if (ctx->n->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->n, key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->n, n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	return 0;
}

const struct caj_handler_vtable cajunfrag_vtable = {
	.start_dict = cajunfrag_start_dict,
	.end_dict = cajunfrag_end_dict,
	.start_array = cajunfrag_start_array,
	.end_array = cajunfrag_end_array,
	.handle_null = cajunfrag_handle_null,
	.handle_string = cajunfrag_handle_string,
	.handle_number = cajunfrag_handle_number,
	.handle_boolean = cajunfrag_handle_boolean,
};

void cajunfrag_ctx_free(struct cajunfrag_ctx *ctx)
{
	size_t i;
	for (i = 0; i < ctx->keystacksz; i++)
	{
		free(ctx->keystack[i].key);
		ctx->keystack[i].key = NULL;
		ctx->keystack[i].keysz = 0;
	}
	free(ctx->keystack);
	ctx->keystack = NULL;
	ctx->keystacksz = 0;
	ctx->keystackcap = 0;
	while (ctx->n != NULL && ctx->n->parent != NULL)
	{
		ctx->n = ctx->n->parent;
	}
	if (ctx->n)
	{
		cajun_node_free(ctx->n);
	}
	ctx->n = NULL;
	ctx->isdict = 0;
	ctx->isarray = 0;
	ctx->handler = NULL;
	ctx->userdata = NULL;
}
