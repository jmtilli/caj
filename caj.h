#ifndef _CAJ_H_
#define _CAJ_H_

#include <stddef.h>
#include "streamingatof.h"

struct caj_handler;

struct caj_handler_vtable {
	int (*start_dict)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*end_dict)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*start_array)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*end_array)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_null)(struct caj_handler *cajh, const char *key, size_t keysz);
	int (*handle_string)(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
	int (*handle_number)(struct caj_handler *cajh, const char *key, size_t keysz, double d, int is_integer);
	int (*handle_boolean)(struct caj_handler *cajh, const char *key, size_t keysz, int b);
};

struct caj_handler {
	const struct caj_handler_vtable *vtable;
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
	CAJ_MODE_NUMBER,
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
	struct streaming_atof_ctx streamingatof;
	int is_integer;
};

void caj_init(struct caj_ctx *caj, struct caj_handler *handler);

void caj_free(struct caj_ctx *caj);

int caj_feed(struct caj_ctx *caj, const void *vdata, size_t usz, int eof);

struct pullcaj_ctx {
	enum caj_mode mode;
	unsigned char sz; // uescape or token
	char uescape[5];
	unsigned char keypresent:1;
	char *key;
	size_t keysz;
	size_t keycap;
	struct caj_keystack_item *keystack;
	size_t keystacksz;
	size_t keystackcap;
	char *val;
	size_t valsz;
	size_t valcap;
	struct streaming_atof_ctx streamingatof;
	int is_integer;

	const void *vdata;
	size_t usz;
	int eof;
	size_t i;

	int state;
};

enum pullcaj_event {
	CAJ_EV_START_DICT,
	CAJ_EV_END_DICT,
	CAJ_EV_START_ARRAY,
	CAJ_EV_END_ARRAY,
	CAJ_EV_NULL,
	CAJ_EV_STR, // union field str
	CAJ_EV_NUM, // union field num
	CAJ_EV_BOOL, // union field b
};

struct pullcaj_event_info {
	enum pullcaj_event ev;
	const char *key;
	size_t keysz;
	union {
		struct {
			int b;
		} b;
		struct {
			double d;
			int is_integer;
		} num;
		struct {
			const char *val;
			size_t valsz;
		} str;
	} u;
};

void pullcaj_init(struct pullcaj_ctx *pc);

// buffer needs to be valid until next call to this function is made
int pullcaj_set_buf(struct pullcaj_ctx *pc, const void *vdata, size_t usz, int eof);

/*
 * 1: got event
 * 0: end of JSON
 * -EINPROGRESS: need to call again with new buffer
 * <0: another error code, means out of memory or invalid JSON
 */
int pullcaj_get_event(struct pullcaj_ctx *pc, struct pullcaj_event_info *ev);

void pullcaj_free(struct pullcaj_ctx *pc);

#endif
