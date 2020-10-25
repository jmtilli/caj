#ifndef _CAJ_H_
#define _CAJ_H_

#include <stddef.h>

struct caj_handler {
	int (*start_dict)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*end_dict)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*start_array)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*end_array)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_null)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_string)(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
	int (*handle_number)(struct caj_handler *cajh, const char *key, size_t keysz, double d);
	int (*handle_boolean)(struct caj_handler *cajh, const char *key, size_t keysz, int b);
	void *userdata;
};

enum caj_mode {
	CAJ_MODE_KEYSTRING,
	CAJ_MODE_KEYSTRING_ESCAPE,
	CAJ_MODE_KEYSTRING_UESCAPE, // sz tells how many of these we have read
	CAJ_MODE_STRING,
	CAJ_MODE_STRING_ESCAPE,
	CAJ_MODE_STRING_UESCAPE, // sz tells how many of these we have read
	CAJ_MODE_TRUE, // sz tells how many of these we have read
	CAJ_MODE_FALSE, // sz tells how many of these we have read
	CAJ_MODE_NULL, // sz tells how many of these we have read
	CAJ_MODE_FIRSTKEY,
	CAJ_MODE_KEY,
	CAJ_MODE_FIRSTVAL,
	CAJ_MODE_VAL,
	CAJ_MODE_COLON,
	CAJ_MODE_COMMA,
	CAJ_MODE_MANTISSA_SIGN,
	CAJ_MODE_MANTISSA_FIRST,
	CAJ_MODE_MANTISSA,
	CAJ_MODE_MANTISSA_FRAC_FIRST,
	CAJ_MODE_MANTISSA_FRAC,
	CAJ_MODE_EXPONENT_CHAR,
	CAJ_MODE_EXPONENT_SIGN,
	CAJ_MODE_EXPONENT_FIRST,
	CAJ_MODE_EXPONENT,
};

struct caj_keystack_item {
	char *key;
	size_t keysz;
};

struct caj_ctx {
	enum caj_mode mode;
	unsigned char sz; // uescape or token
	char uescape[5];
	unsigned char keypresent:1;
	unsigned char negative:1;
	unsigned char expnegative:1;
	int add_exponent;
	int exponent;
	char *key;
	size_t keysz;
	size_t keycap;
	struct caj_keystack_item *keystack;
	size_t keystacksz;
	size_t keystackcap;
	char *val;
	size_t valsz;
	size_t valcap;
	struct caj_handler *handler;
	double d;
};

void caj_init(struct caj_ctx *caj, struct caj_handler *handler);

int caj_feed(struct caj_ctx *caj, const void *vdata, size_t usz, int eof);

#endif
