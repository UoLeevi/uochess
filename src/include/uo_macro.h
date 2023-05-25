#ifndef UO_MACRO_H
#define UO_MACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get string literal corresponding a token
 *
 */
#define uo_nameof(n) \
    UO__NAMEOF(n)

/**
 * @brief create a variable name that is unlikely to collide with macro arguments
 * 
 */
#define uo_var(ident) \
    uo_cat(ident, __LINE__)

/**
 * @brief concatenate tokens
 * 
 */
#define uo_cat(x, y) \
    UO_CAT(x, y)


#define uo_strlen(s) (sizeof(s)/sizeof(s[0]) - 1)

#define UO__NAMEOF(n) #n

#define UO_CAT(x, y) x ## y

#ifdef __cplusplus
}
#endif

#endif
