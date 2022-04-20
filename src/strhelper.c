// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2018-2020 Joel E. Anderson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <string.h>
#include "private/memory.h"
#include "private/strhelper.h"

char *
copy_cstring( const char *str ) {
  char *new_string;
  size_t length;

  length = strlen( str );
  new_string = alloc_mem( length + 1 );
  if( !new_string ) {
    return NULL;
  }

  memcpy( new_string, str, length );
  new_string[length] = '\0';

  return new_string;
}

wchar_t *
copy_cstring_w( const wchar_t *str ) {
  wchar_t *new_string;
  size_t length;
  size_t str_size;

  length = wcslen( str );
  str_size = ( length + 1 ) * sizeof( wchar_t );
  new_string = alloc_mem( str_size );
  if( !new_string ) {
    return NULL;
  }

  memcpy( new_string, str, str_size );
  new_string[length] = '\0';

  return new_string;
}

char *
copy_cstring_with_length( const char *str, size_t *length ) {
  char *new_string;

  *length = strlen( str );

  new_string = alloc_mem( *length + 1 );
  if( !new_string ) {
    return NULL;
  }

  memcpy( new_string, str, *length );
  new_string[*length] = '\0';

  return new_string;
}

