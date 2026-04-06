#include "cajun.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int datasink(struct caj_out_ctx *ctx, const char *data, size_t sz)
{
	fwrite(data, 1, sz, stdout);
	return 0;
}

int main(int argc, char **argv)
{
	struct caj_out_ctx outctx;
	size_t x;
	char *pathological = malloc(2*1024*1024+1);
	struct cajun_node *n;
	if (pathological == NULL)
	{
		printf("Out of memory\n");
		return 1;
	}
	caj_out_init(&outctx, 0, SIZE_MAX, datasink, NULL);
	for (x = 0; x < 1024*1024; x++)
	{
		pathological[x] = '[';
	}
	for (x = 1024*1024; x < 2*1024*1024; x++)
	{
		pathological[x] = ']';
	}
	pathological[2*1024*1024] = '\0';
	n = cajun_node_parse(pathological, strlen(pathological));
	if (n == NULL)
	{
		printf("Parser error\n");
		return 1;
	}
	cajun_node_out(&outctx, n);
	printf("\n");
	cajun_node_delete(n);
	free(pathological);
	return 0;
}
