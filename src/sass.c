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
    zval importer;
    zval function_table;
    zend_object zo;
} sass_object;

static inline sass_object *sass_object_fetch_object(zend_object *obj) {
    return (sass_object *)((char *)(obj) - XtOffsetOf(sass_object, zo));
}
#define Z_SASS_P(zv)  sass_object_fetch_object(Z_OBJ_P((zv)))

zend_class_entry *sass_ce;

static void sass_free_storage(zend_object *object)
{
    sass_object *obj = sass_object_fetch_object(object);
    if (obj->include_paths != NULL)
        efree(obj->include_paths);

    if (obj->map_path != NULL)
        efree(obj->map_path);
    if (obj->map_root != NULL)
        efree(obj->map_root);

    zval_ptr_dtor(&obj->importer);
    zval_ptr_dtor(&obj->function_table);

    zend_object_std_dtor(object);
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


char *to_c_string(zval *var){
    if (Z_TYPE_P(var) != IS_STRING) {
        convert_to_string(var);
    }
    return Z_STRVAL_P(var);
}

Sass_Import_Entry array_to_import(zval* val){
    if (Z_TYPE_P(val) != IS_ARRAY){
        return NULL;
    }

    int len = zend_hash_num_elements(Z_ARRVAL_P(val));
    if (len < 1){
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Need at least redirected path");
        return NULL;
    }

    char *file = 0;
    zval *temp = zend_hash_index_find(Z_ARRVAL_P(val), 0);
    if (temp != NULL && !Z_ISUNDEF_P(temp) && Z_TYPE_P(temp) != IS_NULL){
        file = sass_copy_c_string(to_c_string(temp));
    }

    char *content = 0;
    temp = zend_hash_index_find(Z_ARRVAL_P(val), 1);
    if (temp != NULL && !Z_ISUNDEF_P(temp) && Z_TYPE_P(temp) != IS_NULL){
        content = sass_copy_c_string(to_c_string(temp));
    }

    char *map = 0;
    if (len >= 3){
        temp = zend_hash_index_find(Z_ARRVAL_P(val), 2);
        if (temp != NULL && !Z_ISUNDEF_P(temp) && Z_TYPE_P(temp) != IS_NULL){
            map = sass_copy_c_string(to_c_string(temp));
        }
    }

    return sass_make_import_entry(
        file, content, map
    );
}

Sass_Import_List sass_importer(const char* path, Sass_Importer_Entry cb, struct Sass_Compiler* comp){

    sass_object *obj = (sass_object *) sass_importer_get_cookie(cb);
    if (obj == NULL){
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Internal Error: Failed to retrieve object reference");
        return NULL;
    }

    zval cb_args[1];
    zval cb_retval;
    ZVAL_STRING(&cb_args[0], path);

    if (call_user_function_ex(EG(function_table), NULL, &obj->importer, &cb_retval, 1, cb_args, 0, NULL) != SUCCESS || Z_ISUNDEF(cb_retval)) {
        zval_ptr_dtor(&cb_args[0]);
        return NULL;
    }
    zval_ptr_dtor(&cb_args[0]);

    if (Z_TYPE(cb_retval) == IS_NULL){
        zval_ptr_dtor(&cb_retval);
        return NULL;
    }

    if (Z_TYPE(cb_retval) != IS_ARRAY){
        zval_ptr_dtor(&cb_retval);
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Importer callback must return an array");
        return NULL;
    }

    int result_len = zend_hash_num_elements(Z_ARRVAL(cb_retval));
    if (result_len < 1){
        zval_ptr_dtor(&cb_retval);
        return NULL;
    }


    zval *first_element = zend_hash_index_find(Z_ARRVAL(cb_retval), 0);
    if (first_element == NULL){
        zval_ptr_dtor(&cb_retval);
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Importer callback must return an array");
        return NULL;
    }

    Sass_Import_List list;
    if (Z_TYPE_P(first_element) == IS_ARRAY){
        list = sass_make_import_list(result_len);
        int idx = 0;
        zval *element;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(cb_retval), element) {
            if (Z_TYPE_P(element) != IS_ARRAY){
                zval_ptr_dtor(&cb_retval);
                zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Importer callback must return an array");
                return NULL;
            }
            Sass_Import_Entry imp = array_to_import(element);
            if (imp == NULL) return NULL;
            list[idx] = imp;
            idx++;
        } ZEND_HASH_FOREACH_END();
    }else{
        list = sass_make_import_list(1);
        list[0] = array_to_import(&cb_retval);
    }

    zval_ptr_dtor(&cb_retval);
    return list;
}

union Sass_Value* sass_function(const union Sass_Value* s_args, Sass_Function_Entry cb, struct Sass_Compiler* comp)
{
    sass_object *obj = (sass_object *) sass_function_get_cookie(cb);
    if (obj == NULL){
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Internal Error: Failed to retrieve object reference");
        return NULL;
    }

    const char *signature = sass_function_get_signature(cb);

    if (Z_TYPE(obj->function_table) != IS_ARRAY){
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Internal Error: Function table has vanished");
        return NULL;
    }

    zend_string *fname = zend_string_init(signature, strlen(signature), 0);
    zval* callback = zend_hash_find(Z_ARRVAL(obj->function_table), fname);
    zend_string_release(fname);
    if (callback == NULL){
        return sass_make_null();
    }

    if (!zend_is_callable(callback, 0, NULL)) {
        zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Internal Error: value for sig %s lost its callbackyness", ZSTR_VAL(fname));
        return sass_make_null();
    }

    const char *string_value;
    // For now: Stringify all arguments. Later, we should probably marshal
    // this correctly and keep wrappers around native sass types. But for my
    // personal purpose, this is enough.
    if (!sass_value_is_string(s_args)){
        string_value = sass_string_get_value(sass_value_stringify(s_args, false, obj->precision));
    }else{
        string_value = sass_string_get_value(s_args);
    }

    Sass_Import_Entry import = sass_compiler_get_last_import(comp);

    zval path_info;
    array_init(&path_info);
    add_assoc_string(&path_info, "absolute", sass_import_get_abs_path(import));
    add_assoc_string(&path_info, "relative", sass_import_get_imp_path(import));

    zval cb_args[2];
    zval cb_retval;
    ZVAL_STRING(&cb_args[0], string_value);
    cb_args[1] = path_info;

    if (call_user_function_ex(EG(function_table), NULL, callback, &cb_retval, 2, cb_args, 0, NULL) != SUCCESS || Z_ISUNDEF(cb_retval)) {
        zval_ptr_dtor(&cb_args[0]);
        return sass_make_null();
    }
    zval_ptr_dtor(&cb_args[0]);
    zval_ptr_dtor(&cb_args[1]);

    if (Z_TYPE_P(&cb_retval) != IS_STRING) {
        convert_to_string(&cb_retval);
    }
    union Sass_Value *r = sass_make_string(Z_STRVAL_P(&cb_retval));
    zval_ptr_dtor(&cb_retval);
    return r;
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
    ZVAL_UNDEF(&obj->importer);
    ZVAL_UNDEF(&obj->function_table);
}


void set_generic_options(sass_object *this, struct Sass_Context *ctx)
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
        sass_option_set_omit_source_map_url(opts, true);
        sass_option_set_source_map_contents(opts, false);
    }
    if (this->map_root != NULL) {
        sass_option_set_source_map_root(opts, this->map_root);
    }

    if (!Z_ISUNDEF(this->importer)) {
        Sass_Importer_Entry imp = sass_make_importer(sass_importer, 0, this);
        Sass_Importer_List imp_list = sass_make_importer_list(1);
        sass_importer_set_list_entry(imp_list, 0, imp);
        sass_option_set_c_importers(opts, imp_list);
    }

    if (!Z_ISUNDEF(this->function_table)) {
        int function_count = zend_hash_num_elements(Z_ARRVAL(this->function_table));

        Sass_Function_List fn_list = sass_make_function_list(function_count);
        int idx = 0;

        zend_ulong num_key;
        zend_string *string_key;
        zval *val;
        Sass_Function_Entry fn;

        ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(this->function_table), num_key, string_key, val) {
            if (string_key == NULL){
                zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Keys must be function declarations");
                return;
            }
            if (!zend_is_callable(val, 0, NULL)) {
                zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Values must be callables, but value at `%s` isn't", ZSTR_VAL(string_key));
                return;
            }
            fn = sass_make_function(ZSTR_VAL(string_key), sass_function, (void*)this);
            sass_function_set_list_entry(fn_list, idx, fn);
            idx++;
        } ZEND_HASH_FOREACH_END();

        sass_option_set_c_functions(opts, fn_list);
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
    char *source, *input_path = NULL;
    size_t source_len, input_path_len = 0;

    // Use zend_parse_parameters() to grab our source from the function call
    if (zend_parse_parameters_throw(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &source, &source_len, &input_path, &input_path_len) == FAILURE){
        RETURN_FALSE;
    }

    // Create a new sass_context
    struct Sass_Data_Context* data_context = sass_make_data_context(strdup(source));
    struct Sass_Context* ctx = sass_data_context_get_context(data_context);

    set_generic_options(this, ctx);
    if (input_path != NULL){
        struct Sass_Options* opts = sass_context_get_options(ctx);
        sass_option_set_input_path(opts, input_path);
    }

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

    set_generic_options(this, ctx);

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

PHP_METHOD(Sass, setImporter)
{
    zval *importer;
    zend_string *callback_name;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &importer) == FAILURE) {
        RETURN_FALSE;
    }

    sass_object *obj = Z_SASS_P(getThis());

    if (Z_TYPE_P(importer) == IS_NULL){
        if (!Z_ISUNDEF(obj->importer)) {
            zval_ptr_dtor(&obj->importer);
        }
        ZVAL_UNDEF(&obj->importer);
        RETURN_TRUE;
    }

    if (!zend_is_callable(importer, 0, &callback_name)) {
        php_error_docref(NULL, E_WARNING, "%s is not a valid callable", ZSTR_VAL(callback_name));
        zend_string_release(callback_name);
        RETURN_FALSE;
    }

    if (!Z_ISUNDEF(obj->importer)) {
        zval_ptr_dtor(&obj->importer);
        ZVAL_UNDEF(&obj->importer);
    }

    ZVAL_COPY(&obj->importer, importer);
    RETURN_TRUE;
}

PHP_METHOD(Sass, setFunctions)
{
    zval *funcs;

    if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "a!", &funcs) == FAILURE) {
        return;
    }

    sass_object *obj = Z_SASS_P(getThis());

    if (funcs == NULL){
        if (!Z_ISUNDEF(obj->function_table)) {
            zval_ptr_dtor(&obj->function_table);
        }
        ZVAL_UNDEF(&obj->function_table);
        RETURN_TRUE;
    }

    zend_ulong num_key;
    zend_string *string_key;
    zval *val;

    ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(funcs), num_key, string_key, val) {
        if (string_key == NULL){
            zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Keys must be function declarations");
        }
        if (!zend_is_callable(val, 0, NULL)) {
            zend_throw_exception_ex(sass_exception_ce, 0 TSRMLS_CC, "Values must be callables, but value at `%s` isn't", ZSTR_VAL(string_key));
            RETURN_FALSE;
        }
    } ZEND_HASH_FOREACH_END();

    if (!Z_ISUNDEF(obj->function_table)) {
        zval_ptr_dtor(&obj->function_table);
        ZVAL_UNDEF(&obj->function_table);
    }

    ZVAL_COPY(&obj->function_table, funcs);
    RETURN_TRUE;
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
    ZEND_ARG_INFO(0, input_path)
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setImporter, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, importer, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sass_setFunctions, 0, 0, 1)
    ZEND_ARG_ARRAY_INFO(0, function_table, 1)
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
    PHP_ME(Sass,  setImporter,       arginfo_sass_setImporter,    ZEND_ACC_PUBLIC)
    PHP_ME(Sass,  setFunctions,      arginfo_sass_setFunctions,   ZEND_ACC_PUBLIC)
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
    sass_handlers.free_obj = sass_free_storage;
    sass_handlers.offset = XtOffsetOf(sass_object, zo);

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
