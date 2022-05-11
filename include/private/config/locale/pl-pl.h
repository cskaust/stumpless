/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright 2020-2022 Joel E. Anderson
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

#ifndef __STUMPLESS_PRIVATE_CONFIG_LOCALE_PL_PL_H
#  define __STUMPLESS_PRIVATE_CONFIG_LOCALE_PL_PL_H

#  define L10N_BIND_UNIX_SOCKET_FAILED_ERROR_MESSAGE \
"nie można podłączyć do gniazda unix"

#  define L10N_BUFFER_TOO_SMALL_ERROR_MESSAGE \
"buffer jest za mały dla tej wiadomości"

#  define L10N_CLOSE_UNSUPPORTED_TARGET_ERROR_MESSAGE \
"próba zamknięcia pliku docelowego nieobsługiwanego typu"

#  define L10N_COMMIT_TRANSACTION_FAILED_ERROR_MESSAGE \
"L10N MISSING COMMIT TRANSACTION FAILED ERROR MESSAGE"

#  define L10N_CONNECT_SYS_SOCKET_FAILED_ERROR_MESSAGE \
"awaria połączenia z gniazdem sys/socket.h socketem"

#  define L10N_CONNECT_WIN_SOCKET_FAILED_ERROR_MESSAGE \
"Awaria połączenia winsock socketem"

#  define L10N_CREATE_TRANSACTION_FAILED_ERROR_MESSAGE \
"L10N MISSING CREATE TRANSACTION FAILED ERROR MESSAGE"

#  define L10N_DESTINATION_NETWORK_ONLY_ERROR_MESSAGE \
"podany plik docelowy jest ważny tylko dla miejsc docelowych w sieci"

#  define L10N_DUPLICATE_ELEMENT_ERROR_MESSAGE \
"element o podanej nazwie już pojawia się w tym wejściu"

#  define L10N_ELEMENT_NOT_FOUND_ERROR_MESSAGE \
"nie można znaleźć elementu o określonej charakterystyce"

#  define L10N_ERRNO_ERROR_CODE_TYPE \
"połączenie zwróciło numer błędu (errno)"

#  define L10N_FILE_OPEN_FAILURE_ERROR_MESSAGE \
"komunikat o błędzie - nie udało się otworzyć pliku"

#  define L10N_FILE_WRITE_FAILURE_ERROR_MESSAGE \
"komunikat o błędzie - nie udało się zapisać"

#  define L10N_FUNCTION_TARGET_FAILURE_CODE_TYPE \
"kod powrotu funkcji obsługi protokołu"

#  define L10N_FUNCTION_TARGET_FAILURE_ERROR_MESSAGE \
"obsługa protokołu dla celu funkcji nie powiodła się"

#  define L10N_LOCAL_SOCKET_NAME_FILE_OPEN_ERROR_MESSAGE \
"nie jest możliwe utworzenie pliku z wybraną nazwą gniazda lokalnego przy użyciu mkstemp"

#  define L10N_GETADDRINFO_FAILURE_ERROR_MESSAGE \
"funkcja getaddrinfor nie powiodła się podczas podawania nazwy hosta"

#  define L10N_GETADDRINFO_RETURN_ERROR_CODE_TYPE \
"wywołanie getaddrinfo zwróciło błąd zwracanej wartości"

#  define L10N_GETCOMPUTERNAME_FAILED_ERROR_MESSAGE \
"GetComputerName przegrany"

#  define L10N_GETHOSTNAME_FAILED_ERROR_MESSAGE \
"gethostname przegrany"

#  define L10N_GETLASTERROR_ERROR_CODE_TYPE \
"wynik GetLastError po nieudanym wywołaniu"

#  define L10N_INDEX_OUT_OF_BOUNDS_ERROR_CODE_TYPE \
"nieprawidłowy indeks - indeks jest zbyt duży, aby mógł być reprezentowany jako typ danych int"

#  define L10N_INVALID_FACILITY_ERROR_CODE_TYPE \
"niezgodne urządzenie"

#  define L10N_INVALID_FACILITY_ERROR_MESSAGE \
"wartość kodu urządzenia, po podzieleniu przez 8, należy zdefiniować zgodnie z protokołem RFC 5424"

#  define L10N_INVALID_ID_ERROR_MESSAGE \
"nieprawidłowy cel ID"

#  define L10N_INVALID_INDEX_ERROR_MESSAGE( INDEXED_THING ) \
"nieprawidłowy indeks " INDEXED_THING

#  define L10N_INVALID_SEVERITY_ERROR_CODE_TYPE \
"nieprawidłowa usługa"

#  define L10N_INVALID_SEVERITY_ERROR_MESSAGE \
"kod serwisowy musi być zdefiniowany zgodnie ze standardem RFC 5424: wartości pomiędzy 0 a 7 łącznie z"

#  define L10N_JOURNALD_FAILURE_ERROR_CODE_TYPE \
"kod powrotu sd_journal_sendv"

#  define L10N_JOURNALD_FAILURE_ERROR_MESSAGE \
"sd_journal_sendv przegrany"

#  define L10N_MAX_MESSAGE_SIZE_UDP_ONLY_ERROR_MESSAGE \
"maksymalny rozmiar wiadomości obowiązuje tylko dla UDP"

#  define L10N_MB_TO_WIDE_CONVERSION_ERROR_CODE_TYPE \
"L10N MISSING MB TO WIDE CONVERSION ERROR CODE TYPE"

#  define L10N_MB_TO_WIDE_CONVERSION_ERROR_MESSAGE \
"L10N MISSING MB TO WIDE CONVERSION ERROR MESSAGE"

#  define L10N_MEMORY_ALLOCATION_FAILURE_ERROR_MESSAGE \
"wywołanie alokacji pamięci nie powiodło się"

#  define L10N_MESSAGE_TOO_BIG_FOR_DATAGRAM_ERROR_MESSAGE \
"wiadomość jest zbyt duża, aby można ją było wysłać w jednym datagramie"

#  define L10N_MESSAGE_SIZE_ERROR_CODE_TYPE \
"rozmiar wiadomości, która próbowała je wysłać "

#  define L10N_NETWORK_PROTOCOL_UNSUPPORTED_ERROR_MESSAGE \
"wybrany protokół sieciowy nie jest obsługiwany"

#  define L10N_NETWORK_TARGETS_UNSUPPORTED \
"cele sieciowe nie są obsługiwane przez tę kompilację"

#  define L10N_NULL_ARG_ERROR_MESSAGE( ARG_NAME ) \
ARG_NAME " miał wartość NULL"

#  define L10N_OPEN_UNSUPPORTED_TARGET_ERROR_MESSAGE \
"podjęto próbę otwarcia celu nieobsługiwanego typu"

#  define L10N_PARAM_NOT_FOUND_ERROR_MESSAGE \
"określony parametr nie został znaleziony"

#  define L10N_REGISTRY_SUBKEY_CREATION_FAILED_ERROR_MESSAGE \
"L10N MISSING REGISTRY SUBKEY CREATION FAILED ERROR MESSAGE"

#  define L10N_REGISTRY_VALUE_SET_FAILED_ERROR_MESSAGE \
"L10N MISSING REGISTRY VALUE SET FAILED ERROR MESSAGE"

#  define L10N_SEND_ENTRY_TO_UNSUPPORTED_TARGET_ERROR_MESSAGE \
"próba wysłania nieobsługiwanego typu danych wejściowych do celu"

#  define L10N_SEND_MESSAGE_TO_UNSUPPORTED_TARGET_ERROR_MESSAGE \
"próba wysłania wejścia nieobsługiwanego typu"

#  define L10N_SEND_SYS_SOCKET_FAILED_ERROR_MESSAGE \
"wysyłanie gniazda systemowego (sys/socket.h) przegrany"

#  define L10N_SENDTO_UNIX_SOCKET_FAILED_ERROR_MESSAGE \
"wysyłanie uniksa socketu (unix socket) przegrany"

#  define L10N_SEND_WIN_SOCKET_FAILED_ERROR_MESSAGE \
"wysyłanie winsock2 socketu przegrany"

#  define L10N_SOCKET_FAILED_ERROR_MESSAGE \
"otwór w gnieździe przegrany"

#  define L10N_STREAM_WRITE_FAILURE_ERROR_MESSAGE \
"nie mogę pisać do streamu"

#  define L10N_TARGET_ALWAYS_OPEN_ERROR_MESSAGE \
"cel danego typu jest nadal otwarty"

#  define L10N_TRANSPORT_PORT_NETWORK_ONLY_ERROR_MESSAGE \
"porty transportowe są ważne tylko dla miejsc docelowych w sieci"

#  define L10N_TRANSPORT_PROTOCOL_UNSUPPORTED_ERROR_MESSAGE \
"wybrany protokół transportowy nie jest obsługiwany"

#  define L10N_UNIX_SOCKET_FAILED_ERROR_MESSAGE \
"błąd otwierania gniazda uniksowego przy użyciu funkcji gniazda"

#  define L10N_UNSUPPORTED_TARGET_IS_OPEN_ERROR_MESSAGE \
"sprawdź rozszerzalność pliku docelowego nieobsługiwanego typu"

#  define L10N_WEL_CLOSE_FAILURE_ERROR_MESSAGE \
"nie można zamknąć dziennika zdarzeń systemowych Windows"

#  define L10N_WEL_OPEN_FAILURE_ERROR_MESSAGE \
"Kod błędu Windows Socket"

#  define L10N_WIDE_TO_MB_CONVERSION_ERROR_CODE_TYPE \
"L10N MISSING WIDE TO MB CONVERSION ERROR CODE TYPE"

#  define L10N_WIDE_TO_MB_CONVERSION_ERROR_MESSAGE \
"L10N MISSING WIDE TO MB CONVERSION ERROR MESSAGE"

#  define L10N_WINDOWS_SOCKET_ERROR_CODE_TYPE \
"błąd zwracane wartości (error code) Windows socketu"

#  define L10N_WINDOWS_RETURN_ERROR_CODE_TYPE \
"L10N MISSING WINDOWS RETURN ERROR CODE TYPE"

#  define L10N_WINSOCK2_SOCKET_FAILED_ERROR_MESSAGE \
"awaria otwierania winsock2 socketu"

#  define L10N_WSAGETLASTERROR_ERROR_CODE_TYPE \
"wynik WSAGetLastError po niepowodzeniu połączenia"

#  define L10N_STRING_TOO_LONG_ERROR_MESSAGE \
"długość ciągu przekroczyła maksymalny limit"

#  define L10N_STRING_LENGTH_ERROR_CODE_TYPE \
"długość łańcucha problemu to"

#  define L10N_FORMAT_ERROR_MESSAGE(ARG) \
"nieważny " ARG " formát"

#  define L10N_INVALID_TARGET_TYPE_ERROR_MESSAGE \
"INVALID TARGET TYPE ERROR MESSAGE"

#endif //STUMPLESS_PL_PL_H
