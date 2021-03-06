#include <stdarg.h>
#include <stdio.h>

#include "json_internal.h"

const char *error_out_of_memory = "out of memory";

tss_t error_key;

static void free_error(void *error) {
    if (!error || error == error_out_of_memory) {
        return;
    }
    json_free(error);
}

extern bool error_init(void) {
    return thrd_success == tss_create(&error_key, free_error);
}

extern void error_exit(void) {
    char *error = tss_get(error_key);
    free_error(error);
    tss_delete(error_key);
}

extern void set_error(const char *e) {
    char *error = tss_get(error_key);
    free_error(error);
    tss_set(error_key, (char *) e);
    if (e && e != error_out_of_memory) {
        json_mem_detach((char *) e);
    }
}

static char *vasprintf(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (n < 0) {
        return NULL;
    }
    char *text = json_malloc(n + 1);
    if (!text) {
        return NULL;
    }
    n = vsnprintf(text, n + 1, fmt, args);
    if (n < 0) {
        json_free(text);
        return NULL;
    }
    return text;
}

static char *asprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *result = vasprintf(fmt, args);
    va_end(args);
    return result;
}

extern void errorf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *message = vasprintf(fmt, args);
    va_end(args);
    if (!message) {
        set_error(NULL);
        return;
    }
    set_error(asprintf("at %u:%u %s", line, column, message));
    json_free(message);
}

