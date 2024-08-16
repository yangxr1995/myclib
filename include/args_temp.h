

#ifdef ARGS_DEF_OPTION

#define ARG_DESC(long_name, has_arg, short_name, help_info, var_type, var_name, def_val) \
    {long_name,  has_arg, NULL, short_name},

#elif  defined(ARGS_DEF_STRUCT)

#define ARG_DESC(long_name, has_arg, short_name, help_info, var_type, var_name, def_val) \
    var_type var_name;

#elif  defined(ARGS_DEF_PARSE)

#define ARG_DESC(long_name, _has_arg, short_name, help_info, var_type, var_name, def_val) \
    if (popt->val == short_name) { \
        if (popt->has_arg) { \
            if (strcmp("int", #var_type) == 0) \
                *(int *)&a->var_name = atoi(optarg); \
            else if (strcmp("char *", #var_type) == 0)  \
                *(char **)&a->var_name = strdup(optarg); \
            else \
                assert(0 && "unknow type : " #var_type); \
        } \
        else { \
            *(int *)&a->var_name = 1; \
        } \
        continue; \
    }

#elif  defined(ARGS_DEF_PRINT)

#define ARG_DESC(long_name, has_arg, short_name, help_info, var_type, var_name, def_val) \
    if (strcmp("int", #var_type) == 0) \
        printf("%s : %d\n", #var_name, *(int *)&a->var_name); \
    else if (strcmp("char *", #var_type) == 0)  \
        printf("%s : %s\n", #var_name, *(char **)&a->var_name); \
    else \
        assert(0 && "unknow type : " #var_type); \

#elif  defined(ARGS_DEF_DEFAULT)

#define ARG_DESC(long_name, _has_arg, short_name, help_info, var_type, var_name, def_val) \
    .var_name = def_val,

#elif  defined(ARGS_DEF_HELP)

#define ARG_DESC(long_name, _has_arg, short_name, help_info, var_type, var_name, def_val) \
    printf("--%s -%c %s\t\t\t %s \n", long_name, short_name, _has_arg ? "[args]" : "[no args]", help_info);

#else

#error ARG_DESC not set

#endif


#ifdef ARGS_DEF_OPTION
#undef ARGS_DEF_OPTION
#endif

#ifdef ARGS_DEF_STRUCT
#undef ARGS_DEF_STRUCT
#endif

#ifdef ARGS_DEF_PARSE
#undef ARGS_DEF_PARSE
#endif

#ifdef ARGS_DEF_PRINT
#undef ARGS_DEF_PRINT
#endif

#ifdef ARGS_DEF_DEFAULT
#undef ARGS_DEF_DEFAULT
#endif

#ifdef ARGS_DEF_HELP
#undef ARGS_DEF_HELP
#endif
