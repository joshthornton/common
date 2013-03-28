#ifndef _JSON_POST_H_
#define _JSON_POST_H_

#include <fcgiapp.h>

#define OBJECT			(0)
#define STRING			(1)
#define INT				(2)
#define OBJECT_ARRAY	(3)
#define STRING_ARRAY	(4)
#define INT_ARRAY		(5)

typedef struct json {
	char *name;
	union {
		char *str;
		long long i;
		struct json **objArr;
		char **strArr;
		long long *intArr;
	};
	int type;
	int length;
} json;

void json_out( FCGX_Stream *out, json *r );

#endif
