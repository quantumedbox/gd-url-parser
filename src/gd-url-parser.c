#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <wchar.h>
#include <yuarel.h>
#include <gdnative_api_struct.gen.h>

#define ERROR(msg) api->godot_print_error(msg, __func__, __FILE__, __LINE__)

#define MAX_QUERIES 1024

struct url_parser {
  struct yuarel yuarel_state;
  char *buffer;
  size_t buffer_size;
};

static const godot_gdnative_core_api_struct *api = NULL;
static const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;


void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *p_options) {
  api = p_options->api_struct;

  for (unsigned int i = 0; i < api->num_extensions; i++) {
    if (api->extensions[i]->type == GDNATIVE_EXT_NATIVESCRIPT) {
      nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
    }
  }
}

void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *p_options) {
  (void)p_options;

  api = NULL;
  nativescript_api = NULL;
}


static GDCALLINGCONV void *url_parser_constructor(godot_object *p_instance, void *p_method_data) {
  (void)p_instance;
  (void)p_method_data;
  struct url_parser *user_data = api->godot_alloc(sizeof(struct url_parser));
  memset(user_data, 0, sizeof(struct url_parser));
  return user_data;
}


static GDCALLINGCONV void url_parser_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data) {
  (void)p_instance;
  (void)p_method_data;
  struct url_parser *instance = (struct url_parser *)p_user_data;
  if (instance->buffer) {
    free(instance->buffer);
  }
  api->godot_free(p_user_data);
}


static godot_variant url_parser_parse(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
  (void)p_instance;
  (void)p_method_data;

  struct url_parser *instance = (struct url_parser *)p_user_data;

  godot_variant result;
  api->godot_variant_new_bool(&result, false);

  if (p_num_args != 1) {
    ERROR("Invalid number of parameters passed");
    return result;
  }

  if (api->godot_variant_get_type(p_args[0]) != GODOT_VARIANT_TYPE_STRING) {
    ERROR("Input parameter should be of type String");
    return result;
  }

  godot_string str_variant = api->godot_variant_as_string(p_args[0]);
  godot_char_string ch_str = api->godot_string_ascii(&str_variant);
  size_t buffer_size = api->godot_char_string_length(&ch_str) + 1; // todo: What is the size of each elem? Is it really one byte?

  if (!instance->buffer || (instance->buffer_size < buffer_size)) {
    instance->buffer = realloc(instance->buffer, buffer_size);
    if (!instance->buffer) {
      ERROR("Internal error, something gone really wrong");
      api->godot_char_string_destroy(&ch_str);
      return result;
    }
    instance->buffer_size = buffer_size;
  }

  memcpy(instance->buffer, api->godot_char_string_get_data(&ch_str), buffer_size - 1);
  instance->buffer[buffer_size - 1] = '\0';

  if (yuarel_parse(&instance->yuarel_state, instance->buffer) == -1) {
    ERROR("Could not parse given URL");
    api->godot_char_string_destroy(&ch_str);
    return result;
  }

  api->godot_variant_destroy(&result);
  api->godot_variant_new_bool(&result, true);
  return result;
}


#define CHARSTR_TO_VARIANT(dest, charstr) do { \
  godot_string str = api->godot_string_chars_to_utf8(charstr); \
  api->godot_variant_new_string(dest, &str); \
  api->godot_string_destroy(&str); \
} while (0)


#define IMPL_YUAREL_GETTER(field) \
  static GDCALLINGCONV godot_variant url_parser_get_##field(godot_object *p_instance, void * p_method_data, void *p_user_data) { \
    (void)p_method_data; \
    (void)p_instance; \
    struct url_parser *instance = (struct url_parser *)p_user_data; \
    godot_variant result; \
    CHARSTR_TO_VARIANT(&result, instance->yuarel_state.field); \
    return result; \
  }

IMPL_YUAREL_GETTER(scheme)
IMPL_YUAREL_GETTER(username)
IMPL_YUAREL_GETTER(password)
IMPL_YUAREL_GETTER(host)
IMPL_YUAREL_GETTER(path)
IMPL_YUAREL_GETTER(query)
IMPL_YUAREL_GETTER(fragment)


static GDCALLINGCONV godot_variant url_parser_get_port(godot_object *p_instance, void * p_method_data, void *p_user_data) {
  (void)p_method_data;
  (void)p_instance;
  struct url_parser *instance = (struct url_parser *)p_user_data;
  godot_string str = api->godot_string_num_int64(instance->yuarel_state.port, (godot_int)10);
  godot_variant result;
  api->godot_variant_new_string(&result, &str);
  api->godot_string_destroy(&str);
  return result;
}


// todo: Make sure that `url_parser_parse` was called before
static GDCALLINGCONV godot_variant url_parser_parse_query(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
  (void)p_method_data;
  (void)p_instance;
  struct url_parser *instance = (struct url_parser *)p_user_data;

  if (p_num_args > 1) {
    ERROR("Invalid number of parameters passed");
    goto RETURN_NIL;
  }

  char query_separator = '&';

  if (p_num_args == 1) {
    if (api->godot_variant_get_type(p_args[0]) != GODOT_VARIANT_TYPE_STRING) {
      ERROR("Query separator should be of type String");
      goto RETURN_NIL;
    }
    godot_string str_variant = api->godot_variant_as_string(p_args[0]);
    godot_char_string sep_str = api->godot_string_ascii(&str_variant);
    if (api->godot_char_string_length(&sep_str) != 1) {
      ERROR("Query string should be of size 1");
      api->godot_char_string_destroy(&sep_str);
      goto RETURN_NIL;
    }
    query_separator = api->godot_char_string_get_data(&sep_str)[0];
    api->godot_char_string_destroy(&sep_str);
  }

  struct yuarel_param quary_params[MAX_QUERIES];
  size_t query_count = yuarel_parse_query(instance->yuarel_state.query, query_separator, quary_params, MAX_QUERIES);

  godot_array res_arr;

  api->godot_array_new(&res_arr);
  for (size_t i = 0; i < query_count; i++) {
    // todo: Really convoluted and so much copying and allocating >_<
    godot_array pair;
    api->godot_array_new(&pair);
    godot_variant intermediate;
    CHARSTR_TO_VARIANT(&intermediate, quary_params[i].key);
    api->godot_array_push_back(&pair, &intermediate);
    api->godot_variant_destroy(&intermediate);
    CHARSTR_TO_VARIANT(&intermediate, quary_params[i].val);
    api->godot_array_push_back(&pair, &intermediate);
    api->godot_variant_destroy(&intermediate);
    api->godot_variant_new_array(&intermediate, &pair);
    api->godot_array_push_back(&res_arr, &intermediate);
    api->godot_variant_destroy(&intermediate);
    api->godot_array_destroy(&pair);
  }
  godot_variant result;
  api->godot_variant_new_array(&result, &res_arr);
  api->godot_array_destroy(&res_arr);
  return result;
RETURN_NIL: {
    godot_variant result;
    api->godot_variant_new_nil(&result);
    return result;
  }
}


void GDN_EXPORT godot_nativescript_init(void *p_handle) {
  godot_instance_create_func create = { NULL, NULL, NULL };
  create.create_func = &url_parser_constructor;

  godot_instance_destroy_func destroy = { NULL, NULL, NULL };
  destroy.destroy_func = &url_parser_destructor;

  nativescript_api->godot_nativescript_register_class(p_handle, "URLParser", "Node", create, destroy);

  godot_method_attributes basic_method_attribs = { .rpc_type = GODOT_METHOD_RPC_MODE_DISABLED };

  godot_instance_method parse_method = { NULL, NULL, NULL };
  parse_method.method = &url_parser_parse;
  nativescript_api->godot_nativescript_register_method(p_handle, "URLParser", "parse", basic_method_attribs, parse_method);

  godot_property_attributes string_property_attribs = {
    .rset_type = GODOT_METHOD_RPC_MODE_DISABLED,
    .type = GODOT_VARIANT_TYPE_STRING,
    .hint = GODOT_PROPERTY_HINT_TYPE_STRING,
    .usage = GODOT_PROPERTY_USAGE_DEFAULT,
  };

  godot_property_set_func no_setter = { NULL, NULL, NULL };

  #define STRING_GETTER(field) do { \
    godot_property_get_func get_func = { NULL, NULL, NULL }; \
    get_func.get_func = &url_parser_get_##field; \
    nativescript_api->godot_nativescript_register_property(p_handle, "URLParser", #field, &string_property_attribs, no_setter, get_func); \
  } while (0)

  STRING_GETTER(scheme);
  STRING_GETTER(username);
  STRING_GETTER(password);
  STRING_GETTER(host);
  STRING_GETTER(port);
  STRING_GETTER(path);
  STRING_GETTER(query);
  STRING_GETTER(fragment);

  godot_instance_method get_query_parsed = { NULL, NULL, NULL };
  get_query_parsed.method = &url_parser_parse_query;
  nativescript_api->godot_nativescript_register_method(p_handle, "URLParser", "parse_query", basic_method_attribs, get_query_parsed);
}
