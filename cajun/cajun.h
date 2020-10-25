#ifndef _CAJUN_H_
#define _CAJUN_H_

#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajmurmur.h"
#include "cajcontainerof.h"
#include "../caj.h"
#include <stdint.h>
#include <stdlib.h>

enum cajun_type {
	CAJUN_DICT,
	CAJUN_ARRAY,
	CAJUN_STRING,
	CAJUN_NUMBER,
	CAJUN_BOOL,
	CAJUN_NULL
};
struct cajun_node {
	char *key;
	size_t keysz;
	enum cajun_type type;
	struct caj_rb_tree_node node;
	struct caj_linked_list_node llnode;
	union {
		struct {
			struct caj_rb_tree_nocmp heads[8];
			struct caj_linked_list_head llhead;
		} dict;
		struct {
			struct cajun_node **nodes;
			size_t nodesz;
			size_t nodecap;
		} array;
		struct {
			char *s;
			size_t sz;
		} string;
		struct {
			double d;
		} number;
		struct {
			int b;
		} boolean;
	} u;
};

static inline void cajun_node_init(struct cajun_node *n)
{
	memset(n, 0, sizeof(*n));
	n->key = NULL;
	n->keysz = 0;
	caj_linked_list_node_init(&n->llnode);
	n->type = CAJUN_NULL;
}

static inline struct cajun_node *cajun_node_new(void)
{
	struct cajun_node *result;
	result = malloc(sizeof(*result));
	if (result == NULL)
	{
		return NULL;
	}
	cajun_node_init(result);
	return result;
}

static inline void cajun_null_init(struct cajun_node *n)
{
	cajun_node_init(n);
	n->type = CAJUN_NULL;
}

static inline int cajun_string_init(struct cajun_node *n, const char *val, size_t valsz)
{
	char *val2;
	cajun_node_init(n);
	val2 = malloc(valsz + 1);
	if (val2 == NULL)
	{
		return -ENOMEM;
	}
	memcpy(val2, val, valsz);
	val2[valsz] = '\0';
	n->type = CAJUN_STRING;
	n->u.string.s = val2;
	n->u.string.sz = valsz;
	return 0;
}

static inline void cajun_array_init(struct cajun_node *n)
{
	cajun_node_init(n);
	n->type = CAJUN_ARRAY;
	n->u.array.nodes = NULL;
	n->u.array.nodesz = 0;
	n->u.array.nodecap = 0;
}

static inline void cajun_number_init(struct cajun_node *n, double d)
{
	cajun_node_init(n);
	n->type = CAJUN_NUMBER;
	n->u.number.d = d;
}

static inline void cajun_boolean_init(struct cajun_node *n, int b)
{
	cajun_node_init(n);
	n->type = CAJUN_BOOL;
	n->u.boolean.b = !!b;
}

static inline void cajun_dict_init(struct cajun_node *n)
{
	size_t i;
	cajun_node_init(n);
	n->type = CAJUN_DICT;
	caj_linked_list_head_init(&n->u.dict.llhead);
	for (i = 0; i < sizeof(n->u.dict.heads)/sizeof(*n->u.dict.heads); i++)
	{
		caj_rb_tree_nocmp_init(&n->u.dict.heads[i]);
	}
}

static inline uint32_t cajun_key_hash(const char *key, size_t keysz)
{
	return caj_murmur_buf(0x12345678U, key, keysz);
}
static inline uint32_t cajun_node_hash(struct cajun_node *n)
{
	return cajun_key_hash(n->key, n->keysz);
}

struct caj_string_plus_len
{
	const char *str;
	size_t len;
};

int cajun_node_cmp_asym(struct caj_string_plus_len *a, struct caj_rb_tree_node *b, void *ud);

int cajun_node_cmp(struct caj_rb_tree_node *a, struct caj_rb_tree_node *b, void *ud);

struct cajun_node *cajun_dict_get(struct cajun_node *n, const char *key, size_t keysz);

int cajun_dict_add(struct cajun_node *parent, const char *key, size_t keysz, struct cajun_node *child);

int cajun_array_add(struct cajun_node *parent, struct cajun_node *child);

struct cajun_ctx {
	struct cajun_node *n;
	struct cajun_node **ns;
	size_t nsz;
	size_t ncap;
};

static inline void cajun_ctx_init(struct cajun_ctx *ctx)
{
	ctx->n = NULL;
	ctx->ns = NULL;
	ctx->nsz = 0;
	ctx->ncap = 0;
}

int cajun_start_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_end_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_start_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_end_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_handle_null(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
int cajun_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d);
int cajun_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b);

#endif
