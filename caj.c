#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

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

void caj_init(struct caj_ctx *caj, struct caj_handler *handler)
{
	caj->mode = CAJ_MODE_VAL;
	caj->sz = 0;
	memset(caj->uescape, 0, sizeof(caj->uescape));
	caj->keypresent = 0;
	caj->negative = 0;
	caj->expnegative = 0;
	caj->add_exponent = 0;
	caj->exponent = 0;
	caj->key = NULL;
	caj->keysz = 0;
	caj->keycap = 0;
	caj->keystack = NULL;
	caj->keystacksz = 0;
	caj->keystackcap = 0;
	caj->val = NULL;
	caj->valsz = 0;
	caj->valcap = 0;
	caj->handler = handler;
	caj->d = 0;
}

int caj_put_key(struct caj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->keysz >= caj->keycap)
	{
		newcap = caj->keysz * 2 + 16;
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}
	caj->key[caj->keysz++] = ch;
	return 0;
}

int caj_put_val(struct caj_ctx *caj, char ch)
{
	size_t newcap = 0;
	char *newbuf;
	if (caj->valsz >= caj->valcap)
	{
		newcap = caj->valsz * 2 + 16;
		newbuf = realloc(caj->val, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->val = newbuf;
		caj->valcap = newcap;
	}
	caj->val[caj->valsz++] = ch;
	return 0;
}

int caj_get_keystack(struct caj_ctx *caj)
{
	size_t newcap;
	char *newbuf;

	newcap = caj->keystack[caj->keystacksz-1].keysz + 1;
	if (newcap > caj->keycap)
	{
		newbuf = realloc(caj->key, newcap);
		if (newbuf == NULL)
		{
			return -ENOMEM;
		}
		caj->key = newbuf;
		caj->keycap = newcap;
	}

	if (caj->keystack[caj->keystacksz-1].key == NULL)
	{
		caj->keypresent = 0;
		caj->keystacksz--;
		return 0;
	}

	caj->keypresent = 1;

	caj->keysz = newcap-1;
	memcpy(caj->key, caj->keystack[caj->keystacksz-1].key, caj->keysz);
	caj->key[caj->keysz] = '\0';

	free(caj->keystack[caj->keystacksz-1].key);
	caj->keystack[caj->keystacksz-1].key = NULL;
	caj->keystack[caj->keystacksz-1].keysz = 0;
	caj->keystacksz--;

	return 0;
}
int caj_put_keystack_1(struct caj_ctx *caj)
{
	size_t newcap;
	char *newbuf;
	struct caj_keystack_item *newstack;
	if (caj->keystacksz >= caj->keystackcap)
	{
		newcap = caj->keystacksz * 2 + 16;
		newstack = realloc(caj->keystack, newcap * sizeof(*newstack));
		if (newstack == NULL)
		{
			return -ENOMEM;
		}
		caj->keystack = newstack;
		caj->keystackcap = newcap;
	}

	if (caj->keypresent == 0)
	{
		caj->keystack[caj->keystacksz].key = NULL;
		caj->keystack[caj->keystacksz].keysz = 0;
		caj->keystacksz++;
		caj->keypresent = 0;
		return 0;
	}

	newcap = caj->keysz + 1;
	newbuf = malloc(newcap);
	if (newbuf == NULL)
	{
		return -ENOMEM;
	}
	memcpy(newbuf, caj->key, caj->keysz);
	newbuf[caj->keysz] = '\0';
	caj->keystack[caj->keystacksz].key = newbuf;
	caj->keystack[caj->keystacksz].keysz = caj->keysz;
	caj->keystacksz++;
	return 0;
}
void caj_put_keystack_2(struct caj_ctx *caj)
{
	caj->keypresent = 0;
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

static inline double caj_get_number(struct caj_ctx *caj)
{
	double d = caj->d;
	int exponent = caj->exponent;
	if (caj->expnegative)
	{
		exponent = -exponent;
	}
	d *= myintpow10(exponent - caj->add_exponent);
	if (caj->negative)
	{
		d = -d;
	}
	return d;
}

static inline size_t caj_get_keysz(struct caj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return 0;
	}
	return caj->keysz;
}
static inline char *caj_get_key(struct caj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return NULL;
	}
	return caj->key;
}

int caj_feed(struct caj_ctx *caj, const void *vdata, size_t usz)
{
	const unsigned char *data = (const unsigned char*)vdata;
	ssize_t sz = (ssize_t)usz;
	ssize_t i;
	int ret;
	// FIXME check for ssize_t overflow
	for (i = 0; i < sz; i++)
	{
		if (caj->mode == CAJ_MODE_KEYSTRING)
		{
			if (data[i] == '\\')
			{
				caj->mode = CAJ_MODE_KEYSTRING_ESCAPE;
			}
			else if (data[i] == '"')
			{
				if (caj_put_key(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->keysz--;
				caj->keypresent = 1;
				caj->mode = CAJ_MODE_COLON;
			}
			else
			{
				if (caj_put_key(caj, data[i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING)
		{
			if (data[i] == '\\')
			{
				caj->mode = CAJ_MODE_STRING_ESCAPE;
			}
			else if (data[i] == '"')
			{
				if (caj_put_val(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->valsz--;
				caj->mode = CAJ_MODE_COMMA;
				if (caj->handler->handle_string == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->handle_string(
					caj->handler,
					caj_get_key(caj), caj_get_keysz(caj),
					caj->val, caj->valsz);
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
			}
			else
			{
				if (caj_put_val(caj, data[i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_KEYSTRING_ESCAPE)
		{
			int res = 0;
			switch (data[i])
			{
				case 'b':
					res = caj_put_key(caj, '\b');
					break;
				case 'f':
					res = caj_put_key(caj, '\f');
					break;
				case 'n':
					res = caj_put_key(caj, '\n');
					break;
				case 'r':
					res = caj_put_key(caj, '\r');
					break;
				case 't':
					res = caj_put_key(caj, '\t');
					break;
				case 'u':
					caj->mode = CAJ_MODE_KEYSTRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_KEYSTRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = data[i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (caj_put_key(caj, codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (caj_put_key(caj, 0xC0 | (codepoint>>6)) != 0)
				{
					return -ENOMEM;
				}
				if (caj_put_key(caj, 0x80 | (codepoint & 0x3F)) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (caj_put_key(caj, 0xE0 | (codepoint>>12)) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_key(caj, 0x80 | ((codepoint>>6)&0x3F)) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_key(caj, 0x80 | ((codepoint>>0)&0x3F)) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_KEYSTRING;
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = data[i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (caj_put_val(caj, codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (caj_put_val(caj, 0xC0 | (codepoint>>6)) != 0)
				{
					return -ENOMEM;
				}
				if (caj_put_val(caj, 0x80 | (codepoint & 0x3F)) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (caj_put_val(caj, 0xE0 | (codepoint>>12)) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_val(caj, 0x80 | ((codepoint>>6)&0x3F)) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_val(caj, 0x80 | ((codepoint>>0)&0x3F)) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_STRING;
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING_ESCAPE)
		{
			int res = 0;
			switch (data[i])
			{
				case 'b':
					res = caj_put_val(caj, '\b');
					break;
				case 'f':
					res = caj_put_val(caj, '\f');
					break;
				case 'n':
					res = caj_put_val(caj, '\n');
					break;
				case 'r':
					res = caj_put_val(caj, '\r');
					break;
				case 't':
					res = caj_put_val(caj, '\t');
					break;
				case 'u':
					caj->mode = CAJ_MODE_STRING_UESCAPE;
					caj->sz = 0;
					break;
				default:
					return -EILSEQ;
			}
			if (res != 0)
			{
				return res;
			}
			continue;
		}

		if (data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
		    data[i] == '\t')
		{
			continue;
		}

		if (caj->mode == CAJ_MODE_COLON)
		{
			if (data[i] != ':')
			{
				return -EINVAL;
			}
			caj->mode = CAJ_MODE_VAL;
			continue;
		}
		if (caj->mode == CAJ_MODE_COMMA && data[i] == ',')
		{
			if (data[i] == ',')
			{
				if (caj->keypresent)
				{
					caj->mode = CAJ_MODE_KEY;
					caj->keypresent = 0;
				}
				else
				{
					caj->mode = CAJ_MODE_VAL;
				}
				continue;
			}
		}
		if ((caj->mode == CAJ_MODE_COMMA || caj->mode == CAJ_MODE_FIRSTKEY) && data[i] == '}')
		{
			if (data[i] == '}')
			{
				if (caj->mode == CAJ_MODE_COMMA)
				{
					if (!caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = CAJ_MODE_COMMA;

				if (caj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				if (caj->handler->end_dict == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->end_dict(
					caj->handler,
					caj_get_key(caj), caj_get_keysz(caj));
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
		}
		if ((caj->mode == CAJ_MODE_COMMA || caj->mode == CAJ_MODE_FIRSTVAL) && data[i] == ']')
		{
			if (data[i] == ']')
			{
				if (caj->mode == CAJ_MODE_COMMA)
				{
					if (caj->keypresent)
					{
						return -EINVAL;
					}
					// could be array or dict
				}
				caj->mode = CAJ_MODE_COMMA;

				if (caj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				if (caj->handler->end_array == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->end_array(
					caj->handler,
					caj_get_key(caj), caj_get_keysz(caj));
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
		}
		if ((caj->mode == CAJ_MODE_FIRSTVAL || caj->mode == CAJ_MODE_VAL) && data[i] == '{')
		{
			if (caj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_FIRSTKEY;

			if (caj->handler->start_dict == NULL)
			{
				caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->start_dict(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj));
			caj_put_keystack_2(caj);
			if (ret != 0)
			{
				return ret;
			}
			continue;
		}
		if ((caj->mode == CAJ_MODE_FIRSTVAL || caj->mode == CAJ_MODE_VAL) && data[i] == '[')
		{
			if (caj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_FIRSTVAL;

			if (caj->handler->start_array == NULL)
			{
				caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->start_array(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj));
			caj_put_keystack_2(caj);
			if (ret != 0)
			{
				return ret;
			}
			continue;
		}

		if (caj->mode == CAJ_MODE_TRUE)
		{
			if (data[i] != "true"[sz++])
			{
				return -EINVAL;
			}
			if (sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_boolean(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj),
				1);
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return (i == sz-1) ? 0 : EOVERFLOW;
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_FALSE)
		{
			if (data[i] != "false"[sz++])
			{
				return -EINVAL;
			}
			if (sz < 5)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_boolean(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj),
				0);
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return (i == sz-1) ? 0 : EOVERFLOW;
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_NULL)
		{
			if (data[i] != "null"[sz++])
			{
				return -EINVAL;
			}
			if (sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->handle_null == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_null(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj));
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return (i == sz-1) ? 0 : EOVERFLOW;
			}
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[i] == 'n')
		{
			caj->mode = CAJ_MODE_NULL;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[i] == 'f')
		{
			caj->mode = CAJ_MODE_FALSE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[i] == 't')
		{
			caj->mode = CAJ_MODE_TRUE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_KEY || caj->mode == CAJ_MODE_FIRSTKEY) && data[i] == '"')
		{
			caj->mode = CAJ_MODE_KEYSTRING;
			caj->keysz = 0;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[i] == '"')
		{
			caj->mode = CAJ_MODE_STRING;
			caj->valsz = 0;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && (isdigit((unsigned char)data[i]) || data[i] == '-'))
		{
			caj->negative = 0;
			if (data[i] == '-')
			{
				caj->negative = 1;
				caj->mode = CAJ_MODE_MANTISSA_FIRST;
				continue;
			}
			caj->mode = CAJ_MODE_MANTISSA_FIRST;
		}
		if (caj->mode == CAJ_MODE_MANTISSA_FIRST)
		{
			caj->d = 0;
			caj->add_exponent = 0;
			caj->exponent = 0;
			caj->expnegative = 0;
		}
		if (caj->mode == CAJ_MODE_MANTISSA_FIRST && data[i] == '0')
		{
			caj->mode = CAJ_MODE_EXPONENT_CHAR;
			continue;
		}
		if (caj->mode == CAJ_MODE_EXPONENT_CHAR)
		{
			if (data[i] == 'e' || data[i] == 'E')
			{
				caj->mode = CAJ_MODE_EXPONENT_SIGN;
				continue;
			}
			return -EINVAL;
		}
		if ((caj->mode == CAJ_MODE_MANTISSA || caj->mode == CAJ_MODE_MANTISSA_FIRST))
		{
			caj->mode = CAJ_MODE_MANTISSA;
			if (isdigit(data[i]))
			{
				caj->d *= 10;
				caj->d += (data[i] - '0');
				continue;
			}
			if (caj->mode == CAJ_MODE_MANTISSA_FIRST)
			{
				return -EINVAL;
			}
			if (data[i] == '.')
			{
				caj->mode = CAJ_MODE_MANTISSA_FRAC_FIRST;
				continue;
			}
			if (data[i] == 'e' || data[i] == 'E')
			{
				caj->mode = CAJ_MODE_EXPONENT_SIGN;
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			i--;
			if (caj->handler->handle_number == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return -EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_number(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj),
				caj_get_number(caj));
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return -EOVERFLOW;
			}
			continue;
		}
		if (caj->mode == CAJ_MODE_MANTISSA_FRAC || caj->mode == CAJ_MODE_MANTISSA_FRAC_FIRST)
		{
			if (isdigit(data[i]))
			{
				caj->add_exponent++;
				caj->d *= 10;
				caj->d += (data[i] - '0');
				caj->mode = CAJ_MODE_MANTISSA_FRAC;
				continue;
			}
			if (caj->mode == CAJ_MODE_MANTISSA_FRAC_FIRST)
			{
				return -EINVAL;
			}
			if (data[i] == 'e' || data[i] == 'E')
			{
				caj->mode = CAJ_MODE_EXPONENT_SIGN;
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			i--;
			if (caj->handler->handle_number == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return -EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_number(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj),
				caj_get_number(caj));
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return -EOVERFLOW;
			}
			continue;
		}
		if (caj->mode == CAJ_MODE_EXPONENT_SIGN)
		{
			if (data[i] == '+')
			{
				caj->expnegative = 0;
				caj->mode = CAJ_MODE_EXPONENT_FIRST;
				continue;
			}
			if (data[i] == '-')
			{
				caj->expnegative = 1;
				caj->mode = CAJ_MODE_EXPONENT_FIRST;
				continue;
			}
			if (isdigit(data[i]))
			{
				caj->exponent *= 10;
				caj->exponent += (data[i] - '0');
				caj->mode = CAJ_MODE_EXPONENT;
				continue;
			}
			return -EINVAL;
		}
		if (caj->mode == CAJ_MODE_EXPONENT_FIRST || caj->mode == CAJ_MODE_EXPONENT)
		{
			if (isdigit(data[i]))
			{
				caj->exponent *= 10;
				caj->exponent += (data[i] - '0');
				caj->mode = CAJ_MODE_EXPONENT;
				continue;
			}
			if (caj->mode == CAJ_MODE_EXPONENT_FIRST)
			{
				return -EINVAL;
			}
			caj->mode = CAJ_MODE_COMMA;
			i--;
			if (caj->handler->handle_number == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return -EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->handle_number(
				caj->handler,
				caj_get_key(caj), caj_get_keysz(caj),
				caj_get_number(caj));
			if (ret != 0)
			{
				return ret;
			}
			if (caj->keystacksz <= 0)
			{
				return -EOVERFLOW;
			}
			continue;
		}
		return -EINVAL;
	}
	if (caj->mode == CAJ_MODE_EXPONENT || caj->mode == CAJ_MODE_MANTISSA ||
	    caj->mode == CAJ_MODE_MANTISSA_FRAC)
	{
		if (caj->handler->handle_number == NULL)
		{
			if (caj->keystacksz <= 0)
			{
				return 0;
			}
			return -EINPROGRESS;
		}
		ret = caj->handler->handle_number(
			caj->handler,
			caj_get_key(caj), caj_get_keysz(caj),
			caj_get_number(caj));
		if (ret != 0)
		{
			return ret;
		}
		if (caj->keystacksz <= 0)
		{
			return 0;
		}
		return -EINPROGRESS;
	}
	return -EINPROGRESS;
}

struct caj_ctx ctx;

void caj_dump(void)
{
	//printf("keystacksz %d\n", (int)ctx.keystacksz);
}

int my_start_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("start dict %s\n", key);
	return 0;
}
int my_end_dict(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("end dict %s\n", key);
	return 0;
}
int my_start_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("start array %s\n", key);
	return 0;
}
int my_end_array(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("end array %s\n", key);
	return 0;
}
int my_handle_null(struct caj_handler *cajh, const char *key, size_t keysz)
{
	caj_dump();
	printf("%s: null\n", key);
	return 0;
}
int my_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz)
{
	caj_dump();
	printf("%s: %s\n", key, val);
	return 0;
}
int my_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d)
{
	caj_dump();
	printf("%s: %g\n", key, d);
	return 0;
}
int my_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b)
{
	caj_dump();
	printf("%s: %s\n", key, b ? "true" : "false");
	return 0;
}

struct caj_handler myhandler = {
	.start_dict = my_start_dict,
	.end_dict = my_end_dict,
	.start_array = my_start_array,
	.end_array = my_end_array,
	.handle_null = my_handle_null,
	.handle_string = my_handle_string,
	.handle_number = my_handle_number,
	.handle_boolean = my_handle_boolean,
	.userdata = NULL,
};

int main(int argc, char **argv)
{
	char *data = " {   \"foo\": [1, 2, 3], \"bar\": 4, \"baz\": {}, \"barf\": []     }";
	int ret;
	caj_init(&ctx, &myhandler);
	ret = caj_feed(&ctx, data, strlen(data));
	if (ret != 0)
	{
		printf("ret %d\n", ret);
	}
	return 0;
}
