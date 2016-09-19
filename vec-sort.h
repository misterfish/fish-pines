// ------ ({ }) = GCC extension.

#define vec_to_ary(a, s, v, type) ({ \
    for (size_t i = 0; i < s; i++) { \
        a[i] = (type) vec_get(v, i); \
    } \
})

#define ary_to_vec(a, s, v, type) ({ \
    vec *w = vec_new(); \
    type *b = (type *)a; \
    for (size_t i = 0; i < s; i++) { \
        vec_add(w, b[i]); \
    } \
    w; \
})

#define vec_sort(v, type, cmp) ({ \
    size_t size = vec_size(v); \
    type *a = calloc(size, sizeof(type)); \
    vec_to_ary(a, size, v, type); \
    qsort(a, size, sizeof(type), cmp); \
    vec *sorted = ary_to_vec(a, size, v, type); \
    free(a); \
    sorted; \
})
