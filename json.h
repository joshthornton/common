#ifndef _COMMON_POST_H_
#define _COMMON_POST_H_

#include <fcgiapp.h>
#include <unistd.h>
#include <hiredis.h>

#define SESSION_DURATION 3600

typedef struct param {
	char *name;
	char *value;
} param;

int init( void (*process)( redisContext *c ) );
ssize_t read_stream( char **buf, FCGX_Stream *s, long contentLength );
ssize_t convert_to_params( param **params, char *s, char *firstSep, char *secondSep, int lim ); 
ssize_t content_length( FCGX_ParamArray envp );
void header( FCGX_Stream *out, int code, char *session );
void generate_salt( char salt[9] );
void sha256( char *string, char outputBuffer[65] );
void to_lower( char *s );
void generate_session( char session[33] );
void get_hash( char hash[65], char * pass, char salt[9] );
int check_method_post( FCGX_Stream *out, FCGX_ParamArray envp );
int check_method_get( FCGX_Stream *out, FCGX_ParamArray envp );
char * url_decode( char * str );
void inplace_url_decode( char *str ); 

#endif
