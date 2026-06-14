#ifndef _PULLCAJ_H_
#define _PULLCAJ_H_

#include <stddef.h>
#include "caj.h" // enum caj_mode
#include "streamingatof.h"

struct pullcaj_ctx {
	enum caj_mode mode;
	unsigned char sz; // uescape or token
	char uescape[5];
	unsigned char keypresent:1;
	unsigned char cpp_comment_seen:1;
	unsigned char c_comment_seen:1;
	unsigned char c_comment_seen_star:1;
	unsigned char comments:1;
	unsigned char comment_seen_preliminary:1;
	unsigned char comma_seen:1;
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
	CAJ_EV_COMMENT, // union field comm
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
		struct {
			const char *comment;
			size_t commentsz;
			unsigned char comma_seen:1;
		} comm;
	} u;
};

struct pullcaj_ctx *pullcaj_new(void); // pullcaj_delete later

void pullcaj_init(struct pullcaj_ctx *pc);

void pullcaj_allow_comments(struct pullcaj_ctx *caj);

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

void pullcaj_delete(struct pullcaj_ctx *pc);

#endif
