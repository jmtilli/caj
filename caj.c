#include "caj.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

void caj_init(struct caj_ctx *caj, struct caj_handler *handler)
{
	memset(caj, 0, sizeof(*caj));
	caj->mode = CAJ_MODE_VAL;
	caj->sz = 0;
	memset(caj->uescape, 0, sizeof(caj->uescape));
	caj->keypresent = 0;
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
}

void caj_free(struct caj_ctx *caj)
{
	free(caj->key);
	caj->key = NULL;
	free(caj->keystack);
	caj->keystack = NULL;
	free(caj->val);
	caj->val = NULL;
	memset(caj, 0, sizeof(*caj));
	caj_init(caj, NULL);
}

static int caj_put_key(struct caj_ctx *caj, char ch)
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

static int caj_put_val(struct caj_ctx *caj, char ch)
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

static int caj_get_keystack(struct caj_ctx *caj)
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

static int caj_put_keystack_1(struct caj_ctx *caj)
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

static inline void caj_put_keystack_2(struct caj_ctx *caj)
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

int caj_feed(struct caj_ctx *caj, const void *vdata, size_t usz, int eof)
{
	const unsigned char *data = (const unsigned char*)vdata;
	const char *cdata = (const char*)vdata;
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
				if (caj_put_key(caj, (char)data[i]) != 0)
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
				if (caj->handler->vtable->handle_string == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->vtable->handle_string(
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
				if (caj_put_val(caj, (char)data[i]) != 0)
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
			caj->uescape[caj->sz++] = (char)data[i];
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
			caj->uescape[caj->sz++] = (char)data[i];
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

		if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
		     data[i] == '\t') && (
		       caj->mode == CAJ_MODE_COLON ||
		       caj->mode == CAJ_MODE_COMMA ||
		       caj->mode == CAJ_MODE_FIRSTKEY ||
		       caj->mode == CAJ_MODE_FIRSTVAL ||
		       caj->mode == CAJ_MODE_KEY ||
		       caj->mode == CAJ_MODE_VAL))
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

				if (caj->handler->vtable->end_dict == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->vtable->end_dict(
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

				if (caj->handler->vtable->end_array == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return (i == sz-1) ? 0 : EOVERFLOW;
					}
					continue;
				}
				ret = caj->handler->vtable->end_array(
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

			if (caj->handler->vtable->start_dict == NULL)
			{
				caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->vtable->start_dict(
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

			if (caj->handler->vtable->start_array == NULL)
			{
				caj_put_keystack_2(caj);
				continue;
			}
			ret = caj->handler->vtable->start_array(
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
			if (data[i] != "true"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_boolean(
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
			if (data[i] != "false"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 5)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_boolean == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_boolean(
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
			if (data[i] != "null"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			if (caj->handler->vtable->handle_null == NULL)
			{
				if (caj->keystacksz <= 0)
				{
					return (i == sz-1) ? 0 : EOVERFLOW;
				}
				continue;
			}
			ret = caj->handler->vtable->handle_null(
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
			caj->mode = CAJ_MODE_NUMBER;
			streaming_atof_init(&caj->streamingatof);
		}
		if (caj->mode == CAJ_MODE_NUMBER)
		{
			size_t tofeed;
			ssize_t szret;
			tofeed = (size_t)(sz - i);
			szret = streaming_atof_feed(&caj->streamingatof, &cdata[i], tofeed);
			if (szret < 0)
			{
				return -EINVAL;
			}
			if (szret > sz - i)
			{
				abort();
			}
			if (szret < sz - i)
			{
				caj->mode = CAJ_MODE_COMMA;
				if (streaming_atof_is_error(&caj->streamingatof))
				{
					return -EINVAL;
				}
				if (caj->handler->vtable->handle_number == NULL)
				{
					if (caj->keystacksz <= 0)
					{
						return -EOVERFLOW;
					}
					i += szret;
					i--;
					continue;
				}
				ret = caj->handler->vtable->handle_number(
					caj->handler,
					caj_get_key(caj), caj_get_keysz(caj),
					streaming_atof_end(&caj->streamingatof));
				if (ret != 0)
				{
					return ret;
				}
				if (caj->keystacksz <= 0)
				{
					return -EOVERFLOW;
				}
			}
			i += szret;
			i--;
			continue;
		}
		return -EINVAL;
	}
	if (caj->mode == CAJ_MODE_NUMBER && eof)
	{
		caj->mode = CAJ_MODE_COMMA;
		if (caj->handler->vtable->handle_number == NULL)
		{
			if (caj->keystacksz <= 0)
			{
				return 0;
			}
			return -EINPROGRESS;
		}
		ret = caj->handler->vtable->handle_number(
			caj->handler,
			caj_get_key(caj), caj_get_keysz(caj),
			streaming_atof_end(&caj->streamingatof));
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
