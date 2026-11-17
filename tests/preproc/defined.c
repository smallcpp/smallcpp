#if defined(FOO)
no
#else
yes
#endif
#define FOO
#if defined FOO
yes2
#endif
