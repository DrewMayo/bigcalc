#ifndef ERROR_H
#define ERROR_H

#define ENULL -1
#define EINVALID -2
#define EFULL -3
#define EMEM -4

// for checking the value that we are going
// to discard later
#define CHECK_ERROR_DISCARD(fn, expected) \
    do {                                  \
        uint64_t __retval = (fn);         \
        if (__retval != expected) {       \
            goto error;                   \
        }                                 \
    } while (0)

// for checking the error that we are not going to use later
#define CHECK_ERROR(fn, retval, expected) \
    do {                                  \
        retval = (fn);                    \
        if (retval != expected) {         \
            goto error;                   \
        }                                 \
    } while (0)

#define TEST(val)       \
    do {                \
        if (!(val)) {   \
            goto error; \
        }               \
    } while (0)

#endif
