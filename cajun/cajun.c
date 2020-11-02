#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajun.h"
#include "../caj.h"
#include "../caj_out.h"
#include <stdlib.h>

static int cajun_node_out_nonrecursive(struct caj_out_ctx *ctx, struct cajun_node *root)
{
	struct cajun_node *n;
	struct cajun_node *next;
	int ret = 0;
	n = root;
	if (n->type == CAJUN_ARRAY)
	{
		n->u.array.nodeiter = 0;
		if (n->key == NULL || n == root)
		{
			ret = caj_out_add_start_array(ctx);
		}
		else
		{
			ret = caj_out_put2_start_array(ctx, n->key, n->keysz);
		}
	}
	else if (n->type == CAJUN_DICT)
	{
		n->u.dict.lliter = n->u.dict.llhead.node.next;
		if (n->key == NULL || n == root)
		{
			ret = caj_out_add_start_dict(ctx);
		}
		else
		{
			ret = caj_out_put2_start_dict(ctx, n->key, n->keysz);
		}
	}
	if (ret)
	{
		return ret;
	}
	while (n != NULL)
	{
		switch (n->type) {
			case CAJUN_NULL:
				if (n->key == NULL || n == root)
				{
					ret = caj_out_add_null(ctx);
				}
				else
				{
					ret = caj_out_put2_null(ctx, n->key, n->keysz);
				}
				if (ret != 0)
				{
					return ret;
				}
				n = (n == root) ? NULL : n->parent;
				break;
			case CAJUN_BOOL:
				if (n->key == NULL || n == root)
				{
					ret = caj_out_add_boolean(ctx, !!n->u.boolean.b);
				}
				else
				{
					ret = caj_out_put2_boolean(ctx, n->key, n->keysz, !!n->u.boolean.b);
				}
				if (ret != 0)
				{
					return ret;
				}
				n = (n == root) ? NULL : n->parent;
				break;
			case CAJUN_NUMBER:
				if (n->key == NULL || n == root)
				{
					ret = caj_out_add_number(ctx, n->u.number.d);
				}
				else
				{
					ret = caj_out_put2_number(ctx, n->key, n->keysz, n->u.number.d);
				}
				if (ret != 0)
				{
					return ret;
				}
				n = (n == root) ? NULL : n->parent;
				break;
			case CAJUN_STRING:
				if (n->key == NULL || n == root)
				{
					ret = caj_out_add2_string(ctx, n->u.string.s, n->u.string.sz);
				}
				else
				{
					ret = caj_out_put22_string(ctx, n->key, n->keysz, n->u.string.s, n->u.string.sz);
				}
				if (ret != 0)
				{
					return ret;
				}
				n = (n == root) ? NULL : n->parent;
				break;
			case CAJUN_DICT:
				if (n->u.dict.lliter != &n->u.dict.llhead.node)
				{
					next = CAJ_CONTAINER_OF(n->u.dict.lliter, struct cajun_node, llnode);
					n->u.dict.lliter = next->llnode.next;
					if (next->parent != n)
					{
						abort();
					}
					n = next;
					if (n->type == CAJUN_ARRAY)
					{
						n->u.array.nodeiter = 0;
						if (n->key == NULL || n == root)
						{
							ret = caj_out_add_start_array(ctx);
						}
						else
						{
							ret = caj_out_put2_start_array(ctx, n->key, n->keysz);
						}
					}
					else if (n->type == CAJUN_DICT)
					{
						n->u.dict.lliter = n->u.dict.llhead.node.next;
						if (n->key == NULL || n == root)
						{
							ret = caj_out_add_start_dict(ctx);
						}
						else
						{
							ret = caj_out_put2_start_dict(ctx, n->key, n->keysz);
						}
					}
					if (ret)
					{
						return ret;
					}
				}
				else
				{
					n = (n == root) ? NULL : n->parent;
					ret = caj_out_end_dict(ctx);
					if (ret != 0)
					{
						return ret;
					}
				}
				break;
			case CAJUN_ARRAY:
				if (n->u.array.nodeiter < n->u.array.nodesz)
				{
					next = n->u.array.nodes[n->u.array.nodeiter++];
					if (next->parent != n)
					{
						abort();
					}
					n = next;
					if (n->type == CAJUN_ARRAY)
					{
						n->u.array.nodeiter = 0;
						if (n->key == NULL || n == root)
						{
							ret = caj_out_add_start_array(ctx);
						}
						else
						{
							ret = caj_out_put2_start_array(ctx, n->key, n->keysz);
						}
					}
					else if (n->type == CAJUN_DICT)
					{
						n->u.dict.lliter = n->u.dict.llhead.node.next;
						if (n->key == NULL || n == root)
						{
							ret = caj_out_add_start_dict(ctx);
						}
						else
						{
							ret = caj_out_put2_start_dict(ctx, n->key, n->keysz);
						}
					}
					if (ret)
					{
						return ret;
					}
				}
				else
				{
					n = (n == root) ? NULL : n->parent;
					ret = caj_out_end_array(ctx);
					if (ret != 0)
					{
						return ret;
					}
				}
				break;
			default:
				abort();
		}
	}
	return 0;
}

void cajun_node_out(struct caj_out_ctx *ctx, struct cajun_node *n)
{
	cajun_node_out_nonrecursive(ctx, n);
}

int cajun_node_cmp_asym(struct caj_string_plus_len *a, struct caj_rb_tree_node *b, void *ud)
{
	struct cajun_node *n_b = CAJ_CONTAINER_OF(b, struct cajun_node, node);
	size_t s_a = a->len;
	size_t s_b = n_b->keysz;
	size_t s_min = (s_a < s_b ? s_a : s_b);
	const char *b_a = a->str;
	const char *b_b = n_b->key;
	int res;
	res = memcmp(b_a, b_b, s_min);
	if (res != 0)
	{
		return res;
	}
	if (s_a < s_b)
	{
		return -1;
	}
	if (s_a > s_b)
	{
		return 1;
	}
	return 0;
}

int cajun_node_cmp(struct caj_rb_tree_node *a, struct caj_rb_tree_node *b, void *ud)
{
	struct cajun_node *n_a = CAJ_CONTAINER_OF(a, struct cajun_node, node);
	struct cajun_node *n_b = CAJ_CONTAINER_OF(b, struct cajun_node, node);
	size_t s_a = n_a->keysz;
	size_t s_b = n_b->keysz;
	size_t s_min = (s_a < s_b ? s_a : s_b);
	const char *b_a = n_a->key;
	const char *b_b = n_b->key;
	int res;
	res = memcmp(b_a, b_b, s_min);
	if (res != 0)
	{
		return res;
	}
	if (s_a < s_b)
	{
		return -1;
	}
	if (s_a > s_b)
	{
		return 1;
	}
	return 0;
}

struct cajun_node *cajun_dict_get(struct cajun_node *n, const char *key, size_t keysz)
{
	uint32_t hashval = cajun_key_hash(key, keysz);
	size_t hashloc = hashval % (sizeof(n->u.dict.heads)/sizeof(*n->u.dict.heads));
	struct caj_rb_tree_node *rn;
	struct caj_string_plus_len stringlen = {.str = key, .len = keysz};
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	rn = CAJ_RB_TREE_NOCMP_FIND(&n->u.dict.heads[hashloc], cajun_node_cmp_asym, NULL, &stringlen);
	if (rn == NULL)
	{
		return NULL;
	}
	return CAJ_CONTAINER_OF(rn, struct cajun_node, node);
}

int cajun_dict_add(struct cajun_node *parent, const char *key, size_t keysz, struct cajun_node *child)
{
	uint32_t hashval = cajun_key_hash(key, keysz);
	size_t hashloc = hashval % (sizeof(parent->u.dict.heads)/sizeof(*parent->u.dict.heads));
	struct caj_rb_tree_node *rn;
	struct caj_string_plus_len stringlen = {.str = key, .len = keysz};
	char *key2;
	int ret;
	if (parent->type != CAJUN_DICT || child->key != NULL || child->keysz != 0)
	{
		abort();
	}
	rn = CAJ_RB_TREE_NOCMP_FIND(&parent->u.dict.heads[hashloc], cajun_node_cmp_asym, NULL, &stringlen);
	if (rn != NULL)
	{
		return -EEXIST;
	}
	key2 = malloc(keysz + 1);
	if (key2 == NULL)
	{
		return -ENOMEM;
	}
	memcpy(key2, key, keysz);
	key2[keysz] = '\0';
	child->key = key2;
	child->keysz = keysz;
	ret = caj_rb_tree_nocmp_insert_nonexist(&parent->u.dict.heads[hashloc], cajun_node_cmp, NULL, &child->node);
	if (ret != 0)
	{
		free(child->key);
		child->key = NULL;
		child->keysz = 0;
		return ret;
	}
	child->parent = parent;
	caj_linked_list_add_tail(&child->llnode, &parent->u.dict.llhead);
	return 0;
}

int cajun_array_add(struct cajun_node *parent, struct cajun_node *child)
{
	if (parent->type != CAJUN_ARRAY || child->key != NULL || child->keysz != 0)
	{
		abort();
	}
	if (parent->u.array.nodesz >= parent->u.array.nodecap)
	{
		size_t newcap = 2*parent->u.array.nodesz + 16;
		struct cajun_node **newnodes;
		newnodes = realloc(parent->u.array.nodes, newcap * sizeof(*newnodes));
		if (newnodes == NULL)
		{
			return -ENOMEM;
		}
		parent->u.array.nodes = newnodes;
		parent->u.array.nodecap = newcap;
	}
	parent->u.array.nodes[parent->u.array.nodesz++] = child;
	child->parent = parent;
	return 0;
}

int cajun_start_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *dict = cajun_node_new();
	int ret = 0;
	if (dict == NULL)
	{
		return -ENOMEM;
	}
	cajun_dict_init(dict);
	if (ctx->nsz >= ctx->ncap)
	{
		size_t newcap = 2*ctx->nsz + 16;
		struct cajun_node **newns;
		newns = realloc(ctx->ns, newcap * sizeof(*ctx->ns));
		if (newns == NULL)
		{
			cajun_node_free(dict);
			free(dict);
			return -ENOMEM;
		}
		ctx->ns = newns;
		ctx->ncap = newcap;
	}
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, dict);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], dict);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(dict);
		free(dict);
		return ret;
	}
	ctx->ns[ctx->nsz++] = dict;
	if (ctx->n == NULL)
	{
		ctx->n = dict;
	}
	return 0;
}
int cajun_end_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	if (ctx->nsz == 0)
	{
		abort();
	}
	ctx->nsz--;
	return 0;
}
int cajun_start_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *ar = cajun_node_new();
	int ret = 0;
	if (ar == NULL)
	{
		return -ENOMEM;
	}
	cajun_array_init(ar);
	if (ctx->nsz >= ctx->ncap)
	{
		size_t newcap = 2*ctx->nsz + 16;
		struct cajun_node **newns;
		newns = realloc(ctx->ns, newcap * sizeof(*ctx->ns));
		if (newns == NULL)
		{
			cajun_node_free(ar);
			free(ar);
			return -ENOMEM;
		}
		ctx->ns = newns;
		ctx->ncap = newcap;
	}
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, ar);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], ar);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(ar);
		free(ar);
		return ret;
	}
	ctx->ns[ctx->nsz++] = ar;
	if (ctx->n == NULL)
	{
		ctx->n = ar;
	}
	return 0;
}
int cajun_end_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	if (ctx->nsz == 0)
	{
		abort();
	}
	ctx->nsz--;
	return 0;
}
int cajun_handle_null(struct caj_handler *cajh, const char *key, size_t keysz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *n = cajun_node_new();
	int ret = 0;
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_null_init(n);
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	if (ctx->n == NULL)
	{
		ctx->n = n;
	}
	return 0;
}
int cajun_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *n = cajun_node_new();
	int ret = 0;
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
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	if (ctx->n == NULL)
	{
		ctx->n = n;
	}
	return 0;
}
int cajun_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *n = cajun_node_new();
	int ret = 0;
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_number_init(n, d);
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	if (ctx->n == NULL)
	{
		ctx->n = n;
	}
	return 0;
}
int cajun_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b)
{
	struct cajun_ctx *ctx = cajh->userdata;
	struct cajun_node *n = cajun_node_new();
	int ret = 0;
	if (n == NULL)
	{
		return -ENOMEM;
	}
	cajun_boolean_init(n, b);
	if (ctx->nsz > 0)
	{
		if (ctx->ns[ctx->nsz-1]->type == CAJUN_DICT)
		{
			ret = cajun_dict_add(ctx->ns[ctx->nsz-1], key, keysz, n);
		}
		else
		{
			ret = cajun_array_add(ctx->ns[ctx->nsz-1], n);
		}
	}
	if (ret != 0)
	{
		cajun_node_free(n);
		free(n);
		return ret;
	}
	if (ctx->n == NULL)
	{
		ctx->n = n;
	}
	return 0;
}

static void cajun_node_free_nonrecursive(struct cajun_node *n)
{
	struct cajun_node *next;
	if (n == NULL)
	{
		return;
	}
	if (n->key != NULL || n->keysz != 0)
	{
		abort();
	}
	free(n->key);
	n->key = NULL;
	n->keysz = 0;
	while (n != NULL)
	{
		switch (n->type) {
			case CAJUN_NULL:
			case CAJUN_BOOL:
			case CAJUN_NUMBER:
				next = n->parent;
				memset(n, 0, sizeof(*n));
				n->type = CAJUN_NULL;
				if (next != NULL)
				{
					free(n);
				}
				n = next;
				break;
			case CAJUN_STRING:
				next = n->parent;
				free(n->u.string.s);
				n->u.string.s = NULL;
				n->u.string.sz = 0;
				memset(n, 0, sizeof(*n));
				n->type = CAJUN_NULL;
				if (next != NULL)
				{
					free(n);
				}
				n = next;
				break;
			case CAJUN_DICT:
				if (!caj_linked_list_is_empty(&n->u.dict.llhead))
				{
					struct caj_linked_list_node *node;
					node = n->u.dict.llhead.node.next;
					caj_linked_list_delete(node);
					next = CAJ_CONTAINER_OF(node, struct cajun_node, llnode);
					if (next->parent != n)
					{
						abort();
					}
					n = next;
					free(n->key);
					n->key = NULL;
					n->keysz = 0;
				}
				else
				{
					next = n->parent;
					memset(n, 0, sizeof(*n));
					n->type = CAJUN_NULL;
					if (next != NULL)
					{
						free(n);
					}
					n = next;
				}
				break;
			case CAJUN_ARRAY:
				if (n->u.array.nodesz)
				{
					next = n->u.array.nodes[--n->u.array.nodesz];
					if (next->parent != n)
					{
						abort();
					}
					n = next;
				}
				else
				{
					next = n->parent;
					free(n->u.array.nodes);
					n->u.array.nodes = NULL;
					memset(n, 0, sizeof(*n));
					n->type = CAJUN_NULL;
					if (next != NULL)
					{
						free(n);
					}
					n = next;
				}
				break;
			default:
				abort();
		}
	}
}

void cajun_node_free(struct cajun_node *n)
{
	if (n == NULL)
	{
		return;
	}
	if (n->parent != NULL)
	{
		abort();
	}
	cajun_node_free_nonrecursive(n);
}

const struct caj_handler_vtable cajun_vtable = {
	.start_dict = cajun_start_dict,
	.end_dict = cajun_end_dict,
	.start_array = cajun_start_array,
	.end_array = cajun_end_array,
	.handle_null = cajun_handle_null,
	.handle_string = cajun_handle_string,
	.handle_number = cajun_handle_number,
	.handle_boolean = cajun_handle_boolean,
};
