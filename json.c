#include "json.h"

// Function Definitions
void object_out( FCGX_Stream *out, json *obj );
void int_array_out( FCGX_Stream *out, long long *arr, int length );
void str_array_out( FCGX_Stream *out, char **arr, int length );
void obj_array_out( FCGX_Stream *out, json **arr, int length  );

void json_out( FCGX_Stream *out, json *r )
{
	if ( r )
	{
		switch( r->type )
		{
			case OBJECT:
				object_out( out, r );
				break;
			case STRING:
				FCGX_FPrintF( out, "\"%s\"", r->str );
				break;
			case INT:
				FCGX_FPrintF( out, "%ld", r->i );
				break;
			case OBJECT_ARRAY:
				obj_array_out( out, r->objArr, r->length );
				break;
			case STRING_ARRAY:
				str_array_out( out, r->strArr, r->length );
				break;
			case INT_ARRAY:
				int_array_out( out, r->intArr, r->length );
				break;
			default:
				FCGX_FPrintF( out, "\"\"" );
		}
	}
	
}

void object_out( FCGX_Stream *out, json *obj )
{
	FCGX_FPrintF( out, "{" );
	
	// go through elements
	for ( int i = 0; i < obj->length; ++i )
	{
		json *el = obj->objArr[i];
		FCGX_FPrintF( out, "\"%s\":", el->name );
		json_out( out, el );
		if ( i + 1 < obj->length )
			FCGX_FPrintF( out, "," );
	}

	FCGX_FPrintF( out, "}" );
}

void int_array_out( FCGX_Stream *out, long long *arr, int length )
{
	FCGX_FPrintF( out, "[" );
	for ( int i = 0; i < length; ++i )
	{
		FCGX_FPrintF( out, "%d", arr[i] );
		if ( i + 1 < length )
			FCGX_FPrintF( out, "," );
			
	}
	FCGX_FPrintF( out, "]" );
}

void str_array_out( FCGX_Stream *out, char **arr, int length )
{
	FCGX_FPrintF( out, "[" );
	for ( int i = 0; i < length; ++i )
	{
		FCGX_FPrintF( out, "%s", arr[i] );
		if ( i + 1 < length )
			FCGX_FPrintF( out, "," );
	}
	FCGX_FPrintF( out, "]" );
}
void obj_array_out( FCGX_Stream *out, json **arr, int length  )
{
	FCGX_FPrintF( out, "[" );
	for ( int i = 0; i < length; ++i )
	{
		object_out( out, arr[i] );
		if ( i + 1 < length )
			FCGX_FPrintF( out, "," );
	}
	FCGX_FPrintF( out, "]" );
}
