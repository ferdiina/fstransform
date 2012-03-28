/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * log.hh
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_LOG_HH
#define FSTRANSFORM_LOG_HH

#include "types.hh"      /* for ft_string */

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>      /* for EINVAL */
#elif defined(FT_HAVE_CERRNO) && defined(__cplusplus)
# include <cerrno>       /* for EINVAL */
#endif
#if defined(FT_HAVE_STDARG_H)
# include <stdarg.h>     /* for va_list. also for va_start(), va_end(), va_copy() used by log.cc */
#elif defined(FT_HAVE_CSTDARG) && defined(__cplusplus)
# include <cstdarg>      /* for va_list. also for va_start(), va_end(), va_copy() used by log.cc */
#endif
#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>      /* for FILE. also for stdout, stderr used by log.cc */
#elif defined(FT_HAVE_CSTDIO) && defined(__cplusplus)
# include <cstdio>       /* for FILE. also for stdout, stderr used by log.cc */
#endif

#include <list>          /* for std::list<T>  */
#include <map>           /* for std::map<K,V> */

FT_NAMESPACE_BEGIN


/**
 * note 1.1)
 * log subsystem is automatically initialized and configured upon first call to
 * ff_log(), ff_vlog(), ff_log_register_range(), ff_log_unregister_range() or ff_log_set_threshold().
 *
 * automatic configuration is:
 * print to stdout all INFO and NOTICE messages, with format FC_FMT_MSG
 * print to stderr all WARN, ERROR and FATAL messages, with format FC_FMT_MSG
 */


/* FC_FATAL is reserved for things that should not happen, i.e. bugs in the program or in the operating system. */
enum ft_log_level { FC_LEVEL_NOT_SET, FC_DUMP, FC_TRACE, FC_DEBUG, FC_INFO, FC_NOTICE, FC_WARN, FC_ERROR, FC_FATAL };


enum ft_log_fmt {
    FC_FMT_MSG, /* message only */
    FC_FMT_LEVEL_MSG, /* level + message */
    FC_FMT_DATETIME_LEVEL_MSG, /* datetime + level + message */
    FC_FMT_DATETIME_LEVEL_CALLER_MSG, /* datetime + level + [file.func(line)] + message */
};


/**
 * print to log fmt and subsequent printf-style args log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err
 */
#define ff_log_is_enabled(level)        ff_logl_is_enabled(FT_THIS_FILE, level)
#define ff_log(level, err, ...)         ff_logl(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, level, err, __VA_ARGS__)
#define ff_vlog(level, err, fmt, vargs) ff_logv(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, level, err, fmt, vargs)

bool ff_logl_is_enabled(const char * caller_file, ft_log_level level);
int  ff_logl(const char * caller_file, const char * caller_func, int caller_line, ft_log_level level, int err, const char * fmt, ...);
int  ff_logv(const char * caller_file, const char * caller_func, int caller_line, ft_log_level level, int err, const char * fmt, va_list args);


#if defined(EINVAL) && EINVAL < 0
#  define ff_log_is_reported(err) ((err) >= 0)
#else
#  define ff_log_is_reported(err) ((err) <= 0)
#endif

struct ft_log_event
{
    const char * str_now, * file, * file_suffix, * function, * fmt;
    int file_len, line, err;
    ft_log_level level;
    va_list vargs;
};

class ft_log_appender {
private:
    FILE * stream;
    ft_log_fmt format;
    ft_log_level min_level, max_level;
    
    /** destructor. */
    ~ft_log_appender();

    /** list of all appenders */
    static std::list<ft_log_appender *> & get_all_appenders();

public:
    /** constructor. */
    ft_log_appender(FILE * stream, ft_log_fmt format = FC_FMT_MSG, ft_log_level min_level = FC_TRACE, ft_log_level max_level = FC_FATAL);
    
    FT_INLINE void set_format(ft_log_fmt format) { this->format = format; }

    FT_INLINE void set_min_level(ft_log_level min_level) { this->min_level = min_level; }
    FT_INLINE void set_max_level(ft_log_level max_level) { this->max_level = max_level; }
    
    /** write a log message to stream */
    void append(ft_log_event & event);

    /** flush this appender */
    void flush();
    
    /** flush all buffered streams used to log messages for specified level */
    static void flush_all(ft_log_level level);

    /** set format and min/max levels of this appender */
    void redefine(ft_log_fmt format = FC_FMT_MSG, ft_log_level min_level = FC_DEBUG, ft_log_level max_level = FC_FATAL);

    /** set format and min/max levels of all appenders attached to stream */
    static void redefine(FILE * stream, ft_log_fmt format = FC_FMT_MSG, ft_log_level min_level = FC_DEBUG, ft_log_level max_level = FC_FATAL);
};

                    
class ft_log {
private:
    typedef std::map<ft_string, ft_log *>::iterator all_loggers_iterator;

	friend class ft_log_appender;

    const ft_string * name;
    ft_log * parent;
    std::list<ft_log_appender *> appenders;
    ft_log_level level; /* events less severe than level will be suppressed */

    /** initialize loggers and appenders. */
    static void initialize();

    /** return map of all existing loggers. */
    static std::map<ft_string, ft_log *> & get_all_loggers();
    
    /** constructor. */
    ft_log(const ft_string & name, ft_log * parent, ft_log_level level = FC_LEVEL_NOT_SET);

    /** destructor. */
    ~ft_log();

    /** find or create parent logger given child name. */
    static ft_log & get_parent(const ft_string & child_logger_name);

    /** log a message (skip level check) */
    void append(ft_log_event & event);
    
public:

    /** return root logger */
    static ft_log & get_root_logger();

    /** find or create a logger by name */
    static ft_log & get_logger(const ft_string & logger_name);

    /** log a message (unless its level is suppressed) */
    void log(ft_log_event & event);

    /** return true if level is enabled (i.e. not suppressed) for this logger. */
    FT_INLINE bool is_enabled(ft_log_level level) const { return level >= get_effective_level(); }


    /** get logger name. */
    FT_INLINE const ft_string & get_name() const { return * name; }
    
    /** return the level, i.e. least serious level that is not suppressed. */
    FT_INLINE ft_log_level get_level() const { return level; }

    /** set the level, i.e. least serious level that is not suppressed. */
    FT_INLINE void set_level(ft_log_level level) { this->level = level; }

    /** return the effective level: if level is set return it, otherwise return parent effective level. */
    ft_log_level get_effective_level() const;


    /** add an appender */
    void add_appender(ft_log_appender & appender);
    
    /** remove an appender */
    void remove_appender(ft_log_appender & appender);
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_LOG_HH */
