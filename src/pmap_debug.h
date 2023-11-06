/*
 *    pmap_debug.h
 *
 *    Copyright (c) 2023 Alien Green LLC
 *
 *    This file is part of Mostat.
 *
 *    Mostat is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Mostat is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Mostat. If not, see <http://www.gnu.org/licenses/>.
 *
 *    ASCII font see http://patorjk.com/software/taag/#p=display&f=3D-ASCII
 */

#ifndef _PMAP_DEBUG_H
#define _PMAP_DEBUG_H

#include "pmap_cfg.h"

#if PMAP_DEBUG_LOG_DEBUG

/* Here is the debug routine */

/* Log debug information */
#define PMAP_DEBUG_LOG(format, args...)                                        \
  do {                                                                         \
    PMAP_PRINTF("%s: " format, __func__, ##args);                              \
  } while (0)

/* Log debug information */
#define PMAP_DEBUG_HEX_LOG(d, s, format, args...)                              \
  do {                                                                         \
    PMAP_PRINTF("%s: " format, __func__, ##args);                              \
    pmap_ut_dump_hex(d, s);                                                    \
  } while (0)

#else
/* Then do nothing (easy case) */
#define PMAP_DEBUG_LOG(format, args...)
#define PMAP_DEBUG_HEX_LOG(d, sformat, args...)

#endif

/* Log debug runtime */
#define PMAP_RUNTIME_LOG(format, args...)                                      \
  do {                                                                         \
    PMAP_PRINTF(format, ##args);                                               \
  } while (0)

#if PMAP_DEBUG_LOG_ERROR

/* Log debug error information */
#define PMAP_DEBUG_ERROR(format, args...)                                      \
  do {                                                                         \
    PMAP_PRINTF("%s: " format, __func__, ##args);                              \
  } while (0)

#else

#define PMAP_DEBUG_ERROR(format, args...)

#endif

#endif /* _PMAP_DEBUG_H */
