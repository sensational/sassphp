/**
 * Sass
 * PHP bindings to libsass - fast, native Sass parsing in PHP!
 *
 * https://github.com/jamierumbelow/sassphp
 * Copyright (c)2012 Jamie Rumbelow <http://jamierumbelow.net>
 *
 * Fork updated and maintained by https://github.com/pilif
 */

#include <stdio.h>

#include "php_sass.h"
#include "utilities.h"

/* --------------------------------------------------------------
 * Sass
 * ------------------------------------------------------------ */

zend_object_handlers sass_handlers;

typedef struct sass_object {
    int style;
    char* include_paths;
    long precision;
    bool comments;
    char* map_path;
    bool omit_map_url;
    bool map_embed;
    bool map_contents;
    char* map_root;
    zend_object zo;
} sass_object;

static inline sass_object *sass_object_fetch_object(zend_object *obj) {
    return (sass_object *)((char *)(obj) - XtOffsetOf(sass_object, zo));
}
#define Z_SASS_P(zv)  sass_object_fetch_object(Z_OBJ_P((zv)))

zend_class_entry *sass_ce;

void sass_free_storage(void *object TSRMLS_DC)
{
    sass_object *obj = (sass_object *)object;
    if (obj->include_paths != NULL)
        efree(obj->include_paths);

    if (obj->map_path != NULL)
        efree(obj->map_path);
    if (obj->map_root != NULL)
        efree(obj->map_root);

    zend_hash_destroy(obj->zo.properties);
    FREE_HASHTABLE(obj->zo.properties);

    efree(obj);
}

zend_object *sass_create_handler(zend_class_entry *type)
{
    sass_object *obj = ecalloc(1,
        sizeof(sass_object) +
        zend_object_properties_size(type));

    zend_object_std_init(&obj->zo, type);
    object_properties_init(&obj->zo, type);
    obj->zo.handlers = &sass_handlers;

    return &obj->zo;
}

PHP_METHOD(Sass, __construct)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_NULL();
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(this));
    obj->style = SASS_STYLE_NESTED;
    obj->include_paths = NULL;
    obj->precision = 5;
    obj->map_path = NULL;
    obj->map_root = NULL;
    obj->comments = false;
    obj->map_embed = false;
    obj->map_contents = false;
    obj->omit_map_url = true;
}


void set_options(sass_object *this, struct Sass_Context *ctx)
{
    struct Sass_Options* opts = sass_context_get_options(ctx);

    sass_option_set_precision(opts, this->precision);
    sass_option_set_output_style(opts, this->style);
    if (this->include_paths != NULL) {
        sass_option_set_include_path(opts, this->include_paths);
    }
    sass_option_set_source_comments(opts, this->comments);
    if (this->comments) {
        sass_option_set_omit_source_map_url(opts, false);
    }
    sass_option_set_source_map_embed(opts, this->map_embed);
    sass_option_set_source_map_contents(opts, this->map_contents);
    if (this->map_path != NULL) {
    sass_option_set_source_map_file(opts, this->map_path);
    sass_option_set_omit_source_map_url(opts, false);
    sass_option_set_source_map_contents(opts, true);
    }
    if (this->map_root != NULL) {
    sass_option_set_source_map_root(opts, this->map_root);
    }
}

/**
 * $sass->parse(string $source, [  ]);
 *
 * Parse a string of Sass; a basic input -> output affair.
 */
PHP_METHOD(Sass, compile)
{

    sass_object *this = sass_object_fetch_object(Z_OBJ_P(getThis()));

    // Define our parameters as local variables
    char *source;
    size_t source_len;

    // Use zend_parse_parameters() to grab our source from the function call
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &source, &source_len) == FAILURE){
        RETURN_FALSE;
    }

    // Create a new sass_context
    struct Sass_Data_Context* data_context = sass_make_data_context(strdup(source));
    struct Sass_Context* ctx = sass_data_context_get_context(data_context);

    set_options(this, ctx);

    int status = sass_compile_data_context(data_context);

    // Check the context for any errors...
    if (status != 0)
    {
        zend_throw_exception(sass_exception_ce, sass_context_get_error_message(ctx), 0 TSRMLS_CC);
    }
    else
    {
        RETVAL_STRING(sass_context_get_output_string(ctx));
    }

    sass_delete_data_context(data_context);
}

/**
 * $sass->parse_file(string $file_name);
 *
 * Parse a whole file FULL of Sass and return the CSS output
 */
PHP_METHOD(Sass, compileFile)
{
    sass_object *this = sass_object_fetch_object(Z_OBJ_P(getThis()));

    if (this->map_path != NULL){
        array_init(return_value);
    }

    // We need a file name and a length
    char *file;
    size_t file_len;

    // Grab the file name from the function
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    // First, do a little checking of our own. Does the file exist?
    if( access( file, F_OK ) == -1 )
    {
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "File %s could not be found", file);
        RETURN_FALSE;
    }

    // Create a new sass_file_context
    struct Sass_File_Context* file_ctx = sass_make_file_context(file);
    struct Sass_Context* ctx = sass_file_context_get_context(file_ctx);

    set_options(this, ctx);

    int status = sass_compile_file_context(file_ctx);

    // Check the context for any errors...
    if (status != 0)
    {
        zend_throw_exception(sass_exception_ce, sass_context_get_error_message(ctx), 0 TSRMLS_CC);
    }
    else
    {

        if (this->map_path != NULL ) {
            // Send it over to PHP.
            add_next_index_string(return_value, sass_context_get_output_string(ctx));
        } else {
            RETVAL_STRING(sass_context_get_output_string(ctx));
        }

        // Do we have source maps to go?
        if (this->map_path != NULL)
        {
            // Send it over to PHP.
            add_next_index_string(return_value, sass_context_get_source_map_string(ctx));
        }
    }

    sass_delete_file_context(file_ctx);
}

PHP_METHOD(Sass, getStyle)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    RETURN_LONG(obj->style);
}

PHP_METHOD(Sass, setStyle)
{
    long new_style;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &new_style) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));;
    obj->style = new_style;

    RETURN_NULL();
}

PHP_METHOD(Sass, getIncludePath)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    if (obj->include_paths == NULL) RETURN_STRING("")
    RETURN_STRING(obj->include_paths)
}

PHP_METHOD(Sass, setIncludePath)
{
    char *path;
    size_t path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
        RETURN_FALSE;

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    if (obj->include_paths != NULL)
        efree(obj->include_paths);
    obj->include_paths = estrndup(path, path_len);

    RETURN_NULL();
}

PHP_METHOD(Sass, getMapPath)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));

    if (obj->map_path == NULL) RETURN_STRING("")
    RETURN_STRING(obj->map_path)
}

PHP_METHOD(Sass, setMapPath)
{

    char *path;
    size_t path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
        RETURN_FALSE;

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    if (obj->map_path != NULL)
        efree(obj->map_path);
    obj->map_path = estrndup(path, path_len);

    RETURN_NULL();
}


PHP_METHOD(Sass, getPrecision)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    RETURN_LONG(obj->precision);
}

PHP_METHOD(Sass, setPrecision)
{
    long new_precision;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &new_precision) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    obj->precision = new_precision;

    RETURN_NULL();
}

PHP_METHOD(Sass, getEmbed)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    RETURN_LONG(obj->map_embed);
}

PHP_METHOD(Sass, setEmbed)
{
    zend_bool new_map_embed;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &new_map_embed) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    obj->map_embed = new_map_embed;

    RETURN_NULL();
}


PHP_METHOD(Sass, getComments)
{

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    RETURN_LONG(obj->comments);
}

PHP_METHOD(Sass, setComments)
{

    zend_bool new_comments;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &new_comments) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = sass_object_fetch_object(Z_OBJ_P(getThis()));
    obj->comments = new_comments;

    RETURN_NULL();
}


PHP_METHOD(Sass, getLibraryVersion)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    RETURN_STRING(libsass_version())
}
/* --------------------------------------------------------------
 * EXCEPTION HANDLING
 * ------------------------------------------------------------ */

zend_class_entry *sass_get_exception_base(TSRMLS_D)
{
    return zend_exception_get_default(TSRMLS_C);
}

/* --------------------------------------------------------------
 * PHP EXTENSION INFRASTRUCTURE
 * ------------------------------------------------------------ */

ZEND_BEGIN_ARG_INFO(arginfo_sass_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_compile, 0, 0, 1)
    ZEND_ARG_INFO(0, sass_string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_compileFile, 0, 0, 1)
    ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setStyle, 0, 0, 1)
    ZEND_ARG_INFO(0, style)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setIncludePath, 0, 0, 1)
    ZEND_ARG_INFO(0, include_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setPrecision, 0, 0, 1)
    ZEND_ARG_INFO(0, precision)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setComments, 0, 0, 1)
    ZEND_ARG_INFO(0, comments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setEmbed, 0, 0, 1)
    ZEND_ARG_INFO(0, map_embed)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setMapPath, 0, 0, 1)
    ZEND_ARG_INFO(0, map_path)
ZEND_END_ARG_INFO()

zend_function_entry sass_methods[] = {
    PHP_ME(Sass,  __construct,       arginfo_sass_void,           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Sass,  compile,           arginfo_sass_compile,        ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  compileFile,       arginfo_sass_compileFile,    ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getStyle,          arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setStyle,          arginfo_sass_setStyle,       ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getIncludePath,    arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setIncludePath,    arginfo_sass_setIncludePath, ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getPrecision,      arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setPrecision,      arginfo_sass_setPrecision,   ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getComments,       arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setComments,       arginfo_sass_setComments,    ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getEmbed,          arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setEmbed,          arginfo_sass_setComments,    ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getMapPath,        arginfo_sass_void,           ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setMapPath,        arginfo_sass_setMapPath,     ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getLibraryVersion, arginfo_sass_void,           ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_MALIAS(Sass, compile_file, compileFile, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


static PHP_MINIT_FUNCTION(sass)
{
    zend_class_entry ce;
    zend_class_entry exception_ce;

    INIT_CLASS_ENTRY(ce, "Sass", sass_methods);
    ce.create_object = sass_create_handler;
    sass_ce = zend_register_internal_class(&ce TSRMLS_CC);

    memcpy(&sass_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    sass_handlers.clone_obj = NULL;

    INIT_CLASS_ENTRY(exception_ce, "SassException", NULL);
    sass_exception_ce = zend_register_internal_class_ex(&exception_ce, sass_get_exception_base(TSRMLS_C));

    #define REGISTER_SASS_CLASS_CONST_LONG(name, value) zend_declare_class_constant_long(sass_ce, ZEND_STRS( #name ) - 1, value TSRMLS_CC)

    REGISTER_SASS_CLASS_CONST_LONG(STYLE_NESTED, SASS_STYLE_NESTED);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_EXPANDED, SASS_STYLE_EXPANDED);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_COMPACT, SASS_STYLE_COMPACT);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_COMPRESSED, SASS_STYLE_COMPRESSED);

    REGISTER_STRING_CONSTANT("SASS_FLAVOR", SASS_FLAVOR, CONST_CS | CONST_PERSISTENT);


    return SUCCESS;
}

static PHP_MINFO_FUNCTION(sass)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "sass support", "enabled");
    php_info_print_table_row(2, "version", SASS_VERSION);
    php_info_print_table_row(2, "flavor", SASS_FLAVOR);
    php_info_print_table_row(2, "libsass version", libsass_version());
    php_info_print_table_end();
}

static zend_module_entry sass_module_entry = {
    STANDARD_MODULE_HEADER,
    "sass",
    NULL,
    PHP_MINIT(sass),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(sass),
    SASS_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SASS
ZEND_GET_MODULE(sass)
#endif
