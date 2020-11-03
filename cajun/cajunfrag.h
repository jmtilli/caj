#ifndef _CAJUNFRAG_H_
#define _CAJUNFRAG_H_

#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajmurmur.h"
#include "cajcontainerof.h"
#include "cajun.h"
#include "../caj.h"
#include <stdint.h>
#include <stdlib.h>

struct cajunfrag_keystack_item {
	char *key;
	size_t keysz;
};

struct cajunfrag_ctx;

struct cajunfrag_handler_vtable {
	int (*start_dict)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz);
	int (*end_dict)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, struct cajun_node *n);
	int (*start_array)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz);
	int (*end_array)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, struct cajun_node *n);
	int (*handle_null)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz);
	int (*handle_string)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, const char *val, size_t valsz);
	int (*handle_number)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, double d);
	int (*handle_boolean)(struct cajunfrag_ctx *ctx, const char *key, size_t keysz, int b);
};


struct cajunfrag_ctx {
	struct cajunfrag_keystack_item *keystack;
	size_t keystacksz;
	size_t keystackcap;
	struct cajun_node *n;
	unsigned isdict:1;
	unsigned isarray:1;
	const struct cajunfrag_handler_vtable *handler;
	void *userdata;
};

int cajunfrag_ctx_is2(struct cajunfrag_ctx *ctx, size_t cnt, ...);
int cajunfrag_ctx_is1(struct cajunfrag_ctx *ctx, size_t cnt, ...);

static inline void cajunfrag_ctx_init(struct cajunfrag_ctx *ctx, const struct cajunfrag_handler_vtable *handler)
{
	ctx->keystack = NULL;
	ctx->keystacksz = 0;
	ctx->keystackcap = 0;
	ctx->n = NULL;
	ctx->isdict = 0;
	ctx->isarray = 0;
	ctx->handler = handler;
	ctx->userdata = NULL;
}

void cajunfrag_ctx_free(struct cajunfrag_ctx *ctx);

int cajunfrag_start_fragment_collection(struct cajunfrag_ctx *fragctx);

int cajunfrag_start_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajunfrag_end_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajunfrag_start_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajunfrag_end_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajunfrag_handle_null(struct caj_handler *cajh, const char *key, size_t keysz);
int cajunfrag_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
int cajunfrag_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d);
int cajunfrag_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b);

const struct caj_handler_vtable cajunfrag_vtable;

#endif
