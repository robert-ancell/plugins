// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "include/path_provider/path_provider_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

// See path_provider.dart for documentation.
const char kChannelName[] = "plugins.flutter.io/path_provider";
const char kNoAppIdError[] = "No Application Id Set";
const char kGetTemporaryDirectoryMethod[] = "getTemporaryDirectory";
const char kGetApplicationSupportDirectoryMethod[] = "getApplicationSupportDirectory";
const char kGetApplicationDocumentsDirectoryMethod[] = "getApplicationDocumentsDirectory";

struct _FlPathProviderPlugin {
  GObject parent_instance;

  // Connection to Flutter engine.
  FlMethodChannel* channel;
};

G_DEFINE_TYPE(FlPathProviderPlugin, fl_path_provider_plugin, g_object_get_type())

static FlMethodResponse* get_temporary_directory() {
   g_autoptr(FlValue) result = fl_value_new_string(g_get_tmp_dir());
   return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static FlMethodResponse* get_application_support_directory() {
   GApplication *app = g_application_get_default();
   if (app == nullptr) {
     return FL_METHOD_RESPONSE(fl_method_error_response_new(kNoAppIdError, nullptr, nullptr));
   }
   const gchar *app_id = g_application_get_application_id(app);
   if (app_id == nullptr) {
     return FL_METHOD_RESPONSE(fl_method_error_response_new(kNoAppIdError, nullptr, nullptr));
   }

   g_autofree gchar *path = g_build_filename(g_get_user_data_dir(), app_id, nullptr);
   g_autoptr(FlValue) result = fl_value_new_string(path);
   return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static FlMethodResponse* get_application_documents_directory() {
   g_autoptr(FlValue) result = fl_value_new_string(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));
   return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

// Called when a method call is received from Flutter.
static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);

  g_autoptr(FlMethodResponse) response = nullptr;
  if (strcmp(method, getTemporaryDirectory) == 0) {
    response = get_temporary_directory();
  } else if (strcmp(method, getApplicationSupportDirectory) == 0) {
    response = get_application_support_directory();
  } else if (strcmp(method, getApplicationDocumentsDirectory) == 0) {     
    response = get_application_documents_directory();
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  g_autoptr(GError) error = nullptr;
  if (!fl_method_call_respond(method_call, response, &error))
    g_warning("Failed to send method call response: %s", error->message);
}

static void fl_path_provider_plugin_dispose(GObject* object) {
  FlPathProviderPlugin* self = FL_PATH_PROVIDER_PLUGIN(object);

  g_clear_object(&self->channel);

  G_OBJECT_CLASS(fl_path_provider_plugin_parent_class)->dispose(object);
}

static void fl_path_provider_plugin_class_init(FlPathProviderPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_path_provider_plugin_dispose;
}

static void fl_path_provider_plugin_init(FlPathProviderPlugin* self) {}

FlPathProviderPlugin* fl_path_provider_plugin_new(FlPluginRegistrar* registrar) {
  FlPathProviderPlugin* self = FL_PATH_PROVIDER_PLUGIN(
      g_object_new(fl_path_provider_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  self->channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            kChannelName, FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(self->channel, method_call_cb,
                                            g_object_ref(self), g_object_unref);

  return self;
}

void path_provider_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  FlPathProviderPlugin* plugin = fl_path_provider_plugin_new(registrar);
  g_object_unref(plugin);
}
