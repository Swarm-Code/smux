/* Scroll and Rendering Debug Instrumentation */

#ifndef SCROLL_DEBUG_H
#define SCROLL_DEBUG_H

#include <time.h>
#include <stdio.h>

/* Debug event types */
#define DEBUG_SCROLL_EVENT      1
#define DEBUG_SYNC_START        2
#define DEBUG_SYNC_END          3
#define DEBUG_REDRAW_TRIGGER    4
#define DEBUG_BUFFER_STATE      5
#define DEBUG_CURSOR_MOVE       6

/* Global debug file handle */
extern FILE *scroll_debug_log;

/* Initialize debug logging */
static inline void scroll_debug_init(void) {
    if (scroll_debug_log == NULL) {
        scroll_debug_log = fopen("/tmp/smux_scroll_debug.log", "a");
        if (scroll_debug_log != NULL) {
            fprintf(scroll_debug_log, "=== SMUX SCROLL DEBUG SESSION START ===\n");
            fflush(scroll_debug_log);
        }
    }
}

/* Log a scroll event */
#define SCROLL_DEBUG_LOG(type, fmt, ...) \
    do { \
        if (scroll_debug_log != NULL) { \
            struct timespec ts; \
            clock_gettime(CLOCK_MONOTONIC, &ts); \
            fprintf(scroll_debug_log, "[%ld.%09ld] " fmt "\n", \
                ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
            fflush(scroll_debug_log); \
        } \
    } while(0)

/* Scroll event logging with context */
#define LOG_SCROLL_EVENT(lines, pane, tty_buf_size) \
    SCROLL_DEBUG_LOG(DEBUG_SCROLL_EVENT, \
        "[SCROLL] lines=%d pane=%p tty_buf=%u", \
        lines, pane, tty_buf_size)

/* Sync region logging */
#define LOG_SYNC_START(tty) \
    SCROLL_DEBUG_LOG(DEBUG_SYNC_START, \
        "[SYNC_START] tty=%p buffer_len=%zu", \
        tty, (tty ? evbuffer_get_length(((struct tty *)tty)->out) : 0))

#define LOG_SYNC_END(tty) \
    SCROLL_DEBUG_LOG(DEBUG_SYNC_END, \
        "[SYNC_END] tty=%p buffer_len=%zu", \
        tty, (tty ? evbuffer_get_length(((struct tty *)tty)->out) : 0))

/* Redraw trigger logging */
#define LOG_REDRAW_TRIGGER(flags_str, client) \
    SCROLL_DEBUG_LOG(DEBUG_REDRAW_TRIGGER, \
        "[REDRAW] trigger=%s client=%p", \
        flags_str, client)

/* TTY buffer state */
#define LOG_BUFFER_STATE(tty, sz, pct) \
    SCROLL_DEBUG_LOG(DEBUG_BUFFER_STATE, \
        "[BUFFER] tty=%p size=%zu percent=%d", \
        tty, sz, pct)

#endif /* SCROLL_DEBUG_H */
