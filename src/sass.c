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
    zend_object zo;
    int style;
    char* include_paths;
    char* image_path;
} sass_object;

zend_class_entry *sass_ce;

void sass_free_storage(void *object TSRMLS_DC)
{
    sass_object *obj = (sass_object *)object;
    if (obj->include_paths != NULL)
        efree(obj->include_paths);
    if (obj->image_path != NULL)
        efree(obj->image_path);

    zend_hash_destroy(obj->zo.properties);
    FREE_HASHTABLE(obj->zo.properties);

    efree(obj);
}

zend_object_value sass_create_handler(zend_class_entry *type TSRMLS_DC)
{
    zval *tmp;
    zend_object_value retval;

    sass_object *obj = (sass_object *)emalloc(sizeof(sass_object));
    memset(obj, 0, sizeof(sass_object));

    obj->zo.ce = type;

    ALLOC_HASHTABLE(obj->zo.properties);
    zend_hash_init(obj->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID > 50399
    object_properties_init(&(obj->zo), type);
#endif

    retval.handle = zend_objects_store_put(obj, NULL,
        sass_free_storage, NULL TSRMLS_CC);
    retval.handlers = &sass_handlers;

    return retval;
}

PHP_METHOD(Sass, __construct)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_NULL();
    }

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    obj->style = SASS_STYLE_NESTED;
    obj->include_paths = NULL;
    obj->image_path = NULL;

}

/**
 * $sass->parse(string $source, [  ]);
 *
 * Parse a string of Sass; a basic input -> output affair.
 */
PHP_METHOD(Sass, compile)
{

    sass_object *this = (sass_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

    // Define our parameters as local variables
    char *source;
    int source_len;

    // Use zend_parse_parameters() to grab our source from the function call
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &source, &source_len) == FAILURE){
        RETURN_FALSE;
    }

    // Create a new sass_context
    struct sass_context* context = sass_new_context();

    context->options.include_paths = this->include_paths != NULL ? this->include_paths : "";
    context->options.image_path = this->include_paths != NULL ? this->image_path : "";
    context->options.output_style = this->style;

    // "Hand over the source, buddy!"
    // "Which one, béchamel or arrabbiata?"
    context->source_string = source;

    // Compile it!
    sass_compile(context);

    // Check the context for any errors...
    if (context->error_status)
    {
        zend_throw_exception(sass_exception_ce, trim(context->error_message), 0 TSRMLS_CC);
    }

    // Do we have an output?
    else if (context->output_string)
    {
        // Send it over to PHP.
        RETURN_STRING(context->output_string, 1);
    }

    // There's been a major issue
    else
    {
        zend_throw_exception(sass_exception_ce, "Unknown Error", 0 TSRMLS_CC);
    }

    // Over and out.
    sass_free_context(context);
}

/**
 * $sass->parse_file(string $file_name);
 *
 * Parse a whole file FULL of Sass and return the CSS output
 */
PHP_METHOD(Sass, compile_file)
{
    sass_object *this = (sass_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

    // We need a file name and a length
    char *file;
    int file_len;

    // Grab the file name from the function
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    // First, do a little checking of our own. Does the file exist?
    if( access( file, F_OK ) == -1 )
    {
        char err[200];
        sprintf(err, "File %s could not be found", file);

        zend_throw_exception(sass_exception_ce, err, 0 TSRMLS_CC);
        return;
    }

    // Create a new sass_file_context
    struct sass_file_context* context = sass_new_file_context();

    context->options.include_paths = this->include_paths != NULL ? this->include_paths : "";
    context->options.image_path = this->include_paths != NULL ? this->image_path : "";
    context->options.output_style = this->style;

    context->input_path = file;

    // Compile it!
    sass_compile_file(context);

    // Check the context for any errors...
    if (context->error_status)
    {
        zend_throw_exception(sass_exception_ce, trim(context->error_message), 0 TSRMLS_CC);
    }

    // Do we have an output?
    else if (context->output_string)
    {
        // Send it over to PHP.
        RETURN_STRING(context->output_string, 1);
    }

    // There's been a major issue
    else
    {
        zend_throw_exception(sass_exception_ce, "Unknown Error", 0 TSRMLS_CC);
    }

    // Over and out.
    sass_free_file_context(context);
}

PHP_METHOD(Sass, getStyle)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    RETURN_LONG(obj->style);
}

PHP_METHOD(Sass, setStyle)
{
    zval *this = getThis();

    long new_style;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &new_style) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    obj->style = new_style;

    RETURN_NULL();
}

PHP_METHOD(Sass, getIncludePath)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    if (obj->include_paths == NULL) RETURN_STRING("", 1)
    RETURN_STRING(obj->include_paths, 1)
}

PHP_METHOD(Sass, setIncludePath)
{
    zval *this = getThis();

    char *path;
    int path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
        RETURN_FALSE;

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    if (obj->include_paths != NULL)
        efree(obj->include_paths);
    obj->include_paths = estrndup(path, path_len);

    RETURN_NULL();
}

PHP_METHOD(Sass, getImagePath)
{
    zval *this = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "", NULL) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    if (obj->image_path == NULL) RETURN_STRING("", 1)
    RETURN_STRING(obj->image_path, 1)
}

PHP_METHOD(Sass, setImagePath)
{
    zval *this = getThis();

    char *path;
    int path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
        RETURN_FALSE;

    sass_object *obj = (sass_object *)zend_object_store_get_object(this TSRMLS_CC);
    if (obj->image_path != NULL)
        efree(obj->image_path);
    obj->image_path = estrndup(path, path_len);

    RETURN_NULL();
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

zend_function_entry sass_methods[] = {
    PHP_ME(Sass,  __construct,     NULL,  ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Sass,  compile,         NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  compile_file,    NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getStyle,        NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setStyle,        NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getIncludePath,  NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setIncludePath,  NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  getImagePath,    NULL,  ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setImagePath,    NULL,  ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


static PHP_MINIT_FUNCTION(sass)
{
    zend_class_entry ce;
    zend_class_entry exception_ce;

    INIT_CLASS_ENTRY(ce, "Sass", sass_methods);

    sass_ce = zend_register_internal_class(&ce TSRMLS_CC);
    sass_ce->create_object = sass_create_handler;

    memcpy(&sass_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    sass_handlers.clone_obj = NULL;

    INIT_CLASS_ENTRY(exception_ce, "SassException", NULL);
    sass_exception_ce = zend_register_internal_class_ex(&exception_ce, sass_get_exception_base(TSRMLS_C), NULL TSRMLS_CC);

    #define REGISTER_SASS_CLASS_CONST_LONG(name, value) zend_declare_class_constant_long(sass_ce, ZEND_STRS( #name ) - 1, value TSRMLS_CC)

    REGISTER_SASS_CLASS_CONST_LONG(STYLE_NESTED, SASS_OUTPUT_NESTED);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_EXPANDED, SASS_OUTPUT_EXPANDED);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_COMPACT, SASS_OUTPUT_COMPACT);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_COMPRESSED, SASS_OUTPUT_COMPRESSED);
    REGISTER_SASS_CLASS_CONST_LONG(STYLE_FORMATTED, SASS_OUTPUT_FORMATTED);

    REGISTER_STRING_CONSTANT("SASS_FLAVOR", SASS_FLAVOR, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}

static PHP_MINFO_FUNCTION(sass)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "sass support", "enabled");
    php_info_print_table_row(2, "version", SASS_VERSION);
    php_info_print_table_row(2, "flavor", SASS_FLAVOR);
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
