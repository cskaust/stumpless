// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2018-2022 Joel E. Anderson
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

/* this must be included first to avoid errors */
#include "private/windows_wrapper.h"

#include <ktmw32.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stumpless/config/wel_supported.h>
#include <stumpless/entry.h>
#include <stumpless/error.h>
#include <stumpless/param.h>
#include <stumpless/severity.h>
#include <stumpless/target.h>
#include <stumpless/target/wel.h>
#include "private/config/locale/wrapper.h"
#include "private/config/wel_supported.h"
#include "private/config/wrapper.h"
#include "private/config/wrapper/thread_safety.h"
#include "private/error.h"
#include "private/facility.h"
#include "private/inthelper.h"
#include "private/memory.h"
#include "private/severity.h"
#include "private/validate.h"

/** The base subkey used for event sources, including a trailing backslash. */
static LPCWSTR base_source_subkey = L"SYSTEM\\CurrentControlSet\\Services\\" \
                                      L"EventLog\\";

/** The size of the base subkey in bytes, including the NULL terminator. */
DWORD base_source_subkey_size = 88;

/** The full base subkey used for the default source installation. */
static LPCWSTR default_source_subkey = L"SYSTEM\\CurrentControlSet\\" \
                                         L"Services\\EventLog\\Stumpless";

/**
 * Returns a Windows error code for the current Stumpless error. This may not
 * be equivalent to the result of GetLastError, for example if a memory
 * allocation failure occurred.
 *
 * @return A Windows error code reflecting the last error, or ERROR_SUCCESS
 * if there is not one.
 */
static
DWORD
get_windows_error_code( void ) {
  const struct stumpless_error *error;
  enum stumpless_error_id id;

  error = stumpless_get_error(  );
  if( !error ) {
    return ERROR_SUCCESS;
  }

  id = stumpless_get_error_id( error );
  if( id == STUMPLESS_MEMORY_ALLOCATION_FAILURE ) {
    return ERROR_NOT_ENOUGH_MEMORY;
  } else {
    return GetLastError(  );
  }
}

/**
 * Creates a copy of a NULL terminated multibyte string in wide string format.
 *
 * @param str A multibyte string to copy, in UTF-8 format.
 *
 * @param copy_length The length of the copy including the NULL terminator, in
 * characters. If this is NULL or the function fails, then it is ignored.
 *
 * @return A copy of the given string in wide string format, or NULL if an
 * error is encountered.
 */
static
LPCWSTR
copy_cstring_to_lpcwstr( LPCSTR str, int *copy_length ) {
  int needed_wchar_length;
  LPWSTR str_copy;
  int conversion_result;

  needed_wchar_length = MultiByteToWideChar( CP_UTF8,
                                             MB_ERR_INVALID_CHARS |
                                               MB_PRECOMPOSED,
                                             str,
                                             -1,
                                             NULL,
                                             0 );

  if( needed_wchar_length == 0 ) {
    raise_mb_conversion_failure( GetLastError(  ) );
    return NULL;
  }

  str_copy = alloc_mem( needed_wchar_length * sizeof( WCHAR ) );
  if( !str_copy ) {
    return NULL;
  }

  conversion_result = MultiByteToWideChar( CP_UTF8,
                                           MB_ERR_INVALID_CHARS |
                                             MB_PRECOMPOSED,
                                           str,
                                           -1,
                                           str_copy,
                                           needed_wchar_length );

  if( conversion_result == 0 ) {
    free_mem( str_copy );
    raise_mb_conversion_failure( GetLastError(  ) );
    return NULL;
  }

  if( copy_length ) {
    *copy_length = conversion_result;
  }

  return str_copy;
}

/**
 * Creates a copy of a NULL terminated wide character string.
 *
 * @param str A wide character string to copy.
 *
 * @return A copy of the given wide character string, or NULL if an error is
 * encountered.
 */
static
LPWSTR
copy_lpcwstr( LPCWSTR str ) {
  size_t str_len;
  size_t str_size;
  LPWSTR str_copy;

  str_len = wcslen( str );
  str_size = ( str_len + 1 ) * sizeof( WCHAR );
  str_copy = alloc_mem( str_size );
  if( !str_copy ) {
    return NULL;
  }

  memcpy( str_copy, str, str_size );
  return str_copy;
}

/**
 * Creates the values for an event source in the provided subkey.
 *
 * @param subkey A handle to the subkey to populate.
 *
 * @param category_count The number of categories included in the category file.
 *
 * @param category_file A path to the resource file containing the categories
 * used for this source, as a wide string. If NULL, then the CategoryMessageFile
 * registry value will not be created.
 *
 * @param category_file_size The size of category_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param event_file A path to the resource file containing the messages used
 * for this source, as a wide string. If NULL, then the EventMessageFile
 * registry value will not be created.
 *
 * @param event_file_size The size of event_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param parameter_file A path to the resource file containing the messages
 * used as parameters for this source, as a wide string. If NULL, then the
 * ParameterMessageFile registry value will not be created.
 *
 * @param parameter_file_size The size of parameter_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param types_supported The flags used for the TypesSupported value.
 *
 * @return ERROR_SUCCESS if the operation was successful, or the result of
 * GetLastError if an error was encountered.
 */
static
DWORD
populate_event_source_subkey( HKEY subkey,
                              DWORD category_count,
                              LPCWSTR category_file,
                              DWORD category_file_size,
                              LPCWSTR event_file,
                              DWORD event_file_size,
                              LPCWSTR parameter_file,
                              DWORD parameter_file_size,
                              DWORD types_supported ) {
  DWORD result;

  result = RegSetValueExW( subkey,
                           L"CategoryCount",
                           0,
                           REG_DWORD,
                           ( const BYTE * ) &category_count,
                           sizeof( category_count ) );
  if( result != ERROR_SUCCESS ) {
    raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    return result;
  }

  if( category_file ) {
    result = RegSetValueExW( subkey,
                             L"CategoryMessageFile",
                             0,
                             REG_SZ,
                             ( const BYTE * ) category_file,
                             category_file_size );
    if( result != ERROR_SUCCESS ) {
      raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                             result,
                             L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
      return result;
    }
  }

  if( event_file ) {
    result = RegSetValueExW( subkey,
                             L"EventMessageFile",
                             0,
                             REG_SZ,
                             ( const BYTE * ) event_file,
                             event_file_size );
    if( result != ERROR_SUCCESS ) {
      raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                             result,
                             L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
      return result;
    }
  }

  if( parameter_file ) {
    result = RegSetValueExW( subkey,
                             L"ParameterMessageFile",
                             0,
                             REG_SZ,
                             ( const BYTE * ) parameter_file,
                             parameter_file_size );
    if( result != ERROR_SUCCESS ) {
      raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                             result,
                             L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
      return result;
    }
  }

  result = RegSetValueExW( subkey,
                           L"TypesSupported",
                           0,
                           REG_DWORD,
                           ( const BYTE * ) &types_supported,
                           sizeof( types_supported ) );
  if( result != ERROR_SUCCESS ) {
    raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    return result;
  }

  return ERROR_SUCCESS;
}

/**
 * Creates the given event source registry subkey, under the assumption that
 * it does not currently exist. The source will be put in the registry under
 * SYSTEM\CurrentControlSet\Services\EventLog.
 *
 * @param subkey_name The complete name of the registry subkey that the source
  * should be created under, as a NULL terminated wide character string.
 *
 * @param source_name The event source to create. This string must be
 * terminated by _two_ NULL characters, to allow its use in a REG_MULTI_SZ
 * registry value.
 *
 * @param source_name_size The size of source_name in bytes, including the two
 * terminating NULL characters. This must not be greater than 512, which is the
 * maximum registry key name size including a NULL terminator. Must be greater
 * than 2.
 *
 * @param category_count The number of categories present in the message file.
 * This is used for the CategoryCount registry value.
 *
 * @param category_file A path to the resource file containing the categories
 * used for this source, as a wide string. If NULL, then the CategoryMessageFile
 * registry value will not be created.
 *
 * @param category_file_size The size of category_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param event_file A path to the resource file containing the messages used
 * for this source, as a wide string. If NULL, then the EventMessageFile
 * registry value will not be created.
 *
 * @param event_file_size The size of event_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param parameter_file A path to the resource file containing the messages
 * used as parameters for this source, as a wide string. If NULL, then the
 * ParameterMessageFile registry value will not be created.
 *
 * @param parameter_file_size The size of parameter_file path string in bytes,
 * including the terminating NULL character.
 *
 * @param types_supported A set of flags designating the event types that are
 * supported by this source. This is used for the TypesSupported registry value.
 *
 * @return ERROR_SUCCESS if the operation was successful, or the result of
 * GetLastError if an error was encountered.
 */
static
DWORD
create_event_source_subkey( LPCWSTR subkey_name,
                            LPCWSTR source_name,
                            DWORD source_name_size,
                            DWORD category_count,
                            LPCWSTR category_file,
                            DWORD category_file_size,
                            LPCWSTR event_file,
                            DWORD event_file_size,
                            LPCWSTR parameter_file,
                            DWORD parameter_file_size,
                            DWORD types_supported ) {
  HANDLE trans;
  DWORD result  = ERROR_SUCCESS;
  LSTATUS reg_result;
  HKEY subkey_handle;
  HKEY source_key_handle;
  BOOL bool_result;

  trans = CreateTransaction( NULL,
                           0,
                           0,
                           0,
                           0,
                           0,
                           L"Stumpless registration of Event Source" ); // TODO need to localize
  if( trans == INVALID_HANDLE_VALUE ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_CREATE_TRANSACTION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
    return result;
  }

  reg_result = RegCreateKeyTransactedW( HKEY_LOCAL_MACHINE,
                                        subkey_name,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                                        NULL,
                                        &subkey_handle,
                                        NULL,
                                        trans,
                                        NULL );
  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_SUBKEY_CREATION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    goto cleanup_transaction;
  }

  reg_result = RegSetValueExW( subkey_handle,
                               L"Sources",
                               0,
                               REG_MULTI_SZ,
                               ( const BYTE * ) source_name,
                               source_name_size );
  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    goto cleanup_key;
  }

  reg_result = RegCreateKeyTransactedW( subkey_handle,
                                        source_name,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_SET_VALUE,
                                        NULL,
                                        &source_key_handle,
                                        NULL,
                                        trans,
                                        NULL );
  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_SUBKEY_CREATION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    goto cleanup_key;
  }

  result = populate_event_source_subkey( source_key_handle,
                                         category_count,
                                         category_file,
                                         category_file_size,
                                         event_file,
                                         event_file_size,
                                         parameter_file,
                                         parameter_file_size,
                                         types_supported );
  if( result != ERROR_SUCCESS ) {
    goto cleanup_source;
  }

  bool_result = CommitTransaction( trans );
  if( bool_result == 0 ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_COMMIT_TRANSACTION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
  }

cleanup_source:
  RegCloseKey( source_key_handle );
cleanup_key:
  RegCloseKey( subkey_handle );
cleanup_transaction:
  CloseHandle( trans );
  return result;
}

/**
 * Detects if a given string is in a MULTI_SZ registry value.
 *
 * @param val The MULTI_SZ registry key value.
 *
 * @param str The string to search for in the entry.
 *
 * @return true if the string is present, false otherwise.
 */
static
bool
multi_sz_contains( const WCHAR *value, LPCWSTR str ){
  LPCWSTR current = value;
  size_t str_size;

  str_size = wcslen( str ) + 1;

  while( *current != L'\0' ) {
    if( wcsncmp(current, str, str_size ) == 0 ) {
      return true;
    }

    current += wcslen( current ) + 1;
  }

  return false;
}

/**
 * Sets the insertion string at the given index to the provided wide string,
 * freeing the previous one if it existed.
 *
 * @param entry The entry to set the insertion string of. Must not be NULL.
 *
 * @param index The index of the insertion string.
 *
 * @param str The wide string to use as the insertion string. Must not be NULL.
 *
 * @return The modified entry, or NULL if an error is encountered.
 */
static
struct stumpless_entry *
swap_wel_insertion_string( struct stumpless_entry *entry,
                           WORD index,
                           LPCWSTR str ) {
  struct wel_data *data;
  struct stumpless_entry *result;

  data = entry->wel_data;

  lock_wel_data( data );
  result = unsafe_swap_wel_insertion_string( entry, index, str );
  unlock_wel_data( data );

  return result;
}

/**
* Sets the insertion string at the given index to the provided UTF-8 string.
*
* A copy of the string is created in wide character format.
*
* If the index is higher than the current max, then the insertion string arrays
* are expanded to accomodate it.
*
* @param entry The entry to set the insertion string of. Must not be NULL.
*
* @param index The index of the insertion string.
*
* @param str The UTF-8 string to use as the insertion string. Must not be NULL.
*
* @return The modified entry, or NULL if an error is encountered.
*/
static
struct stumpless_entry *
set_wel_insertion_string( struct stumpless_entry *entry,
                          WORD index,
                          LPCSTR str ) {
  LPCWSTR str_copy;

  str_copy = copy_cstring_to_lpcwstr( str, NULL );
  if( !str_copy ) {
    return NULL;
  }

  return swap_wel_insertion_string( entry, index, str_copy );
}

/**
* Sets the insertion string at the given index to the provided wide string.
*
* A copy of the string is created in wide character format.
*
* If the index is higher than the current max, then the insertion string arrays
* are expanded to accomodate it.
*
* @param entry The entry to set the insertion string of. Must not be NULL.
*
* @param index The index of the insertion string.
*
* @param str The wide string to use as the insertion string. Must not be NULL.
*
* @return The modified entry, or NULL if an error is encountered.
*/
static
struct stumpless_entry *
set_wel_insertion_string_w( struct stumpless_entry *entry,
                            WORD index,
                            LPCWSTR str ) {
  LPCWSTR str_copy;

  str_copy = copy_lpcwstr( str );
  if( !str_copy ) {
    return NULL;
  }

  return swap_wel_insertion_string( entry, index, str_copy );
}

/**
 * Creates the registry entries to install an event source with the given
 * specifications.
 *
 * @param subkey_name The name of the subkey that the source should be added to,
 * as a wide char string. This subkey will be created under
 * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog.
 *
 * @param source_name The name of the event source, as a double-NULL terminated
 * wide char string. This will be added to the "Sources" value of the subkey,
 * and created as a subkey under it as well. An error is raised if this is NULL.
 * If the value "Sources" already exists and is not a properly formatted
 * MULTI_SZ value, then this function fails and returns ERROR_INVALID_PARAMETER.
 *
 * @param category_count The number of categories present in the message file.
 * This is used for the CategoryCount registry value.
 *
 * @param category_file A path to the resource file containing the categories
 * used for this source, as a wide char string. If NULL, then the
 * CategoryMessageFile registry value will not be created.
 *
 * @param event_file A path to the resource file containing the messages used
 * for this source, as a wide char string. If NULL, then the
 * EventMessageFile registry value will not be created.
 *
 * @param parameter_file A path to the resource file containing the messages
 * used as parameters for this source, as a wide char string. If
 * NULL, then the ParameterMessageFile registry value will not be created.
 *
 * @param types_supported A set of flags designating the event types that are
 * supported by this source. This is used for the TypesSupported registry value.
 *
 * @return ERROR_SUCCESS if the operation was successful, or a Windows error
 * code result if an error was encountered. Note that the error code may not
 * necessarily correspond to a call to GetLastError after this, for example in
 * the case where a registry value was not correctly formed.
 */
static
DWORD
add_event_source( LPCWSTR subkey_name,
                  DWORD subkey_name_size,
                  LPCWSTR source_name,
                  DWORD source_name_size,
                  DWORD category_count,
                  LPCWSTR category_file,
                  DWORD category_file_size,
                  LPCWSTR event_file,
                  DWORD event_file_size,
                  LPCWSTR parameter_file,
                  DWORD parameter_file_size,
                  DWORD types_supported ) {
  LPWSTR complete_subkey;
  HKEY subkey_handle;
  LSTATUS reg_result;
  DWORD result = ERROR_SUCCESS;
  DWORD value_type;
  WCHAR sources_value_buffer[256];
  DWORD value_size = sizeof( sources_value_buffer );
  LPWSTR sources_value = sources_value_buffer;
  LPWSTR new_sources_value;
  DWORD new_sources_size;
  HKEY source_subkey_handle;
  HANDLE trans;
  BOOL bool_result;

  // build the complete source subkey
  complete_subkey = alloc_mem( base_source_subkey_size + subkey_name_size - sizeof( WCHAR ) );
  if( !complete_subkey ) {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup;
  }
  memcpy( complete_subkey, base_source_subkey, base_source_subkey_size - sizeof( WCHAR ) );
  memcpy( ( ( char * ) complete_subkey ) + base_source_subkey_size - sizeof( WCHAR ), subkey_name, subkey_name_size );

  // before the modification transaction starts, we open the main key to see if it exists
  reg_result = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                              complete_subkey,
                              0,
                              KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                              &subkey_handle );
  if( reg_result != ERROR_SUCCESS ) {
    if( reg_result == ERROR_FILE_NOT_FOUND ) {
      // if the key doesn't exist at all, we can simply create it
      return create_event_source_subkey( complete_subkey,
                                         source_name,
                                         source_name_size,
                                         category_count,
                                         category_file,
                                         category_file_size,
                                         event_file,
                                         event_file_size,
                                         parameter_file,
                                         parameter_file_size,
                                         types_supported );
    } else {
      result = reg_result;
      raise_windows_failure( L10N_REGISTRY_SUBKEY_OPEN_FAILED_ERROR_MESSAGE,
                             result,
                             L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
      goto cleanup_complete_subkey;
    }
  }

  // if the key already exists, we need to check the sources list to see if the
  // source name already exists
  reg_result = RegGetValueW( subkey_handle,
                             NULL,
                             L"Sources",
                             RRF_RT_REG_MULTI_SZ,
                             &value_type,
                             sources_value,
                             &value_size );

  if( reg_result == ERROR_MORE_DATA ) {
    // allocate a new buffer and call again
    sources_value = alloc_mem( value_size );
    if( !sources_value ) {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup_subkey;
    }

    reg_result = RegGetValueW( subkey_handle,
                               NULL,
                               L"Sources",
                               RRF_RT_REG_MULTI_SZ,
                               &value_type,
                               sources_value,
                               &value_size );
  }

  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_VALUE_GET_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    goto cleanup_sources;
  }

  if( sources_value[0] != L'\0' &&
      !( sources_value[(value_size / sizeof( WCHAR) ) - 2] == L'\0' &&
         sources_value[(value_size / sizeof( WCHAR) ) - 1] == L'\0' ) ) {
    raise_invalid_encoding( "the Sources MULTI_SZ registry value was neither empty nor terminated with two NULL characters" ); // TODO need to localize
    result = ERROR_INVALID_PARAMETER;
    goto cleanup_sources;
  }

  if( !multi_sz_contains( sources_value, source_name ) ) {
    new_sources_size = value_size + source_name_size - 1;

    new_sources_value = alloc_mem( new_sources_size );
    if( !new_sources_value ) {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup_sources;
    }

    memcpy( new_sources_value, sources_value, value_size - 2 );
    memcpy( ( ( char * ) new_sources_value ) + value_size - 2,
            source_name,
            source_name_size );

    reg_result = RegSetValueExW( subkey_handle,
                                 L"Sources",
                                 0,
                                 REG_MULTI_SZ,
                                 ( const BYTE * ) new_sources_value,
                                 new_sources_size );

    free_mem( new_sources_value ); // TODO candidate to move to alloca

    if( reg_result != ERROR_SUCCESS ) {
      result = reg_result;
      raise_windows_failure( L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE,
                             result,
                             L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
      goto cleanup_sources;
    }
  }

  trans = CreateTransaction( NULL,
                           0,
                           0,
                           0,
                           0,
                           0,
                           L"stumpless_add_wel_event_source registration of Event Source" ); // TODO need to localize
  if( trans == INVALID_HANDLE_VALUE ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_CREATE_TRANSACTION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
    return result;
  }

  reg_result = RegCreateKeyTransactedW( subkey_handle,
                                        source_name,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_SET_VALUE,
                                        NULL,
                                        &source_subkey_handle,
                                        NULL,
                                        trans,
                                        NULL );
  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_SUBKEY_CREATION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
    goto cleanup_trans;
  }

  result = populate_event_source_subkey( source_subkey_handle,
                                         category_count,
                                         category_file,
                                         category_file_size,
                                         event_file,
                                         event_file_size,
                                         parameter_file,
                                         parameter_file_size,
                                         types_supported );
  if( result != ERROR_SUCCESS ) {
    goto cleanup_source_subkey;
  }

  bool_result = CommitTransaction( trans );
  if( bool_result == 0 ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_COMMIT_TRANSACTION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
  }

cleanup_source_subkey:
  RegCloseKey( source_subkey_handle );
cleanup_trans:
  CloseHandle( trans );
cleanup_sources:
  if( sources_value != sources_value_buffer ) {
    free_mem( sources_value );
  }
cleanup_subkey:
  RegCloseKey( subkey_handle );
cleanup_complete_subkey:
  free_mem( complete_subkey );
cleanup:
  return result;
}

/* public definitions */

DWORD
stumpless_add_default_wel_event_source( void ) {
  HMODULE this_module;
  WCHAR library_path[MAX_PATH];
  DWORD library_path_size;
  DWORD result = ERROR_SUCCESS;
  LPCWSTR source_name = L"Stumpless\0";
  DWORD source_name_size = 22; // includes both NULL terminators
  BOOL bool_result;

  clear_error(  );

  // get the path to this library
  bool_result = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    ( LPCWSTR ) &stumpless_add_default_wel_event_source,
                                    &this_module ) ;
  if( bool_result == 0 ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_GETMODULEHANDLEXW_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
    return result;
  }

  // TODO need to handle long path names at some point (of form `//?/`)
  library_path_size = GetModuleFileNameW( this_module, library_path, MAX_PATH );
  if( library_path_size == 0 ) {
    result = GetLastError(  );
    raise_windows_failure( L10N_GETMODULEFILENAMEW_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_GETLASTERROR_ERROR_CODE_TYPE );
    goto cleanup_module;
  }

  if( library_path_size != MAX_PATH ) {
    library_path_size += 1;
  }
  library_path_size *= sizeof( WCHAR );

  result = add_event_source( source_name,
                             source_name_size,
                             source_name,
                             source_name_size,
                             8,
                             library_path,
                             library_path_size,
                             library_path,
                             library_path_size,
                             NULL,
                             0,
                             EVENTLOG_AUDIT_FAILURE |
                               EVENTLOG_AUDIT_SUCCESS |
                               EVENTLOG_ERROR_TYPE |
                               EVENTLOG_INFORMATION_TYPE |
                               EVENTLOG_WARNING_TYPE );

cleanup_module:
  FreeLibrary( this_module );
  return result;
}

DWORD
stumpless_add_wel_event_source( LPCSTR subkey_name,
                                LPCSTR source_name,
                                DWORD category_count,
                                LPCSTR category_file,
                                LPCSTR event_file,
                                LPCSTR parameter_file,
                                DWORD types_supported ) {
  DWORD result = ERROR_SUCCESS;
  DWORD subkey_name_length = 0;
  LPCWSTR subkey_name_w;
  DWORD source_name_length = 0;
  LPWSTR source_name_w;
  DWORD conversion_result;
  DWORD category_file_length = 0;
  LPCWSTR category_file_w = NULL;
  DWORD event_file_length = 0;
  LPCWSTR event_file_w = NULL;
  DWORD parameter_file_length = 0;
  LPCWSTR parameter_file_w = NULL;

  VALIDATE_ARG_NOT_NULL_WINDOWS_RETURN( subkey_name );
  VALIDATE_ARG_NOT_NULL_WINDOWS_RETURN( source_name );

  subkey_name_w = copy_cstring_to_lpcwstr( subkey_name, &subkey_name_length );
  if( !subkey_name_w ) {
    result = get_windows_error_code(  );
    goto finish;
  }

  source_name_length = MultiByteToWideChar( CP_UTF8,
                                             MB_ERR_INVALID_CHARS |
                                               MB_PRECOMPOSED,
                                             source_name,
                                             -1,
                                             NULL,
                                             0 );

  if( source_name_length == 0 ) {
    raise_mb_conversion_failure( GetLastError(  ) );
   goto cleanup_subkey;
  }

  source_name_length++; // make room for the double NULL termination
  source_name_w = alloc_mem( source_name_length * sizeof( WCHAR ) );
  if( !source_name_w ) {
    result = ERROR_NOT_ENOUGH_MEMORY;
    goto cleanup_subkey;
  }

  conversion_result = MultiByteToWideChar( CP_UTF8,
                                           MB_ERR_INVALID_CHARS |
                                             MB_PRECOMPOSED,
                                           source_name,
                                           -1,
                                           source_name_w,
                                           source_name_length );

  if( conversion_result == 0 ) {
    raise_mb_conversion_failure( GetLastError(  ) );
    goto cleanup_source;
  }
  source_name_w[source_name_length-1] = L'\0';

  if( category_file ) {
    category_file_w = copy_cstring_to_lpcwstr( category_file,
                                               &category_file_length );
    if( !category_file_w ) {
      result = get_windows_error_code(  );
      goto cleanup_source;
    }
  }

  if( event_file ) {
    event_file_w = copy_cstring_to_lpcwstr( event_file,
                                            &event_file_length );
    if( !event_file_w ) {
      result = get_windows_error_code(  );
      goto cleanup_category;
    }
  }

  if( parameter_file ) {
    parameter_file_w = copy_cstring_to_lpcwstr( parameter_file,
                                                &parameter_file_length );
    if( !parameter_file_w ) {
      result = get_windows_error_code(  );
      goto cleanup_event;
    }
  }

  result = add_event_source( subkey_name_w,
                             subkey_name_length * sizeof( WCHAR ),
                             source_name_w,
                             source_name_length * sizeof( WCHAR ),
                             category_count,
                             category_file_w,
                             category_file_length * sizeof( WCHAR ),
                             event_file_w,
                             event_file_length * sizeof( WCHAR ),
                             parameter_file_w,
                             parameter_file_length * sizeof( WCHAR ),
                             types_supported );

  free_mem( parameter_file_w );
cleanup_event:
  free_mem( event_file_w );
cleanup_category:
  free_mem( category_file_w );
cleanup_source:
  free_mem( source_name_w );
cleanup_subkey:
  free_mem( subkey_name_w );
finish:
  return result;
}

WORD
stumpless_get_wel_category( const struct stumpless_entry *entry ) {
  const struct wel_data *data;
  int prival;
  WORD category;

  VALIDATE_ARG_NOT_NULL_UNSIGNED_RETURN( entry );

  data = entry->wel_data;

  lock_wel_data( data );
  if( data->category_set ) {
    category = data->category;
  } else {
    prival = stumpless_get_entry_prival( entry );
    category = get_category( prival );
  }
  unlock_wel_data( data );

  return category;
}

DWORD
stumpless_get_wel_event_id( const struct stumpless_entry *entry ) {
  const struct wel_data *data;
  int prival;
  DWORD event_id;

  VALIDATE_ARG_NOT_NULL_UNSIGNED_RETURN( entry );

  data = entry->wel_data;

  lock_wel_data( data );
  if( data->event_id_set ) {
    event_id = data->event_id;
  } else {
    prival = stumpless_get_entry_prival( entry );
    event_id = get_event_id( prival );
  }
  unlock_wel_data( data );

  return event_id;
}

struct stumpless_param *
stumpless_get_wel_insertion_param( const struct stumpless_entry *entry,
                                   WORD index ) {
  const struct wel_data *data;
  struct stumpless_param *param = NULL;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  if( index >= data->insertion_count ) {
    raise_index_out_of_bounds(
       L10N_INVALID_INDEX_ERROR_MESSAGE( "insertion string" ),
       index
    );
    goto cleanup_and_return;
  }

  clear_error(  );
  param = ( struct stumpless_param * ) data->insertion_params[index];

cleanup_and_return:
  unlock_wel_data( data );
  return param;
}

LPCSTR
stumpless_get_wel_insertion_string( const struct stumpless_entry *entry,
                                    WORD index ) {
  struct wel_data *data;
  const struct stumpless_param *param;
  char *str_copy = NULL;
  int needed_mb_length;
  int conversion_result;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  if( index >= data->insertion_count ) {
    raise_index_out_of_bounds(
       L10N_INVALID_INDEX_ERROR_MESSAGE( "insertion string" ),
       index
    );
    goto cleanup_and_return;
  }

  clear_error(  );
  param = data->insertion_params[index];
  if( param ) {
    str_copy = alloc_mem( param->value_length + 1 );
    if( !str_copy ) {
      goto cleanup_and_return;
    }
    memcpy( str_copy, param->value, param->value_length );
    str_copy[param->value_length] = '\0';

  } else if( data->insertion_strings[index] ) {
    needed_mb_length = WideCharToMultiByte( CP_UTF8,
                                            WC_ERR_INVALID_CHARS |
                                              WC_NO_BEST_FIT_CHARS,
                                            data->insertion_strings[index],
                                            -1,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL );
    if( needed_mb_length == 0 ) {
      raise_wide_conversion_failure( GetLastError(  ) );
      goto cleanup_and_return;
    }

    str_copy = alloc_mem( needed_mb_length  );
    if( !str_copy ) {
      goto cleanup_and_return;
    }

    conversion_result = WideCharToMultiByte( CP_UTF8,
                                             WC_ERR_INVALID_CHARS |
                                               WC_NO_BEST_FIT_CHARS,
                                             data->insertion_strings[index],
                                             -1,
                                             str_copy,
                                             needed_mb_length,
                                             NULL,
                                             NULL );
    if( conversion_result == 0 ) {
      free_mem( str_copy );
      str_copy = NULL;
      raise_wide_conversion_failure( GetLastError(  ) );
      goto cleanup_and_return;
    }
  }

cleanup_and_return:
  unlock_wel_data( data );
  return str_copy;
}

LPCWSTR
stumpless_get_wel_insertion_string_w( const struct stumpless_entry *entry,
                                      WORD index ) {
  struct wel_data *data;
  const struct stumpless_param *param;
  LPCWSTR str_copy = NULL;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  if( index >= data->insertion_count ) {
    raise_index_out_of_bounds(
       L10N_INVALID_INDEX_ERROR_MESSAGE( "insertion string" ),
       index
    );
    goto cleanup_and_return;
  }

  clear_error(  );
  param = data->insertion_params[index];
  if( param ) {
    str_copy = copy_param_value_to_lpwstr( param );

  } else if( data->insertion_strings[index] ) {
    str_copy = copy_lpcwstr( data->insertion_strings[index] );
  }

cleanup_and_return:
  unlock_wel_data( data );
  return str_copy;
}

WORD
stumpless_get_wel_type( const struct stumpless_entry *entry ) {
  const struct wel_data *data;
  int prival;
  WORD type;

  VALIDATE_ARG_NOT_NULL_UNSIGNED_RETURN( entry );

  data = entry->wel_data;

  lock_wel_data( data );
  if( data->type_set ) {
    type = data->type;
  } else {
    prival = stumpless_get_entry_prival( entry );
    type = get_type( prival );
  }
  unlock_wel_data( data );

  return type;
}

DWORD
stumpless_remove_default_wel_event_source( void ) {
  DWORD result = ERROR_SUCCESS;
  LSTATUS reg_result;

  reg_result = RegDeleteTreeW( HKEY_LOCAL_MACHINE, default_source_subkey );
  if( reg_result != ERROR_SUCCESS ) {
    result = reg_result;
    raise_windows_failure( L10N_REGISTRY_SUBKEY_DELETION_FAILED_ERROR_MESSAGE,
                           result,
                           L10N_WINDOWS_RETURN_ERROR_CODE_TYPE );
  }

  return result;
}

struct stumpless_entry *
stumpless_set_wel_category( struct stumpless_entry *entry, WORD category ) {
  struct wel_data *data;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  data->category = category;
  data->category_set = TRUE;
  unlock_wel_data( data );

  clear_error();
  return entry;
}

struct stumpless_entry *
stumpless_set_wel_event_id( struct stumpless_entry *entry, DWORD event_id ) {
  struct wel_data *data;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  data->event_id = event_id;
  data->event_id_set = TRUE;
  unlock_wel_data( data );

  clear_error();
  return entry;
}

struct stumpless_entry *
stumpless_set_wel_insertion_param( struct stumpless_entry *entry,
                                   WORD index,
                                   const struct stumpless_param *param ) {
  struct wel_data *data;
  LPCWSTR old_str;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  if( index >= data->insertion_count ) {
    if( !resize_insertion_params( entry, index ) ) {
      unlock_wel_data( data );
      return NULL;
    }
  }

  clear_error();

  data->insertion_params[index] = param;
  old_str = data->insertion_strings[index];
  data->insertion_strings[index] = NULL;
  unlock_wel_data( data );

  free_mem( old_str );
  return entry;
}

struct stumpless_entry *
stumpless_set_wel_insertion_string( struct stumpless_entry *entry,
                                    WORD index,
                                    LPCSTR str ) {
  VALIDATE_ARG_NOT_NULL( entry );
  VALIDATE_ARG_NOT_NULL( str );

  clear_error(  );
  return set_wel_insertion_string( entry, index, str );
}

struct stumpless_entry *
stumpless_set_wel_insertion_string_w( struct stumpless_entry *entry,
                                      WORD index,
                                      LPCWSTR str ) {
  VALIDATE_ARG_NOT_NULL( entry );
  VALIDATE_ARG_NOT_NULL( str );

  clear_error(  );
  return set_wel_insertion_string_w( entry, index, str );
}

struct stumpless_entry *
stumpless_set_wel_insertion_strings( struct stumpless_entry *entry,
                                     WORD count,
                                     ... ) {
  va_list insertions;
  struct stumpless_entry *result;

  va_start( insertions, count );
  result = vstumpless_set_wel_insertion_strings( entry, count, insertions );
  va_end( insertions );

  return result;
}

struct stumpless_entry *
  stumpless_set_wel_insertion_strings_w( struct stumpless_entry *entry,
                                         WORD count,
                                         ... ) {
  va_list insertions;
  struct stumpless_entry *result;

  va_start( insertions, count );
  result = vstumpless_set_wel_insertion_strings_w( entry, count, insertions );
  va_end( insertions );

  return result;
}

struct stumpless_entry *
stumpless_set_wel_type( struct stumpless_entry *entry, WORD type ) {
  struct wel_data *data;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );
  data->type = type;
  data->type_set = TRUE;
  unlock_wel_data( data );

  clear_error();
  return entry;
}

struct stumpless_entry *
vstumpless_set_wel_insertion_strings( struct stumpless_entry *entry,
                                      WORD count,
                                      va_list insertions ) {
  struct wel_data *data;
  struct stumpless_entry *result;
  WORD i = 0;
  const char *arg;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );

  for( i = 0; i < count; i++ ) {
    arg = va_arg( insertions, char * );

    if( !arg ) {
      raise_argument_empty( L10N_NULL_ARG_ERROR_MESSAGE( "insertion string" ) );
      goto fail;
    }

    result = set_wel_insertion_string( entry, i, arg );
    if( !result ) {
      goto fail;
    }
  }

  unlock_wel_data( data );
  clear_error(  );
  return entry;

fail:
  unlock_wel_data( data );
  return NULL;
}

struct stumpless_entry *
vstumpless_set_wel_insertion_strings_w( struct stumpless_entry *entry,
                                        WORD count,
                                        va_list insertions ) {
  struct wel_data *data;
  struct stumpless_entry *result;
  WORD i = 0;
  LPCWSTR arg;

  VALIDATE_ARG_NOT_NULL( entry );

  data = entry->wel_data;
  lock_wel_data( data );

  for( i = 0; i < count; i++ ) {
    arg = va_arg( insertions, LPCWSTR );

    if( !arg ) {
      raise_argument_empty( L10N_NULL_ARG_ERROR_MESSAGE( "insertion string" ) );
      goto fail;
    }

    result = set_wel_insertion_string_w( entry, i, arg );
    if( !result ) {
      goto fail;
    }
  }

  unlock_wel_data( data );
  clear_error(  );
  return entry;

fail:
  unlock_wel_data( data );
  return NULL;
}

/* private definitions */

LPCWSTR
copy_param_value_to_lpwstr( const struct stumpless_param *param ) {
  LPWSTR str_copy;
  int needed_wchar_length;
  size_t needed_wchar_count;
  int value_length_int;
  int conversion_result;

  value_length_int = cap_size_t_to_int( param->value_length );

  needed_wchar_length = MultiByteToWideChar( CP_UTF8,
                                             MB_ERR_INVALID_CHARS |
                                               MB_PRECOMPOSED,
                                             param->value,
                                             value_length_int,
                                             NULL,
                                             0 );

  if( needed_wchar_length == 0 ) {
    raise_mb_conversion_failure( GetLastError(  ) );
    return NULL;
  }

  needed_wchar_count = ( ( size_t ) needed_wchar_length ) + 1;
  str_copy = alloc_mem( needed_wchar_count * sizeof( WCHAR ) );
  if( !str_copy ) {
    return NULL;
  }

  conversion_result = MultiByteToWideChar( CP_UTF8,
                                           MB_ERR_INVALID_CHARS |
                                             MB_PRECOMPOSED,
                                           param->value,
                                           value_length_int,
                                           str_copy,
                                           needed_wchar_length );

  if( conversion_result == 0 ) {
    free_mem( str_copy );
    raise_mb_conversion_failure( GetLastError(  ) );
    return NULL;
  }

  str_copy[needed_wchar_length] = L'\0';

  return str_copy;
}

struct stumpless_entry *
copy_wel_data( struct stumpless_entry *destination,
               const struct stumpless_entry *source ) {
  struct wel_data *dest_data;
  struct wel_data* source_data;
  WORD i;

  if( !config_initialize_wel_data( destination ) ) {
    return NULL;
  }

  source_data = source->wel_data;
  dest_data = destination->wel_data;
  lock_wel_data( source_data );

  dest_data->type = source_data->type;
  dest_data->type_set = source_data->type_set;
  dest_data->category = source_data->category;
  dest_data->category_set = source_data->category_set;
  dest_data->event_id = source_data->event_id;
  dest_data->event_id_set = source_data->event_id_set;


  if( source_data->insertion_count > 0 ) {
    dest_data->insertion_params = alloc_mem( sizeof( struct stumpless_param * ) * source_data->insertion_count );
    if( !dest_data->insertion_params) {
      goto fail;
    }

    dest_data->insertion_strings = alloc_mem( sizeof( LPCSTR ) * source_data->insertion_count );
    if( !dest_data->insertion_strings) {
      goto fail_strings;
    }

    for( i = 0; i < source_data->insertion_count; i++ ) {
      dest_data->insertion_params[i] = source_data->insertion_params[i];
      if( source_data->insertion_strings[i] ){
        dest_data->insertion_strings[i] = copy_lpcwstr( source_data->insertion_strings[i] );
        if( !dest_data->insertion_strings[i] ) {
          goto fail_set_strings;
        }
      } else {
        dest_data->insertion_strings[i] = NULL;
      }
    }

    dest_data->insertion_count = source_data->insertion_count;
  }

  unlock_wel_data( source_data );
  clear_error(  );
  return destination;

fail_set_strings:
  while( i > 0 ) {
    i -= 1;
    free_mem( dest_data->insertion_strings[i] );
  }
  free_mem( dest_data->insertion_strings );
fail_strings:
  free_mem( dest_data->insertion_params );
fail:
  unlock_wel_data( source_data );
  return NULL;
}

void
destroy_wel_data(const struct stumpless_entry* entry) {
  struct wel_data *data;
  WORD i;

  data = ( struct wel_data * ) entry->wel_data;
  config_destroy_mutex( &data->mutex );

  for( i = 0; i < data->insertion_count; i++ ) {
    free_mem( data->insertion_strings[i] );
  }

  free_mem( data->insertion_strings );
  free_mem( data->insertion_params );
  free_mem( data );
}

WORD
get_category( int prival ) {
  return get_severity( prival ) + 1;
}

DWORD
get_event_id( int prival ) {
  return ( get_facility( prival ) >> 3 ) + ( get_type( prival ) * 23 ) + 1;
}

WORD
get_type( int prival ) {
  switch( get_severity( prival ) ) {
    case STUMPLESS_SEVERITY_DEBUG_VALUE:
      return EVENTLOG_SUCCESS;

    case STUMPLESS_SEVERITY_NOTICE_VALUE:
    case STUMPLESS_SEVERITY_INFO_VALUE:
      return EVENTLOG_INFORMATION_TYPE;

    case STUMPLESS_SEVERITY_WARNING_VALUE:
      return EVENTLOG_WARNING_TYPE;

    case STUMPLESS_SEVERITY_EMERG_VALUE:
    case STUMPLESS_SEVERITY_ALERT_VALUE:
    case STUMPLESS_SEVERITY_CRIT_VALUE:
    case STUMPLESS_SEVERITY_ERR_VALUE:
    default:
      return EVENTLOG_ERROR_TYPE;
  }
}

bool
initialize_wel_data( struct stumpless_entry *entry ) {
  struct wel_data *data;

  data = alloc_mem( sizeof( *data ) );
  if( !data ){
    return false;
  }


  data->category_set = FALSE;
  data->event_id_set = FALSE;
  data->type_set = FALSE;
  data->insertion_strings = NULL;
  data->insertion_params = NULL;
  data->insertion_count = 0;
  config_init_mutex( &data->mutex );

  entry->wel_data = data;
  return true;
}

void
lock_wel_data( const struct wel_data *data ) {
  config_lock_mutex( &data->mutex );
}

struct stumpless_param **
resize_insertion_params( struct stumpless_entry *entry, WORD max_index ) {
  size_t new_max_index;
  size_t new_size;
  struct wel_data *data;
  struct stumpless_param **new_params;
  LPCWSTR *new_strings;
  WORD i;

  new_max_index = ( ( size_t ) max_index ) + 1;
  new_size = sizeof( *new_params ) * new_max_index;
  data = entry->wel_data;
  new_params = realloc_mem( data->insertion_params, new_size );
  if( !new_params ) {
    return NULL;

  } else {
    for( i = data->insertion_count; i <= max_index; i++ ) {
      new_params[i] = NULL;
    }

    data->insertion_params = new_params;

  }

  new_size = sizeof( LPCSTR ) * new_max_index;
  new_strings = realloc_mem( ( void * ) data->insertion_strings, new_size );
  if( !new_strings ) {
    return NULL;

  } else {
    for( i = data->insertion_count; i <= max_index; i++ ) {
      new_strings[i] = NULL;
    }

    data->insertion_strings = new_strings;

  }

  data->insertion_count = max_index + 1;
  return new_params;
}

void
set_entry_wel_type( struct stumpless_entry *entry, int severity ) {
  struct wel_data *data;

  data = entry->wel_data;

  switch ( severity ) {
    case STUMPLESS_SEVERITY_ERR:
      data->type = EVENTLOG_ERROR_TYPE;
      break;

    case STUMPLESS_SEVERITY_INFO:
      data->type = EVENTLOG_INFORMATION_TYPE;
      break;

    case STUMPLESS_SEVERITY_WARNING:
      data->type = EVENTLOG_WARNING_TYPE;
      break;

    default:
      data->type = EVENTLOG_SUCCESS;
  }
}

void
unlock_wel_data( const struct wel_data *data ) {
  config_unlock_mutex( &data->mutex );
}

struct stumpless_entry *
unsafe_swap_wel_insertion_string( struct stumpless_entry *entry,
                                  WORD index,
                                  LPCWSTR str ) {
  struct wel_data *data;

  data = entry->wel_data;
  if( index >= data->insertion_count ) {
    if( !resize_insertion_params( entry, index ) ) {
      return NULL;
    }
  } else {
    free_mem( data->insertion_strings[index] );
  }

  data->insertion_strings[index] = str;

  return entry;
}

struct stumpless_target *
wel_open_default_target( void ) {
  return stumpless_open_local_wel_target( "Stumpless" );
}
