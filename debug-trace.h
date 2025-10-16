/* $OpenBSD$ */

/*
 * COMPREHENSIVE DEBUG TRACING FOR SMUX HANG INVESTIGATION
 * This header provides extensive logging and tracing facilities
 */

#ifndef DEBUG_TRACE_H
#define DEBUG_TRACE_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Debug levels */
#define DEBUG_LEVEL_CRITICAL 0
#define DEBUG_LEVEL_ERROR    1
#define DEBUG_LEVEL_WARN     2
#define DEBUG_LEVEL_INFO     3
#define DEBUG_LEVEL_DEBUG    4
#define DEBUG_LEVEL_TRACE    5

extern int debug_trace_level;
extern FILE *debug_trace_file;

/* Initialize debug tracing */
static inline void
debug_trace_init(const char *filename)
{
	debug_trace_file = fopen(filename, "a");
	if (debug_trace_file == NULL) {
		fprintf(stderr, "Failed to open debug trace file: %s\n", filename);
		debug_trace_file = stderr;
	}
	debug_trace_level = DEBUG_LEVEL_TRACE;
	fprintf(debug_trace_file, "\n=== DEBUG TRACE SESSION STARTED (PID=%d) ===\n\n", getpid());
	fflush(debug_trace_file);
}

/* Get timestamp with microsecond precision */
static inline void
debug_get_timestamp(char *buf, size_t len)
{
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
	    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);
}

/* Core debug macro */
#define DEBUG_TRACE(level, fmt, ...) do { \
	if (debug_trace_level >= level && debug_trace_file != NULL) { \
		char timestamp[32]; \
		debug_get_timestamp(timestamp, sizeof(timestamp)); \
		fprintf(debug_trace_file, "[%s] [%s:%d:%s] " fmt "\n", \
		    timestamp, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
		fflush(debug_trace_file); \
	} \
} while (0)

/* Convenience macros */
#define DEBUG_CRITICAL(fmt, ...) DEBUG_TRACE(DEBUG_LEVEL_CRITICAL, "CRITICAL: " fmt, ##__VA_ARGS__)
#define DEBUG_ERROR(fmt, ...)    DEBUG_TRACE(DEBUG_LEVEL_ERROR, "ERROR: " fmt, ##__VA_ARGS__)
#define DEBUG_WARN(fmt, ...)     DEBUG_TRACE(DEBUG_LEVEL_WARN, "WARN: " fmt, ##__VA_ARGS__)
#define DEBUG_INFO(fmt, ...)     DEBUG_TRACE(DEBUG_LEVEL_INFO, "INFO: " fmt, ##__VA_ARGS__)
#define DEBUG_DEBUG(fmt, ...)    DEBUG_TRACE(DEBUG_LEVEL_DEBUG, "DEBUG: " fmt, ##__VA_ARGS__)
#define DEBUG_TRACE_MSG(fmt, ...)    DEBUG_TRACE(DEBUG_LEVEL_TRACE, "TRACE: " fmt, ##__VA_ARGS__)

/* Function entry/exit tracing */
#define DEBUG_ENTER() DEBUG_TRACE_MSG(">>> ENTER")
#define DEBUG_EXIT()  DEBUG_TRACE_MSG("<<< EXIT")
#define DEBUG_EXIT_INT(val) do { \
	DEBUG_TRACE_MSG("<<< EXIT (return=%d)", val); \
	return (val); \
} while (0)
#define DEBUG_EXIT_PTR(val) do { \
	DEBUG_TRACE_MSG("<<< EXIT (return=%p)", val); \
	return (val); \
} while (0)
#define DEBUG_EXIT_VOID() do { \
	DEBUG_TRACE_MSG("<<< EXIT (void)"); \
	return; \
} while (0)

/* Loop iteration tracing */
#define DEBUG_LOOP_ITER(iter) DEBUG_TRACE_MSG("Loop iteration %d", iter)

/* Checkpoint tracing */
#define DEBUG_CHECKPOINT(name) DEBUG_TRACE_MSG("CHECKPOINT: %s", name)

/* Variable value tracing */
#define DEBUG_VAR_INT(var) DEBUG_TRACE_MSG("%s = %d", #var, var)
#define DEBUG_VAR_PTR(var) DEBUG_TRACE_MSG("%s = %p", #var, var)
#define DEBUG_VAR_STR(var) DEBUG_TRACE_MSG("%s = \"%s\"", #var, (var) ? (var) : "(null)")

/* System call tracing */
#define DEBUG_SYSCALL(call, ...) do { \
	DEBUG_TRACE_MSG("SYSCALL: " #call "(%s)", #__VA_ARGS__); \
	int _ret = call(__VA_ARGS__); \
	if (_ret < 0) { \
		DEBUG_ERROR("SYSCALL FAILED: " #call " returned %d (errno=%d: %s)", \
		    _ret, errno, strerror(errno)); \
	} else { \
		DEBUG_TRACE_MSG("SYSCALL OK: " #call " returned %d", _ret); \
	} \
} while (0)

/* Event loop tracing */
#define DEBUG_EVENT_LOOP_START() DEBUG_TRACE_MSG("=== EVENT LOOP START ===")
#define DEBUG_EVENT_LOOP_ITER()  DEBUG_TRACE_MSG("--- Event loop iteration ---")
#define DEBUG_EVENT_LOOP_END()   DEBUG_TRACE_MSG("=== EVENT LOOP END ===")

/* Format processing tracing */
#define DEBUG_FORMAT_START(str) DEBUG_TRACE_MSG("FORMAT START: \"%s\"", str)
#define DEBUG_FORMAT_EXPAND(key, val) DEBUG_TRACE_MSG("FORMAT EXPAND: #{%s} -> \"%s\"", key, val)
#define DEBUG_FORMAT_END(result) DEBUG_TRACE_MSG("FORMAT END: \"%s\"", result)

/* Style processing tracing */
#define DEBUG_STYLE_START(str) DEBUG_TRACE_MSG("STYLE START: \"%s\"", str)
#define DEBUG_STYLE_PARSE(attr) DEBUG_TRACE_MSG("STYLE PARSE: %s", attr)
#define DEBUG_STYLE_END() DEBUG_TRACE_MSG("STYLE END")

/* Project/Session tracing */
#define DEBUG_PROJECT_CREATE(name) DEBUG_TRACE_MSG("PROJECT CREATE: \"%s\"", name)
#define DEBUG_PROJECT_DESTROY(name) DEBUG_TRACE_MSG("PROJECT DESTROY: \"%s\"", name)
#define DEBUG_SESSION_CREATE(name) DEBUG_TRACE_MSG("SESSION CREATE: \"%s\"", name)
#define DEBUG_SESSION_DESTROY(name) DEBUG_TRACE_MSG("SESSION DESTROY: \"%s\"", name)

/* Hang detection - periodic heartbeat */
#define DEBUG_HEARTBEAT() DEBUG_TRACE_MSG("*** HEARTBEAT ***")

/* Memory allocation tracing */
#define DEBUG_ALLOC(ptr, size) DEBUG_TRACE_MSG("ALLOC: %p (size=%zu)", ptr, (size_t)size)
#define DEBUG_FREE(ptr) DEBUG_TRACE_MSG("FREE: %p", ptr)

/* Close debug tracing */
static inline void
debug_trace_close(void)
{
	if (debug_trace_file != NULL && debug_trace_file != stderr) {
		fprintf(debug_trace_file, "\n=== DEBUG TRACE SESSION ENDED ===\n");
		fflush(debug_trace_file);
		fclose(debug_trace_file);
		debug_trace_file = NULL;
	}
}

/* Global variables (must be defined in one .c file) */
#ifndef DEBUG_TRACE_IMPL
extern int debug_trace_level;
extern FILE *debug_trace_file;
#endif

#endif /* DEBUG_TRACE_H */
