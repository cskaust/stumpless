/* SPDX-License-Identifier: Apache-2.0 */

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

#ifndef __STUMPLESS_PRIVATE_SEVERITY_H
#  define __STUMPLESS_PRIVATE_SEVERITY_H

/**
 * Gets the value of the severity from the given prival. This will be equivalent
 * to the STUMPLESS_SEVERITY_*_VALUE constant for the severity.
 *
 * @param prival The prival to extract the severity from.
 *
 * @return the severity of the prival, as an integer.
 */
int
get_severity( int prival );

int
severity_is_invalid( int severity );

#endif /* __STUMPLESS_PRIVATE_SEVERITY_H */
