/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once


/* used as type with tpalloc */
#define CASUAL_STRING "CSTRING"

#define CASUAL_STRING_SUCCESS 0
#define CASUAL_STRING_INVALID_HANDLE 1
#define CASUAL_STRING_INVALID_ARGUMENT 2
#define CASUAL_STRING_OUT_OF_MEMORY 4
#define CASUAL_STRING_OUT_OF_BOUNDS 8
#define CASUAL_STRING_INTERNAL_FAILURE 128


#ifdef __cplusplus
extern "C" {
#endif

const char* casual_string_description( int code);

int casual_string_explore_buffer( const char* buffer, long* size, long* used);

int casual_string_set( char** buffer, const char* value);
int casual_string_get( const char* buffer, const char** value);


#ifdef __cplusplus
}
#endif


