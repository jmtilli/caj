#include "caj.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

typedef ptrdiff_t myssize_t;

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

void pullcaj_init(struct pullcaj_ctx *caj)
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

	caj->vdata = NULL;
	caj->usz = 0;
	caj->eof = 0;
	caj->i = 0;

	caj->state = 0;
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

void pullcaj_free(struct pullcaj_ctx *caj)
{
	free(caj->key);
	caj->key = NULL;
	free(caj->keystack);
	caj->keystack = NULL;
	free(caj->val);
	caj->val = NULL;
	memset(caj, 0, sizeof(*caj));
	pullcaj_init(caj);
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

static int pullcaj_put_key(struct pullcaj_ctx *caj, char ch)
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

static int pullcaj_put_val(struct pullcaj_ctx *caj, char ch)
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

static int pullcaj_get_keystack(struct pullcaj_ctx *caj)
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

static int pullcaj_put_keystack_1(struct pullcaj_ctx *caj)
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

static inline void pullcaj_put_keystack_2(struct pullcaj_ctx *caj)
{
	caj->keypresent = 0;
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

static inline size_t pullcaj_get_keysz(struct pullcaj_ctx *caj)
{
	if (caj->keypresent == 0)
	{
		return 0;
	}
	return caj->keysz;
}
static inline char *pullcaj_get_key(struct pullcaj_ctx *caj)
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
	myssize_t sz = (myssize_t)usz;
	myssize_t i;
	int ret;
	if (sz < 0 || (size_t)sz != usz)
	{
		return -EFAULT;
	}
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
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
				if (caj_put_key(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (caj_put_key(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (caj_put_key(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (caj_put_key(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_key(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_key(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
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
				if (caj_put_val(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (caj_put_val(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (caj_put_val(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (caj_put_val(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_val(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (caj_put_val(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
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
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
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
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
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
				i++;
				while (i < sz)
				{
					if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
					     data[i] == '\t'))
					{
						i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
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
			caj->is_integer = 1;
			streaming_atof_init_strict_json(&caj->streamingatof);
		}
		if (caj->mode == CAJ_MODE_NUMBER)
		{
			size_t tofeed;
			myssize_t szret;
			myssize_t j;
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
			for (j = i; j < i + szret; j++)
			{
				if (cdata[j] == '.' || cdata[j] == 'e')
				{
					caj->is_integer = 0;
				}
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
					i += szret;
					i--;
					if (caj->keystacksz <= 0)
					{
						i++;
						while (i < sz)
						{
							if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
							     data[i] == '\t'))
							{
								i++;
								continue;
							}
							return -EOVERFLOW;
						}
						return 0;
					}
					continue;
				}
				ret = caj->handler->vtable->handle_number(
					caj->handler,
					caj_get_key(caj), caj_get_keysz(caj),
					streaming_atof_end(&caj->streamingatof),
					caj->is_integer);
				if (ret != 0)
				{
					return ret;
				}
				i += szret;
				i--;
				if (caj->keystacksz <= 0)
				{
					i++;
					while (i < sz)
					{
						if ((data[i] == ' ' || data[i] == '\n' || data[i] == '\r' ||
						     data[i] == '\t'))
						{
							i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				i += szret;
				i--;
			}
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
			streaming_atof_end(&caj->streamingatof),
			caj->is_integer);
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

int pullcaj_set_buf(struct pullcaj_ctx *pc, const void *vdata, size_t usz, int eof)
{
	myssize_t sz = (myssize_t)usz;
	if (vdata == NULL && usz > 0)
	{
		return -EFAULT;
	}
	if (sz < 0 || (size_t)sz != usz)
	{
		return -EFAULT;
	}
	if (pc->eof)
	{
		return -EFAULT;
	}
	if (pc->i < pc->usz)
	{
		return -EINTR;
	}
	pc->i = 0;
	pc->usz = usz;
	pc->vdata = vdata;
	pc->eof = eof;
	return 0;
}

int pullcaj_get_event(struct pullcaj_ctx *caj, struct pullcaj_event_info *ev)
{
	const unsigned char *data = (const unsigned char*)caj->vdata;
	const char *cdata = (const char*)caj->vdata;
	switch(caj->state) {
		case 0:
			break;
		case 1:
			goto state1;
		case 2:
			goto state2;
		case 3:
			goto state3;
		case 4:
			goto state4;
		case 5:
			goto state5;
		case 6:
			goto state6;
		case 7:
			goto state7;
		case 8:
			goto state8;
		case 9:
			goto state9;
		case 10:
			goto state10;
		default:
			abort();
	}
	for (; caj->i < caj->usz; caj->i++)
	{
		if (caj->mode == CAJ_MODE_KEYSTRING)
		{
			if (data[caj->i] == '\\')
			{
				caj->mode = CAJ_MODE_KEYSTRING_ESCAPE;
			}
			else if (data[caj->i] == '"')
			{
				if (pullcaj_put_key(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->keysz--;
				caj->keypresent = 1;
				caj->mode = CAJ_MODE_COLON;
			}
			else
			{
				if (pullcaj_put_key(caj, (char)data[caj->i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING)
		{
			if (data[caj->i] == '\\')
			{
				caj->mode = CAJ_MODE_STRING_ESCAPE;
			}
			else if (data[caj->i] == '"')
			{
				if (pullcaj_put_val(caj, '\0') != 0)
				{
					return -ENOMEM;
				}
				caj->valsz--;
				caj->mode = CAJ_MODE_COMMA;
				ev->ev = CAJ_EV_STR;
				ev->key = pullcaj_get_key(caj);
				ev->keysz = pullcaj_get_keysz(caj);
				ev->u.str.val = caj->val;
				ev->u.str.valsz = caj->valsz;
				caj->state = 1;
				return 1;
state1:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				if (pullcaj_put_val(caj, (char)data[caj->i]) != 0)
				{
					return -ENOMEM;
				}
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_KEYSTRING_ESCAPE)
		{
			int res = 0;
			switch (data[caj->i])
			{
				case 'b':
					res = pullcaj_put_key(caj, '\b');
					break;
				case 'f':
					res = pullcaj_put_key(caj, '\f');
					break;
				case 'n':
					res = pullcaj_put_key(caj, '\n');
					break;
				case 'r':
					res = pullcaj_put_key(caj, '\r');
					break;
				case 't':
					res = pullcaj_put_key(caj, '\t');
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
			if (!isxdigit((unsigned char)data[caj->i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[caj->i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (pullcaj_put_key(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (pullcaj_put_key(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (pullcaj_put_key(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_KEYSTRING;
				continue;
			}
			if (pullcaj_put_key(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (pullcaj_put_key(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (pullcaj_put_key(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_KEYSTRING;
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING_UESCAPE)
		{
			unsigned long codepoint;
			if (!isxdigit((unsigned char)data[caj->i]))
			{
				return -EILSEQ;
			}
			caj->uescape[caj->sz++] = (char)data[caj->i];
			if (caj->sz < 4)
			{
				continue;
			}
			caj->uescape[caj->sz] = '\0';
			codepoint = strtoul(caj->uescape, NULL, 16);
			if (codepoint < 0x80)
			{
				if (pullcaj_put_val(caj, (char)codepoint) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (codepoint < 0x800)
			{
				if (pullcaj_put_val(caj, (char)(0xC0 | (codepoint>>6))) != 0)
				{
					return -ENOMEM;
				}
				if (pullcaj_put_val(caj, (char)(0x80 | (codepoint & 0x3F))) != 0)
				{
					return -ENOMEM;
				}
				caj->mode = CAJ_MODE_STRING;
				continue;
			}
			if (pullcaj_put_val(caj, (char)(0xE0 | (codepoint>>12))) != 0)
			{
				return -ENOMEM;
			}
			if (pullcaj_put_val(caj, (char)(0x80 | ((codepoint>>6)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			if (pullcaj_put_val(caj, (char)(0x80 | ((codepoint>>0)&0x3F))) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_STRING;
			continue;
		}
		else if (caj->mode == CAJ_MODE_STRING_ESCAPE)
		{
			int res = 0;
			switch (data[caj->i])
			{
				case 'b':
					res = pullcaj_put_val(caj, '\b');
					break;
				case 'f':
					res = pullcaj_put_val(caj, '\f');
					break;
				case 'n':
					res = pullcaj_put_val(caj, '\n');
					break;
				case 'r':
					res = pullcaj_put_val(caj, '\r');
					break;
				case 't':
					res = pullcaj_put_val(caj, '\t');
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

		if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
		     data[caj->i] == '\t') && (
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
			if (data[caj->i] != ':')
			{
				return -EINVAL;
			}
			caj->mode = CAJ_MODE_VAL;
			continue;
		}
		if (caj->mode == CAJ_MODE_COMMA && data[caj->i] == ',')
		{
			if (data[caj->i] == ',')
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
		if ((caj->mode == CAJ_MODE_COMMA || caj->mode == CAJ_MODE_FIRSTKEY) && data[caj->i] == '}')
		{
			if (data[caj->i] == '}')
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

				if (pullcaj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				ev->ev = CAJ_EV_END_DICT;
				ev->key = pullcaj_get_key(caj);
				ev->keysz = pullcaj_get_keysz(caj);
				caj->state = 2;
				return 1;
state2:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == CAJ_MODE_COMMA || caj->mode == CAJ_MODE_FIRSTVAL) && data[caj->i] == ']')
		{
			if (data[caj->i] == ']')
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

				if (pullcaj_get_keystack(caj) != 0)
				{
					return -ENOMEM;
				}

				ev->ev = CAJ_EV_END_ARRAY;
				ev->key = pullcaj_get_key(caj);
				ev->keysz = pullcaj_get_keysz(caj);
				caj->state = 3;
				return 1;
state3:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->sz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
				continue;
			}
		}
		if ((caj->mode == CAJ_MODE_FIRSTVAL || caj->mode == CAJ_MODE_VAL) && data[caj->i] == '{')
		{
			if (pullcaj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_FIRSTKEY;

			ev->ev = CAJ_EV_START_DICT;
			ev->key = pullcaj_get_key(caj);
			ev->keysz = pullcaj_get_keysz(caj);
			caj->state = 4;
			return 1;
state4:
			pullcaj_put_keystack_2(caj);
			continue;
		}
		if ((caj->mode == CAJ_MODE_FIRSTVAL || caj->mode == CAJ_MODE_VAL) && data[caj->i] == '[')
		{
			if (pullcaj_put_keystack_1(caj) != 0)
			{
				return -ENOMEM;
			}
			caj->mode = CAJ_MODE_FIRSTVAL;

			ev->ev = CAJ_EV_START_ARRAY;
			ev->key = pullcaj_get_key(caj);
			ev->keysz = pullcaj_get_keysz(caj);
			caj->state = 5;
			return 1;
state5:
			pullcaj_put_keystack_2(caj);
			continue;
		}

		if (caj->mode == CAJ_MODE_TRUE)
		{
			if (data[caj->i] != "true"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			ev->ev = CAJ_EV_BOOL;
			ev->key = pullcaj_get_key(caj);
			ev->keysz = pullcaj_get_keysz(caj);
			ev->u.b.b = 1;
			caj->state = 6;
			return 1;
state6:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_FALSE)
		{
			if (data[caj->i] != "false"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 5)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			ev->ev = CAJ_EV_BOOL;
			ev->key = pullcaj_get_key(caj);
			ev->keysz = pullcaj_get_keysz(caj);
			ev->u.b.b = 0;
			caj->state = 7;
			return 1;
state7:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		else if (caj->mode == CAJ_MODE_NULL)
		{
			if (data[caj->i] != "null"[caj->sz++])
			{
				return -EINVAL;
			}
			if (caj->sz < 4)
			{
				continue;
			}
			caj->mode = CAJ_MODE_COMMA;
			ev->ev = CAJ_EV_NULL;
			ev->key = pullcaj_get_key(caj);
			ev->keysz = pullcaj_get_keysz(caj);
			caj->state = 8;
			return 1;
state8:
			if (caj->keystacksz <= 0)
			{
				caj->i++;
				while (caj->i < caj->usz)
				{
					if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
					     data[caj->i] == '\t'))
					{
						caj->i++;
						continue;
					}
					return -EOVERFLOW;
				}
				return 0;
			}
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[caj->i] == 'n')
		{
			caj->mode = CAJ_MODE_NULL;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[caj->i] == 'f')
		{
			caj->mode = CAJ_MODE_FALSE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[caj->i] == 't')
		{
			caj->mode = CAJ_MODE_TRUE;
			caj->sz = 1;
			continue;
		}
		if ((caj->mode == CAJ_MODE_KEY || caj->mode == CAJ_MODE_FIRSTKEY) && data[caj->i] == '"')
		{
			caj->mode = CAJ_MODE_KEYSTRING;
			caj->keysz = 0;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && data[caj->i] == '"')
		{
			caj->mode = CAJ_MODE_STRING;
			caj->valsz = 0;
			continue;
		}
		if ((caj->mode == CAJ_MODE_VAL || caj->mode == CAJ_MODE_FIRSTVAL) && (isdigit((unsigned char)data[caj->i]) || data[caj->i] == '-'))
		{
			caj->mode = CAJ_MODE_NUMBER;
			caj->is_integer = 1;
			streaming_atof_init_strict_json(&caj->streamingatof);
		}
		if (caj->mode == CAJ_MODE_NUMBER)
		{
			size_t tofeed;
			myssize_t szret;
			myssize_t j;
			tofeed = (size_t)(caj->usz - caj->i);
			szret = streaming_atof_feed(&caj->streamingatof, &cdata[caj->i], tofeed);
			if (szret < 0)
			{
				return -EINVAL;
			}
			if (szret > (myssize_t)(caj->usz - caj->i))
			{
				abort();
			}
			for (j = (myssize_t)caj->i; j < (myssize_t)caj->i + szret; j++)
			{
				if (cdata[j] == '.' || cdata[j] == 'e')
				{
					caj->is_integer = 0;
				}
			}
			if (szret < (myssize_t)(caj->usz - caj->i))
			{
				caj->mode = CAJ_MODE_COMMA;
				if (streaming_atof_is_error(&caj->streamingatof))
				{
					return -EINVAL;
				}
				ev->ev = CAJ_EV_NUM;
				ev->key = pullcaj_get_key(caj);
				ev->keysz = pullcaj_get_keysz(caj);
				ev->u.num.d = streaming_atof_end(&caj->streamingatof);
				ev->u.num.is_integer = caj->is_integer;
				caj->state = 9;
				caj->i += (size_t)szret;
				caj->i--;
				return 1;
state9:
				if (caj->keystacksz <= 0)
				{
					caj->i++;
					while (caj->i < caj->usz)
					{
						if ((data[caj->i] == ' ' || data[caj->i] == '\n' || data[caj->i] == '\r' ||
						     data[caj->i] == '\t'))
						{
							caj->i++;
							continue;
						}
						return -EOVERFLOW;
					}
					return 0;
				}
			}
			else
			{
				caj->i += (size_t)szret;
				caj->i--;
			}
			continue;
		}
		return -EINVAL;
	}
	if (caj->mode == CAJ_MODE_NUMBER && caj->eof)
	{
		caj->mode = CAJ_MODE_COMMA;
		ev->ev = CAJ_EV_NUM;
		ev->key = pullcaj_get_key(caj);
		ev->keysz = pullcaj_get_keysz(caj);
		ev->u.num.d = streaming_atof_end(&caj->streamingatof);
		ev->u.num.is_integer = caj->is_integer;
		caj->state = 10;
		return 1;
state10:
		if (caj->keystacksz <= 0)
		{
			return 0;
		}
		caj->state = 0;
		return -EINPROGRESS;
	}
	caj->state = 0;
	return -EINPROGRESS;
}

struct caj_ctx *caj_new(struct caj_handler *handler)
{
	struct caj_ctx *mem = malloc(sizeof(*mem));
	if (mem == NULL)
	{
		return NULL;
	}
	caj_init(mem, handler);
	return mem;
}
void caj_delete(struct caj_ctx *caj)
{
	if (caj == NULL)
	{
		return;
	}
	caj_free(caj);
	free(caj);
}

struct pullcaj_ctx *pullcaj_new(void)
{
	struct pullcaj_ctx *mem = malloc(sizeof(*mem));
	if (mem == NULL)
	{
		return NULL;
	}
	pullcaj_init(mem);
	return mem;
}
void pullcaj_delete(struct pullcaj_ctx *pcaj)
{
	if (pcaj == NULL)
	{
		return;
	}
	pullcaj_free(pcaj);
	free(pcaj);
}
