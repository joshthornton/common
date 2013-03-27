#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <hiredis.h>
#include <fcgiapp.h>
#include <openssl/sha.h>
#include "common.h"

char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

void inplace_url_decode( char *str ) 
{
	char *actual = str, *lookahead = str;
	while ( *lookahead )
	{
		if ( *lookahead == '%' ) {
			if ( lookahead[1] && lookahead[2] ) {
				*actual++ = from_hex( lookahead[1] ) << 4 | from_hex( lookahead[2] );
				lookahead += 2;
			}
		} else if ( *lookahead == '+' ) {
			*actual++ = ' ';
		} else {
			*actual++ = *lookahead;
		}
		lookahead++;
	}
	*actual = *lookahead; // Copy null char
}

char * url_decode( char *str ) 
{
 	char *pstr = str, *buf = malloc(strlen(pstr) + 1), *pbuf = buf;
	while ( *pstr ) {
		if ( *pstr == '%' ) {
			if ( pstr[1] && pstr[2] ) {
				*pbuf++ = from_hex( pstr[1] ) << 4 | from_hex( pstr[2] );
				pstr += 2;
			}
		} else if ( *pstr == '+' ) { 
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *pstr;
		}
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

void get_hash( char hash[65], char * pass, char salt[9] )
{
		int l = strlen( pass );
		char *combined = malloc( sizeof( char ) * l + 9 );
		memcpy( combined, pass, l );
		memcpy( combined + l, salt, 9 );
		sha256( combined, hash );
		free( combined );
}

void to_lower( char *s )
{
	for ( ; *s; ++s) *s = tolower(*s);
}

void sha256( char *string, char outputBuffer[65] )
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

void generate_session( char session[33] )
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
		"!@#$%^&*()_+";

	int i = 32;
	while ( i-- )
        session[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    session[32] = '\0';
}

void generate_salt( char salt[9] )
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
		"!@#$%^&*()_+";

	int i = 9;
	while ( i-- )
        salt[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    salt[8] = '\0';
}

void header( FCGX_Stream *out, int code, char *session )
{
	switch ( code )
	{
		case 400:
			FCGX_FPrintF( out, "Status: 400 Bad Request\r\n" );
			break;
		case 405:
			FCGX_FPrintF( out, "Status: 405 Method Not Allowed\r\n" );
			break;
		case 401:
			FCGX_FPrintF( out, "Status: 401 Unauthorized\r\n" );
			break;
		case 500:
			FCGX_FPrintF( out, "Status: 500 Internal Server Error\r\n" );
			break;
		case 201:
			FCGX_FPrintF( out, "Status: 201 Created\r\n" );
			break;
		default:
			FCGX_FPrintF( out, "Status: 200 Ok\r\n" );
			break;
	}
	FCGX_FPrintF( out, "Content-Type: application/json\r\n" );
	if ( session )
		FCGX_FPrintF( out, "Set-Cookie: sid=%s; Path=/; Max-Age=%d; Secure; HttpOnly;\r\n", session, SESSION_DURATION );
	FCGX_FPrintF( out, "\r\n" );
}

int check_method_post( FCGX_Stream *out, FCGX_ParamArray envp )
{
	char *method = FCGX_GetParam( "REQUEST_METHOD", envp );
	if ( strcmp( method, "POST" ) ) {
		header( out, 405, NULL );
		return 1;
	}
	return 0;
}

int check_method_get( FCGX_Stream *out, FCGX_ParamArray envp )
{
	char *method = FCGX_GetParam( "REQUEST_METHOD", envp );
	if ( strcmp( method, "GET" ) ) {
		header( out, 405, NULL );
		return 1;
	}
	return 0;
}

ssize_t content_length( FCGX_ParamArray envp )
{
	// Get contentlength
	char *cl = FCGX_GetParam( "CONTENT_LENGTH", envp );
	if ( cl )
		return strtol( cl, NULL, 10 );
	return 0;
}

ssize_t read_stream( char **buf, FCGX_Stream *s, long contentLength )
{
    ssize_t length = 0;
    if ( contentLength > 0 )
    {
        // Allow for null char
        contentLength += 1;

        *buf = malloc( sizeof(char) * contentLength );
        while ( !FCGX_HasSeenEOF( s ) )
        {
            if ( length == contentLength )
            {
                contentLength += contentLength;
                *buf = realloc( *buf, sizeof(char) * contentLength );
            }
            length += FCGX_GetStr( *buf + length, contentLength - length, s );
        }

		// Explicitely null terminate
		(*buf)[length] = '\0';
    }
    return length;
}

ssize_t convert_to_params( param **params, char *s, char *firstSep, char *secondSep, int lim ) 
{
	char *p = s, *n = NULL, *v = NULL, *r1 = NULL, *r2 = NULL;
	ssize_t len = 0;
	ssize_t size = 1;
	(*params) = malloc( sizeof( param ) * size );
	
	// Set initial pointer to string
	p = s;

	// Locate first ampersand
	while ( ( n = strtok_r( p, firstSep, &r1 ) ) && lim-- )
	{

		// Increase result array size if necessary
		if ( len == size )
		{
			size += size;
			(*params) = realloc( *params, sizeof( param ) * size );
		}
		
		// Save param name
		(*params)[len].name = n;

		// Check for value
		strtok_r( n, secondSep, &r2 );
		if ( ( v = strtok_r( NULL, secondSep, &r2 ) ) )
			(*params)[len].value = v;
		else
			(*params)[len].value = r2;

		// Null char should follow value
		if ( strtok_r( NULL, secondSep, &r2 ) )
		{
			free( *params );
			return 0;
		}

		// Update for next loop	
		p = NULL;
		len++;
	}
	
	return len;
}

int init ( void (*process)( redisContext * ) )
{

    redisContext *c; 
    struct timeval timeout = { 1, 500000 };

    c = redisConnectWithTimeout((const char *)"127.0.0.1", 6379, timeout );
    if ( c == NULL || c->err ) { 
        if ( c ) { 
            printf( "Connection error: %s\n", c->errstr );
            redisFree( c );
        } else {
            printf( "Connection error: can't allocate redis context\n" );
        }   
        exit( 1 );
    }   

	process( c );

    // Close redis connection
    redisFree( c );

    return EXIT_SUCCESS;

}
