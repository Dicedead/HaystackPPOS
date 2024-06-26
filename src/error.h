#pragma once

/**
 * @file error.h
 * @brief Error codes for PPS course (CS-212)
 *
 * @author E. Bugnion, J.-C. Chappelier, V. Rousset
 * @date 2016-2021
 */

#ifdef DEBUG
#include <stdio.h> // for fprintf
#endif
#include <string.h> // strerror()
#include <errno.h>  // errno
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================================
/**
 * @brief internal error codes.
 *
 */
typedef enum {
    ERR_CLANG_TYPE_FIX = -1, // this stupid value is to fix type to be int instead of unsigned on some compilers (e.g. clang version 8.0)
    ERR_NONE = 0, // no error

    ERR_IO,
    ERR_OUT_OF_MEMORY,
    ERR_NOT_ENOUGH_ARGUMENTS,
    ERR_INVALID_FILENAME,
    ERR_INVALID_COMMAND,
    ERR_INVALID_ARGUMENT,
    ERR_MAX_FILES,
    ERR_RESOLUTIONS,
    ERR_INVALID_IMGID,
    ERR_FULL_IMGSTORE,
    ERR_FILE_NOT_FOUND,
    NOT_IMPLEMENTED,
    ERR_DUPLICATE_ID,
    ERR_IMGLIB,
    ERR_DEBUG,

    NB_ERR // not an actual error but to have the total number of errors
} error_code;

#define DO_LIST_UNIMP_MSG ("unimplemented do_list output mode")

//Helper functions

void free_list(size_t n, ... );


// ======================================================================
/*
 * Helpers (macros)
 */
// ----------------------------------------------------------------------
#define VIPS_ERR_NONE 0
#define JSON_ERR_NONE 0

// ----------------------------------------------------------------------
/**
 * @brief debug_print macro is useful to print message in DEBUG mode only.
 */
#ifdef DEBUG
#define debug_print(fmt, ...) \
    fprintf(stderr, __FILE__ ":%d:%s(): " fmt "\n", \
       __LINE__, __func__, __VA_ARGS__)
#else
#define debug_print(fmt, ...) \
    do {} while(0)
#endif

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT macro is useful to return an error code from a function with a debug message.
 *        Example usage:
 *           M_EXIT(ERR_INVALID_ARGUMENT, "unable to do something decent with value %lu", i);
 */
#define M_EXIT(error_code, fmt, ...)  \
    do { \
        debug_print(fmt, __VA_ARGS__); \
        return error_code; \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_ERR_NOMSG macro is a kind of M_EXIT, indicating (in debug mode)
 *        only the error message corresponding to the error code.
 *        Example usage:
 *            M_EXIT_ERR_NOMSG(ERR_INVALID_ARGUMENT);
 */
#define M_EXIT_ERR_NOMSG(error_code)   \
    M_EXIT(error_code, "error: %s", ERR_MESSAGES[error_code - ERR_NONE])

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR_DO_SOMETHING macro is a shortcut for M_EXIT_ERR on some call,
 *        which does something before exiting;
 *        i.e. if call's return value is different from ERR_NONE, first
 *        executing some code (typically to dis-allocated resources)
 *        and then displaying the call which causes the error before exiting.
 *        Example usage:
 *            M_EXIT_IF_ERR_DO_SOMETHING(foo(a, b), free(p));
 *
 *
 */
#define M_EXIT_IF_ERR_DO_SOMETHING(call_one, call_two) \
    do { \
        const error_code retVal = call_one; \
        if (retVal != ERR_NONE) { \
            call_two; \
            M_EXIT_ERR(retVal, ", when calling: %s", #call_one); \
        } \
    } while(0)

#define GROUP_CALLS(call_one, call_two) \
    do {                                \
        call_one;                       \
        call_two;                       \
    } while(0)

#define FREE(ptr)        \
    do {                 \
        free((ptr));     \
        (ptr) = NULL;    \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_ERR macro is a kind of M_EXIT, indicating (in debug mode)
 *        the error message corresponding to the error code, followed
 *        by the message passed in argument.

 *        Example usage:
 *            M_EXIT_ERR(ERR_INVALID_ARGUMENT, "unable to generate from %lu", i);
 */
#define M_EXIT_ERR(error_code, fmt, ...)   \
    M_EXIT(error_code, "error: %s" fmt, ERR_MESSAGES[error_code - ERR_NONE], __VA_ARGS__)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF macro calls M_EXIT_ERR only if provided test is true.
 *        Example usage:
 *            M_EXIT_IF(i > 3, ERR_INVALID_ARGUMENT, "third argument value (%lu) is too high (> 3)", i);
 */
#define M_EXIT_IF(test, error_code, fmt, ...)   \
    do { \
        if (test) { \
            M_EXIT_ERR(error_code, fmt, __VA_ARGS__); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR_MSG macro is a shortcut for M_EXIT_IF testing if some error occured,
 *        i.e. if error `ret` is different from ERR_NONE, and displaying some explaination.
 *        Without explaination, see M_EXIT_IF_ERR below.
 *        Example usage:
 *            M_EXIT_IF_ERR_MSG(foo(a, b), "calling foo()");
 */
#define M_EXIT_IF_ERR_MSG(ret, name)   \
   do { \
        const error_code retVal = ret; \
        M_EXIT_IF(retVal != ERR_NONE, retVal, "%s", name);   \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR macro is like M_EXIT_IF_ERR_MSG but where the message is the full call.
 *        Example usage:
 *            M_EXIT_IF_ERR(foo(a, b));
 */
#define M_EXIT_IF_ERR(ret)   \
  M_EXIT_IF_ERR_MSG(ret, ", when calling: " #ret)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_NULL macro is usefull to warn (and stop) when NULL pointers are detected.
 *        size is typically the allocation size.
 *        Example usage:
 *            M_EXIT_IF_NULL(p = malloc(size), size);
 */
#define M_EXIT_IF_NULL(var, size)   \
    M_EXIT_IF((var) == NULL, ERR_OUT_OF_MEMORY, ", cannot allocate %zu bytes for %s", size, #var)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TOO_LONG macro is usefull to warn (and stop) when a string is more than size characters long.
 *        Example usage:
 *            M_EXIT_IF_TOO_LONG(answer, INPUT_MAX_SIZE);
 */
#define M_EXIT_IF_TOO_LONG(var, size)   \
    M_EXIT_IF(strlen(var) > size, ERR_INVALID_ARGUMENT, ", given %s is larger than %d bytes", #var, size)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TRAILING macro is useful to test whether some trailing character(s) remain(s) in file.
 *        Example usage:
 *            M_EXIT_IF_TRAILING(file);
 */
#define M_EXIT_IF_TRAILING(file)   \
    M_EXIT_IF(getc(file) != '\n', ERR_INVALID_ARGUMENT, ", trailing chars on \"%s\"", #file)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TRAILING_WITH_CODE macro is similar to M_EXIT_IF_TRAILING but allows to
 *        run some code before exiting.
 *        Example usage:
 *           M_EXIT_IF_TRAILING_WITH_CODE(stdin, { free(something); } );
 */
#define M_EXIT_IF_TRAILING_WITH_CODE(file, code)   \
    do { \
        if (getc(file) != '\n') { \
            code; \
            M_EXIT_ERR(ERR_INVALID_ARGUMENT, ", trailing chars on \"%s\"", #file); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE macro is similar to M_EXIT_IF but with two differences:
 *         1) making use of the NEGATION of the test (thus "require")
 *         2) making use of M_EXIT rather than M_EXIT_ERR
 *        Example usage:
 *            M_REQUIRE(i <= 3, ERR_INVALID_ARGUMENT, "input value (%lu) is too high (> 3)", i);
 */
#define M_REQUIRE(test, error_code, fmt, ...)   \
    do { \
        if (!(test)) { \
             M_EXIT(error_code, fmt, __VA_ARGS__); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQ is simply a shorthand of M_REQUIRE
 */
#define M_REQ(test, error_code, fmt) \
    M_REQUIRE((test), (error_code), (fmt), NULL)

// ----------------------------------------------------------------------
/**
 * @brief M_REQ_CLEAN is similar to M_REQUIRE, although it adds the functionality of
 *        freeing some resources before exiting
 *
 *        M_EXIT_IF_ERR_DO_SOMETHING exists (for now) and somewhat resembles this macro, but is too rigid for
 *        functions NOT defined in this project, as they do not necessarily return ERR_NONE in case of success.
 *        Consequently, we'll use this macro for standard C and VIPS functions
 *
 * @param n = the number of __VA_ARGS__
 */
#define M_REQ_CLEAN(test, error_code, fmt, n, ...) \
    do {                                           \
        if (!(test)) {                             \
             (free_list)((n), __VA_ARGS__);        \
             M_EXIT(error_code, fmt, __VA_ARGS__); \
        }                                          \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NO_ERR macro is a M_REQUIRE test on a return.
 *        This might be useful to return an error if one occured.
 *        Example usage:
 *            M_REQUIRE_NO_ERR(ERR_INVALID_ARGUMENT);
 */
#define M_REQUIRE_NO_ERR(error_code) \
    M_REQUIRE(error_code == 0, error_code, "%s %s", "Recieved error code: ", ERR_MESSAGES[error_code - ERR_NONE])

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NO_ERRNO macro is a M_REQUIRE test on errno.
 *        This might be useful to change errno into our own error code.
 *        Example usage:
 *            M_REQUIRE_NO_ERRNO(ERR_INVALID_ARGUMENT);
 */
#define M_REQUIRE_NO_ERRNO(error_code) \
    M_REQUIRE(errno == 0, error_code, "%s", strerror(errno))

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL_CUSTOM_ERR macro is usefull to check for non NULL argument.
 *        Example usage:
 *            M_REQUIRE_NON_NULL_CUSTOM_ERR(key, ERR_IO);
 */
#define M_REQUIRE_NON_NULL_CUSTOM_ERR(arg, error_code) \
    M_REQUIRE((arg) != NULL, error_code, "parameter %s is NULL", #arg)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL macro is a shortcut for
 *        M_REQUIRE_NON_NULL_CUSTOM_ERR returning ERR_INVALID_ARGUMENT.
 *        Example usage:
 *            M_REQUIRE_NON_NULL(key);
 */
#define M_REQUIRE_NON_NULL(arg) \
    M_REQUIRE_NON_NULL_CUSTOM_ERR(arg, ERR_INVALID_ARGUMENT)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL_RET_VOID macro simply returns void if arg is null
 */
#define M_REQUIRE_NON_NULL_RET_VOID(arg, fmt) \
    do { if ((arg) == NULL) { debug_print(fmt, NULL); return; } } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_CUSTOM_RET tests some condition and returns some value
 * if the test does not hold
 */
#define M_REQUIRE_CUSTOM_RET(should_be_true, to_ret, call) \
    do { if (!(should_be_true)) { call; return to_ret; } } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_NO_ERR_IF exits a function with ERR_NONE if some condition is valid
 */
#define M_EXIT_NO_ERR_IF(test) \
    do { if ((test)) { return ERR_NONE; } } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_IO_COMMAND executes common IO functions verification, in the most general way possible
 */
#define M_IO_COMMAND(func, obj, nb_el, disk, msg) \
    M_REQ((func)(&(obj), sizeof((obj)), (nb_el), (disk)) == (nb_el), ERR_IO, msg)

// ----------------------------------------------------------------------
/**
 * @brief M_READ specializes M_IO_COMMAND to read one element from a pointer
 */
#define M_READ(obj, src, msg) \
    M_IO_COMMAND(fread, obj, 1, src, msg)

// ----------------------------------------------------------------------
/**
 * @brief M_WRITE specializes M_IO_COMMAND to write N elements to a pointer
 */
#define M_WRITE_N(obj, dest, msg, N) \
    M_IO_COMMAND(fwrite, obj, N, dest, msg)

/**
 * @brief M_WRITE specializes M_IO_COMMAND to write one element to a pointer
 */
#define M_WRITE(obj, dest, msg) \
    M_WRITE_N(obj, dest, msg, 1)

// ======================================================================
/**
* @brief internal error messages. defined in error.c
*
*/
extern
const char* const ERR_MESSAGES[];

#define LOG_DATA(filename, data, msg)                                    \
    do {                                                                 \
         FILE* log = fopen("log/(filename)", w+);                        \
         M_WRITE(data, log, msg)                                         \
        fclose(log);                                                     \
    } while (0)

#define IDENT(x) x
#define XSTR(x) #x
#define STR(x) XSTR(x)
#define PATH(x,y) STR(IDENT(x))IDENT(y)
#define LOG_DIR log/

#define LOG_MSG_DIR(filename, msg)                                      \
     do {                                                            \
     FILE* log = fopen(filename, "ab+");                        \
     fprintf(log, __FILE__ ":%d:%s(): " msg "\n",                    \
       __LINE__, __func__);                             \
    fclose(log);                                                     \
    } while (0)



#define LOG_(filename, msg, ...)                                      \
     do {                                                            \
     FILE* log = fopen(filename, "ab+");                        \
     fprintf(log, __FILE__ ":%d:%s(): " msg "\n",                    \
       __LINE__, __func__, __VA_ARGS__);                             \
    fclose(log);                                                     \
    } while (0)



#define LOG(filename, msg,...)                                      \
     LOG_(PATH(LOG_DIR,filename), msg, __VA_ARGS__)

#define LOG_MSG(filename, msg)                                      \
     LOG_MSG_DIR(PATH(LOG_DIR,filename), msg)

#ifdef __cplusplus
}
#endif
