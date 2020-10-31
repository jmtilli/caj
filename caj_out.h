#ifndef _CAJ_OUT_H_
#define _CAJ_OUT_H_

#include <stddef.h>
#include <stdint.h>

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

int caj_out_put2_start_dict(struct caj_out_ctx *ctx, const char *key, size_t keysz);
int caj_out_put_start_dict(struct caj_out_ctx *ctx, const char *key);
int caj_out_put2_start_array(struct caj_out_ctx *ctx, const char *key, size_t keysz);
int caj_out_put_start_array(struct caj_out_ctx *ctx, const char *key);
int caj_out_add_start_dict(struct caj_out_ctx *ctx);
int caj_out_add_start_array(struct caj_out_ctx *ctx);
int caj_out_end_dict(struct caj_out_ctx *ctx);
int caj_out_end_array(struct caj_out_ctx *ctx);
int caj_out_put22_string(struct caj_out_ctx *ctx, const char *key, size_t keysz, const char *val, size_t valsz);
int caj_out_put_string(struct caj_out_ctx *ctx, const char *key, const char *val);
int caj_out_add2_string(struct caj_out_ctx *ctx, const char *val, size_t valsz);
int caj_out_add_string(struct caj_out_ctx *ctx, const char *val);
int caj_out_put2_number(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int caj_out_put_number(struct caj_out_ctx *ctx, const char *key, double d);
int caj_out_add_number(struct caj_out_ctx *ctx, double d);
int caj_out_put2_flop(struct caj_out_ctx *ctx, const char *key, size_t keysz, double d);
int caj_out_put_flop(struct caj_out_ctx *ctx, const char *key, double d);
int caj_out_add_flop(struct caj_out_ctx *ctx, double d);
int caj_out_put2_i64(struct caj_out_ctx *ctx, const char *key, size_t keysz, int64_t i);
int caj_out_put_i64(struct caj_out_ctx *ctx, const char *key, int64_t i);
int caj_out_add_i64(struct caj_out_ctx *ctx, int64_t i);
int caj_out_put2_boolean(struct caj_out_ctx *ctx, const char *key, size_t keysz, int b);
int caj_out_put_boolean(struct caj_out_ctx *ctx, const char *key, int b);
int caj_out_add_boolean(struct caj_out_ctx *ctx, int b);
int caj_out_put2_null(struct caj_out_ctx *ctx, const char *key, size_t keysz);
int caj_out_put_null(struct caj_out_ctx *ctx, const char *key);
int caj_out_add_null(struct caj_out_ctx *ctx);

void caj_out_init(struct caj_out_ctx *ctx, int tabs, size_t indentamount,
                  int (*datasink)(struct caj_out_ctx *ctx, const char *data, size_t sz),
		  void *userdata);

#endif
