/*
  JACK control API

  Copyright (C) 2008 Nedko Arnaudov

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __control_types__
#define __control_types__

#include "jslist.h"
#include "JackExports.h"

#ifdef WIN32
#ifdef __MINGW32__
#include <sys/types.h>
#else
typedef HANDLE sigset_t;
#endif
#endif

/** Parameter types, intentionally similar to jack_driver_param_type_t */
typedef enum
{
    JackParamInt = 1,			/**< @brief value type is a signed integer */
    JackParamUInt,				/**< @brief value type is an unsigned integer */
    JackParamChar,				/**< @brief value type is a char */
    JackParamString,			/**< @brief value type is a string with max size of ::JACK_PARAM_STRING_MAX+1 chars */
    JackParamBool,				/**< @brief value type is a boolean */
} jackctl_param_type_t;

/** @brief Max value that jackctl_param_type_t type can have */
#define JACK_PARAM_MAX (JackParamBool + 1)

/** @brief Max length of string parameter value, excluding terminating nul char */
#define JACK_PARAM_STRING_MAX  63

/** @brief Type for parameter value */
/* intentionally similar to jack_driver_param_value_t */
union jackctl_parameter_value
{
    uint32_t ui;				/**< @brief member used for ::JackParamUInt */
    int32_t i;					/**< @brief member used for ::JackParamInt */
    char c;						/**< @brief member used for ::JackParamChar */
    char str[JACK_PARAM_STRING_MAX + 1]; /**< @brief member used for ::JackParamString */
    bool b;				/**< @brief member used for ::JackParamBool */
};

/** opaque type for server object */
typedef struct jackctl_server jackctl_server_t;

/** opaque type for driver object */
typedef struct jackctl_driver jackctl_driver_t;

/** opaque type for parameter object */
typedef struct jackctl_parameter jackctl_parameter_t;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

EXPORT sigset_t
jackctl_setup_signals(
    unsigned int flags);

EXPORT void
jackctl_wait_signals(
    sigset_t signals);

EXPORT jackctl_server_t *
jackctl_server_create();

EXPORT void
jackctl_server_destroy(
	jackctl_server_t * server);

EXPORT const JSList *
jackctl_server_get_drivers_list(
	jackctl_server_t * server);

EXPORT bool
jackctl_server_start(
    jackctl_server_t * server,
    jackctl_driver_t * driver);

EXPORT bool
jackctl_server_stop(
	jackctl_server_t * server);

EXPORT const JSList *
jackctl_server_get_parameters(
	jackctl_server_t * server);

EXPORT const char *
jackctl_driver_get_name(
	jackctl_driver_t * driver);

EXPORT const JSList *
jackctl_driver_get_parameters(
	jackctl_driver_t * driver);

EXPORT const char *
jackctl_parameter_get_name(
	jackctl_parameter_t * parameter);

EXPORT const char *
jackctl_parameter_get_short_description(
	jackctl_parameter_t * parameter);

EXPORT const char *
jackctl_parameter_get_long_description(
	jackctl_parameter_t * parameter);

EXPORT jackctl_param_type_t
jackctl_parameter_get_type(
	jackctl_parameter_t * parameter);

EXPORT char
jackctl_parameter_get_id(
	jackctl_parameter_t * parameter);

EXPORT bool
jackctl_parameter_is_set(
	jackctl_parameter_t * parameter);

EXPORT bool
jackctl_parameter_reset(
	jackctl_parameter_t * parameter);

EXPORT union jackctl_parameter_value
jackctl_parameter_get_value(
	jackctl_parameter_t * parameter);

EXPORT bool
jackctl_parameter_set_value(
	jackctl_parameter_t * parameter,
	const union jackctl_parameter_value * value_ptr);

EXPORT union jackctl_parameter_value
jackctl_parameter_get_default_value(
	jackctl_parameter_t * parameter);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
