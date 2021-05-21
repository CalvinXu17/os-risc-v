/* Copyright 2018 Canaan Inc.
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
#ifndef DRIVER_UTILS_H
#define DRIVER_UTILS_H

#include "type.h"

void set_bits_value(volatile uint32 *bits, uint32 mask, uint32 value);
void set_bits_value_offset(volatile uint32 *bits, uint32 mask, uint32 value, uint32 offset);
void set_bit(volatile uint32 *bits, uint32 offset);
void clear_bit(volatile uint32 *bits, uint32 offset);

#endif /* _DRIVER_COMMON_H */
