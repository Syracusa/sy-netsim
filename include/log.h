/* Modified from https://github.com/Akagi201/lwlog/blob/master/lwlog.h(MIT License) */

#ifndef LOG_H_
#define LOG_H_ (1)

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

extern char dbgname[10];
extern FILE* dbgfile;

#ifndef LOG_LEVEL
#define LOG_LEVEL (7)
#endif

#ifndef LOG_COLOR
#define LOG_COLOR (1)
#endif

// log levels the same as syslog
#define FATALS (1)
#define CRIT (2)
#define ERR (3)
#define WARNING (4)
#define NOTICE (5)
#define INFO (6)
#define DEBUG (7)

#define __FILENAME__ (strrchr(__FILENAME__, '/') ? strrchr(__FILENAME__, '/') + 1 : __FILENAME__)

/* safe readable version of errno */
#define clean_errno() (errno == 0 ? "COLOR_NONE" : strerror(errno))

/* Modify this macro to specify scope */
#define DEFAULT_LOG_HDR " "

#ifndef LOG_HDR
#define LOG_HDR DEFAULT_LOG_HDR
#endif

/* Color list */
#define COLOR_NONE                 "\e[0m"
#define COLOR_RED                  "\e[0;41m"
#define COLOR_YELLOW               "\e[1;43m"
#define COLOR_BLUE                 "\e[0;44m"
#define COLOR_CYAN                 "\e[0;46m"
#define COLOR_GREEN                "\e[0;42m"
#define COLOR_GRAY                 "\e[0;47m"

#define FATAL_LOGCOLOR COLOR_RED
#define CRITICAL_LOGCOLOR COLOR_YELLOW
#define ERROR_LOGCOLOR COLOR_YELLOW
#define WARNING_LOGCOLOR COLOR_BLUE
#define NOTICE_LOGCOLOR COLOR_CYAN
#define INFO_LOGCOLOR COLOR_GREEN
#define DEBUG_LOGCOLOR COLOR_GRAY

/* Disable annoying vscode error squiggle */
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define _GET_CURRTIME(_tbuf) do{\
    struct timespec _ts;\
    clock_gettime(CLOCK_REALTIME, &_ts);\
    sprintf(_tbuf, "[%3lu:%3lu] ", _ts.tv_sec % 1000, _ts.tv_nsec / 1000000);\
}while(0);

/* Log macro */
#define _LOGPRT(_color, _loglv, M, ...)  do { fprintf(dbgfile, _color _loglv COLOR_NONE " " LOG_HDR  M, ##__VA_ARGS__); fflush(dbgfile); } while(0)
#define LOGF(M, ...) _LOGPRT(FATAL_LOGCOLOR, "F", M,  ##__VA_ARGS__)
#define LOGC(M, ...) _LOGPRT(CRITICAL_LOGCOLOR, "C", M,  ##__VA_ARGS__)
#define LOGE(M, ...) _LOGPRT(ERROR_LOGCOLOR, "E", M,  ##__VA_ARGS__)
#define LOGW(M, ...) _LOGPRT(WARNING_LOGCOLOR, "W", M,  ##__VA_ARGS__)
#define LOGN(M, ...) _LOGPRT(NOTICE_LOGCOLOR, "N", M,  ##__VA_ARGS__)
#define LOGI(M, ...) _LOGPRT(INFO_LOGCOLOR, "I", M,  ##__VA_ARGS__)
#define LOGD(M, ...) _LOGPRT(DEBUG_LOGCOLOR, "D", M,  ##__VA_ARGS__)

/* Log macro with where */
#define _WLOGPRT(_color, _loglv, M, ...) do { fprintf(dbgfile, _color _loglv COLOR_NONE " " LOG_HDR  "%s (%s:%d) " M, __func__, __FILENAME__, __LINE__, ##__VA_ARGS__); fflush(dbgfile); } while(0)
#define WLOGF(M, ...) _WLOGPRT(FATAL_LOGCOLOR, "F", M,  ##__VA_ARGS__)
#define WLOGC(M, ...) _WLOGPRT(CRITICAL_LOGCOLOR, "C", M,  ##__VA_ARGS__)
#define WLOGE(M, ...) _WLOGPRT(ERROR_LOGCOLOR, "E", M,  ##__VA_ARGS__)
#define WLOGW(M, ...) _WLOGPRT(WARNING_LOGCOLOR, "W", M,  ##__VA_ARGS__)
#define WLOGN(M, ...) _WLOGPRT(NOTICE_LOGCOLOR, "N", M,  ##__VA_ARGS__)
#define WLOGI(M, ...) _WLOGPRT(INFO_LOGCOLOR, "I", M,  ##__VA_ARGS__)
#define WLOGD(M, ...) _WLOGPRT(DEBUG_LOGCOLOR, "D", M,  ##__VA_ARGS__)

/* Log macro with Time */
#define _TLOGPRT(_color, _loglv, M, ...) do { char _tbuf[20]; _GET_CURRTIME(_tbuf); fprintf(dbgfile, _color _loglv COLOR_NONE " "  "%s %-7s"  M, _tbuf, dbgname, ##__VA_ARGS__); fflush(dbgfile); } while(0)
#define TLOGF(M, ...) _TLOGPRT(FATAL_LOGCOLOR, "F", M,  ##__VA_ARGS__)
#define TLOGC(M, ...) _TLOGPRT(CRITICAL_LOGCOLOR, "C", M,  ##__VA_ARGS__)
#define TLOGE(M, ...) _TLOGPRT(ERROR_LOGCOLOR, "E", M,  ##__VA_ARGS__)
#define TLOGW(M, ...) _TLOGPRT(WARNING_LOGCOLOR, "W", M,  ##__VA_ARGS__)
#define TLOGN(M, ...) _TLOGPRT(NOTICE_LOGCOLOR, "N", M,  ##__VA_ARGS__)
#define TLOGI(M, ...) _TLOGPRT(INFO_LOGCOLOR, "I", M,  ##__VA_ARGS__)
#define TLOGD(M, ...) _TLOGPRT(DEBUG_LOGCOLOR, "D", M,  ##__VA_ARGS__)

#define PRT_ERRNO() do{fprintf(dbgfile, "errno : %s\n", clean_errno());}while(0)

/* LOG_LEVEL controls */
#if LOG_LEVEL < DEBUG
#undef LOGD
#undef WLOGD
#undef TLOGD
#define LOGD(M, ...) do{}while(0)
#define WLOGD(M, ...) do{}while(0)
#define TLOGD(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < INFO
#undef LOGI
#undef WLOGI
#undef TLOGI
#define LOGI(M, ...) do{}while(0)
#define WLOGI(M, ...) do{}while(0)
#define TLOGI(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < NOTICE
#undef LOGN
#undef WLOGN
#undef TLOGN
#define LOGN(M, ...) do{}while(0)
#define WLOGN(M, ...) do{}while(0)
#define TLOGN(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < WARNING
#undef LOGW
#undef WLOGW
#undef TLOGW
#define LOGW(M, ...) do{}while(0)
#define WLOGW(M, ...) do{}while(0)
#define TLOGW(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < ERR
#undef LOGE
#undef WLOGE
#undef TLOGE
#define LOGE(M, ...) do{}while(0)
#define WLOGE(M, ...) do{}while(0)
#define TLOGE(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < CRIT
#undef LOGC
#undef WLOGC
#undef TLOGC
#define LOGC(M, ...) do{}while(0)
#define WLOGC(M, ...) do{}while(0)
#define TLOGC(M, ...) do{}while(0)
#endif

#if LOG_LEVEL < FATAL
#undef LOGF
#undef WLOGF
#undef TLOGF
#define LOGF(M, ...) do{}while(0)
#define WLOGF(M, ...) do{}while(0)
#define TLOGF(M, ...) do{}while(0)
#endif

/* LOG_COLOR controls */
#if LOG_COLOR < 1

#undef COLOR_NONE
#define COLOR_NONE

#undef COLOR_RED
#define COLOR_RED

#undef COLOR_YELLOW
#define COLOR_YELLOW

#undef COLOR_GREEN
#define COLOR_GREEN

#undef COLOR_CYAN
#define COLOR_CYAN

#undef COLOR_BLUE
#define COLOR_BLUE

#undef COLOR_GRAY
#define COLOR_GRAY

#endif

#endif