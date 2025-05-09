//
// Created by dingjing on 25-5-8.
//

#include "vfs-limit.h"

#include <sys/stat.h>


#define BREAK_IF_FAIL(x)            if (!(x)) { break; }
#define RETURN_IF_OK(x)             if ((x)) { return; }
#define RETURN_VAL_IF_OK(x, val)    if ((x)) { return val; }
#define BREAK_NULL(x)               if ((x) == NULL) { break; }
#define NOT_NULL_RUN(x,f,...)       G_STMT_START if (x) { f(x, ##__VA_ARGS__); x = NULL; } G_STMT_END
#define G_OBJ_FREE(x)               G_STMT_START if (G_IS_OBJECT(x)) {g_object_unref (G_OBJECT(x)); x = NULL;} G_STMT_END


typedef enum
{
    PROP_0,
    PROP_FILE_PATH,
    PROP_FILE_URI,
    PROP_N
} LimitFileProperty;

struct _LimitFile
{
    GObject                 parent;

    char*                   filePath;
    char*                   fileURI;
};

struct _LimitFileEnum
{
    GFileEnumerator         parent;
    char*                   fileURI;
    char*                   filePath;
    GFileEnumerator*        fileEnumerator;
};

static void limit_file_init                     (LimitFile* self);
static void limit_file_interface_init           (GFileIface* interface);
static void limit_file_class_init               (LimitFileClass* klass);
static void limit_file_get_property             (GObject* object, guint id, GValue* value, GParamSpec* spec);
static void limit_file_set_property             (GObject* object, guint id, const GValue* value, GParamSpec* spec);

static void limit_file_enum_init                (LimitFileEnum* self);
static void limit_file_enum_class_init          (LimitFileEnumClass* klass);
static GFileEnumerator* vfs_file_enum_children  (GFile* file, const char* attribute, GFileQueryInfoFlags flags, GCancellable* cancel, GError** error);

static char gRootDir[PATH_MAX] = {0};

G_DEFINE_TYPE (LimitFileEnum, limit_file_enum, G_TYPE_FILE_ENUMERATOR);
G_DEFINE_TYPE_WITH_CODE (LimitFile, limit_file, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(G_TYPE_FILE, limit_file_interface_init));


static void limit_file_set_path_and_uri             (LimitFile* obj, const char* pu);
static GFile* vfs_lookup                            (GVfs* vfs, const char* uri, gpointer data);
static GFile* vfs_parse_name                        (GVfs* vfs, const char* parseName, gpointer data);

static char*        vfs_get_uri                     (GFile* file);
static char*        vfs_get_path                    (GFile* file);
static char*        vfs_get_uri_schema              (GFile* file);
static char*        vfs_get_basename                (GFile* file);
static gboolean     vfs_is_native                   (GFile* file);
static GFile*       vfs_dup                         (GFile* file);
static GFile*       vfs_get_parent                  (GFile* file);
static guint        vfs_hash                        (GFile* file);
static gboolean     vfs_is_equal                    (GFile* file, GFile* file2);
static gboolean     vfs_has_schema                  (GFile* file, const char* uriSchema);
static GFile*       vfs_resolve_relative_path       (GFile* file, const char* relativePath);
static GFile*       vfs_get_child_for_display_name  (GFile* file, const char* displayName, GError** error);
static GFileInfo*   vfs_file_query_fs_info          (GFile* file, const char* attr, GCancellable* cancel, GError** error);
static GFileInfo*   vfs_file_query_info             (GFile* file, const char* attr, GFileQueryInfoFlags flags, GCancellable* cancel, GError** error);

static GFileInfo*   vfs_file_enum_next_file         (GFileEnumerator *enumerator, GCancellable *cancellable, GError **error);
static gboolean     vfs_file_enum_close             (GFileEnumerator *enumerator, GCancellable *cancellable, GError **error);

static void         file_path_format                (char* filePath);

static GParamSpec* gsBackupFileProperty[PROP_N] = { NULL };


GFile* limit_file_new_for_uri (const gchar* uri)
{
    g_return_val_if_fail(uri && !strncmp(uri, LIMIT_STR, strlen(LIMIT_STR)) && (strlen(uri) > strlen(LIMIT_STR) + 3), NULL);

    LimitFile* self = LIMIT_FILE(g_object_new(LIMIT_FILE_TYPE, "s-path", uri + strlen(LIMIT_STR) + 3, NULL));

    return (GFile*) self;
}

GFile* limit_file_new_for_path (const gchar* path)
{
    g_return_val_if_fail(path && '/' == path[0], NULL);

    LimitFile* self = LIMIT_FILE(g_object_new(LIMIT_FILE_TYPE, "s-path", path, NULL));

    return (GFile*) self;
}

const char* limit_get_root_uri()
{
    return LIMIT_STR":///";
}

const char * limit_get_root_path()
{
    return gRootDir;
}

void limit_file_register()
{
    static gsize init = 0;

    if (g_once_init_enter (&init)) {
        g_vfs_register_uri_scheme(g_vfs_get_default(), LIMIT_STR, vfs_lookup, NULL, NULL, vfs_parse_name, NULL, NULL);
        g_once_init_leave (&init, 1);
    }
}

void limit_set_root_dir(const char * rootDir, gint32 pathLen)
{
    g_return_if_fail(rootDir && pathLen > 0);

    memset(gRootDir, 0, sizeof (gRootDir));
    memcpy(gRootDir, rootDir, pathLen);
    if ('/' != gRootDir[pathLen]) {
        gRootDir[pathLen] = '/';
    }
}

static void limit_file_init (LimitFile* self)
{
    (void) self;
}

static void limit_file_enum_init (LimitFileEnum* self)
{
    (void) self;
    // g_return_if_fail(LIMIT_IS_FILE_ENUM(self));
}

static void limit_file_class_init (LimitFileClass* klass)
{
    GObjectClass* objClass = G_OBJECT_CLASS (klass);

    objClass->get_property = limit_file_get_property;
    objClass->set_property = limit_file_set_property;

    gsBackupFileProperty[PROP_FILE_PATH] = g_param_spec_string ("s-path", "Path", "", NULL, G_PARAM_READWRITE);
    gsBackupFileProperty[PROP_FILE_URI]  = g_param_spec_string ("s-uri", "URI", "", NULL, G_PARAM_READWRITE);

    g_object_class_install_properties(objClass, PROP_N, gsBackupFileProperty);

    limit_file_register();
}

static void limit_file_enum_class_init (LimitFileEnumClass* klass)
{
    g_return_if_fail (LIMIT_IS_FILE_ENUM_CLASS(klass));

    GObjectClass* objClass = G_OBJECT_CLASS (klass);
    GFileEnumeratorClass* enumerator = G_FILE_ENUMERATOR_CLASS (objClass);

    enumerator->next_file       = vfs_file_enum_next_file;
    enumerator->close_fn        = vfs_file_enum_close;
}

static void limit_file_set_property (GObject* object, guint id, const GValue* value, GParamSpec* spec)
{
    g_return_if_fail(LIMIT_IS_FILE(object) && value);

    LimitFile* self = LIMIT_FILE (object);

    switch ((LimitFileProperty) id) {
        case PROP_FILE_PATH:
        case PROP_FILE_URI: {
            limit_file_set_path_and_uri(self, g_value_get_string(value));
            break;
        }
        case PROP_0:
        case PROP_N:
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
            break;
        }
    }
}

static void limit_file_get_property (GObject* object, guint id, GValue* value, GParamSpec* spec)
{
    g_return_if_fail(LIMIT_IS_FILE(object) && value);

    const LimitFile* self = LIMIT_FILE (object);

    switch ((LimitFileProperty) id) {
        case PROP_FILE_PATH: {
            g_value_set_string(value, self->filePath);
            break;
        }
        case PROP_FILE_URI: {
            g_value_set_string(value, self->fileURI);
            break;
        }
        case PROP_0:
        case PROP_N:
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
            break;
        }
    }
}

static void limit_file_set_path_and_uri(LimitFile* obj, const char* pu)
{
    g_return_if_fail (LIMIT_IS_FILE(obj) && pu);

    const int puLen = (int) strlen (pu);
    const int schemaLen = (int) strlen(LIMIT_STR);

    STR_FREE(obj->fileURI);
    STR_FREE(obj->filePath);

    // format
    char* dupPath = NULL;
    if ((puLen - schemaLen >= 3) && (0 == strncmp(pu, LIMIT_STR, schemaLen)) && ('/' == pu[schemaLen + 3])) {
        dupPath = g_strdup(pu + schemaLen + 3);
    }
    else if (puLen > 0 && '/' == pu[0]) {
        dupPath = g_strdup(pu);
    }

    if (dupPath) {
        file_path_format(dupPath);
        obj->filePath = g_strdup (dupPath);
        obj->fileURI = g_strdup_printf ("%s://%s", LIMIT_STR, dupPath);
    }

    // g_print("%s\n", obj->filePath);
    // g_print("%s\n", obj->fileURI);

    STR_FREE (dupPath);
}

static void limit_file_interface_init (GFileIface* interface)
{
    interface->dup                          = vfs_dup;
    interface->hash                         = vfs_hash;
    interface->get_uri                      = vfs_get_uri;
    interface->get_path                     = vfs_get_path;
    interface->equal                        = vfs_is_equal;
    interface->is_native                    = vfs_is_native;
    interface->get_parent                   = vfs_get_parent;
    interface->has_uri_scheme               = vfs_has_schema;
    interface->get_basename                 = vfs_get_basename;
    interface->get_uri_scheme               = vfs_get_uri_schema;
    interface->query_info                   = vfs_file_query_info;
    interface->enumerate_children           = vfs_file_enum_children;
    interface->query_filesystem_info        = vfs_file_query_fs_info;
    interface->move                         = NULL;
    interface->copy                         = NULL;
    interface->resolve_relative_path        = vfs_resolve_relative_path;
    interface->get_child_for_display_name   = vfs_get_child_for_display_name;
    interface->supports_thread_contexts     = FALSE;
}

static GFile* vfs_lookup (GVfs* vfs, const char* uri, gpointer data)
{
    return vfs_parse_name(vfs, uri, data);
}

static GFile* vfs_parse_name (GVfs* vfs, const char* parseName, gpointer data)
{
    return limit_file_new_for_uri(parseName);
    (void) vfs;
    (void) data;
}

static char* vfs_get_basename (GFile* file)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);

    char** strArr = NULL;
    char* basename = NULL;
    char* path = vfs_get_path(file);
    if (path) {
        strArr = g_strsplit(path, "/", -1);
        if (g_strv_length(strArr) > 1) {
            for (int i = 0; strArr[i]; i++) {
                if (strArr[i] && (strArr[i + 1] == NULL)) {
                    basename = g_strdup(strArr[i]);
                }
            }
        }
    }

    STR_FREE(path);
    NOT_NULL_RUN(strArr, g_strfreev);

    return basename;
}

static char* vfs_get_uri (GFile* file)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);

    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);

    g_object_get_property (G_OBJECT (file), "s-uri", &value);

    return g_value_dup_string (&value);
}

static char* vfs_get_path (GFile* file)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);

    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);

    g_object_get_property (G_OBJECT (file), "s-path", &value);

    char* path = g_value_dup_string (&value);

    return path;
}

static char* vfs_get_uri_schema (GFile* file)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), NULL);

    return g_strdup(LIMIT_STR);
}

static gboolean vfs_is_native (GFile* file)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), FALSE);

    return TRUE;
}

static gboolean vfs_is_equal (GFile* file, GFile* file2)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file) && LIMIT_IS_FILE(file2), FALSE);

    const LimitFile* s1 = LIMIT_FILE (file);
    const LimitFile* s2 = LIMIT_FILE (file2);

    return (0 == g_strcmp0 (s1->fileURI, s2->fileURI));
}

static GFile* vfs_resolve_relative_path (GFile* file, const char* relativePath)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), NULL);

    GFile* resFile = NULL;
    char* path = g_file_get_path (file);

    g_print("===> path: %s, relative path: %s\n", path, relativePath);

#define D(base, real, gf) G_STMT_START { \
    char* p1 = g_strdup_printf("%s:///%s", base, real); \
    if (p1) { \
        gf = g_file_new_for_uri(p1); \
        g_free(p1); \
    } \
} G_STMT_END

    if (0 == g_strcmp0(path, "/")) {
        D(LIMIT_STR, relativePath, resFile);
    }
    else if (g_str_has_prefix(path, gRootDir)) {
        char* p1 = g_strdup_printf("/%s/%s", path + strlen(gRootDir) - 1, relativePath);
        if (p1) {
            file_path_format(p1);
            resFile = limit_file_new_for_path(p1);
            g_free(p1);
        }
    }

    NOT_NULL_RUN(path, g_free);
#undef D

    return resFile;
}

static GFile* vfs_dup (GFile* file)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), NULL);

    GFile* f = limit_file_new_for_uri(LIMIT_FILE(file)->fileURI);

    return f;
}

static GFile* vfs_get_parent (GFile* file)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), NULL);

    g_print("===>parent: %s\n", g_file_get_uri(file));

    GFile* f = limit_file_new_for_uri("/");
    return f;
    (void) file;
}

static GFile* vfs_get_child_for_display_name (GFile* file, const char* displayName, GError** error)
{
    g_return_val_if_fail (LIMIT_IS_FILE(file), NULL);

    g_print("display\n");

    return NULL;
}

static guint vfs_hash (GFile* file)
{
    guint hash = 0;

    g_return_val_if_fail (LIMIT_IS_FILE(file), hash);

    char* uri = vfs_get_uri (file);
    if (uri) {
        hash = g_str_hash(uri);
    }

    STR_FREE (uri);

    return hash;
}

static GFileInfo* vfs_file_enum_next_file (GFileEnumerator *enumerator, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail(LIMIT_IS_FILE_ENUM(enumerator), NULL);

    LimitFileEnum* eb = LIMIT_FILE_ENUM(enumerator);

    if (cancellable && g_cancellable_is_cancelled(cancellable)) {
        if (error) {
            *error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
        }
        return NULL;
    }

    // g_print("enumerator next file\n");

    char* uri = NULL;
    GFile* file = NULL;
    GFileInfo* fileInfo = NULL;
    GFileInfo* fileInfoT = g_file_enumerator_next_file(eb->fileEnumerator, cancellable, error);
    if (fileInfoT) {
        const char* name = g_file_info_get_name(fileInfoT);
        if (name) {
           const char* formatStr = (g_str_has_suffix(eb->fileURI, "/") ? "%s%s" : "%s/%s");
            uri = g_strdup_printf(formatStr, eb->fileURI, name);
            if (uri) {
                file = g_file_new_for_uri (uri);
                if (G_IS_FILE(file)) {
                    fileInfo = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NONE, cancellable, error);
                }
            }
        }
    }

#if 0
    g_print("\n");
    g_print("uri: %s\n", uri);
    g_print("fileInfo: %s\n", fileInfo ? "true" : "false");
    g_print("\n");
#endif

    NOT_NULL_RUN(uri, g_free);
    NOT_NULL_RUN(file, g_object_unref);
    NOT_NULL_RUN(fileInfoT, g_object_unref);

    return fileInfo;
}

GFileEnumerator* vfs_file_enum_children (GFile* file, const char* attribute, GFileQueryInfoFlags flags, GCancellable* cancel, GError** error)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);

    if (strlen(gRootDir) <= 0) {
        g_warning("please call function 'limit_set_root_dir' first!");
        return NULL;
    }

    GFileEnumerator* e = G_FILE_ENUMERATOR(g_object_new(LIMIT_FILE_ENUM_TYPE, "container", file, NULL));
    LimitFileEnum* eb = LIMIT_FILE_ENUM(e);
    if (eb) {
        char* path = g_file_get_path (file);
        if (path) {
            eb->filePath = g_strdup_printf("%s/%s", gRootDir, path);
            file_path_format(eb->filePath);
            eb->fileURI = g_file_get_uri(file);
            char* uri = g_strdup_printf("file://%s", eb->filePath);
            if (uri) {
                GFile* fileUri = g_file_new_for_uri (uri);
                if (G_IS_FILE(fileUri)) {
                    eb->fileEnumerator = g_file_enumerate_children(fileUri, attribute, flags, cancel, error);
                    g_object_unref (fileUri);
                }
                g_free(uri);
            }
            // g_print("uri: %s -- path: %s\n", eb->fileURI, eb->filePath);
        }
        NOT_NULL_RUN(path, g_free);
    }

    return e;
}

static gboolean vfs_file_enum_close (GFileEnumerator *enumerator, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail(LIMIT_IS_FILE_ENUM(enumerator), FALSE);

    LimitFileEnum* e = LIMIT_FILE_ENUM(enumerator);

    NOT_NULL_RUN(e->fileURI, g_free);
    NOT_NULL_RUN(e->filePath, g_free);
    NOT_NULL_RUN(e->fileEnumerator, g_object_unref);

    return TRUE;

    (void) error;
    (void) cancellable;
}

static GFileInfo* vfs_file_query_fs_info (GFile* file, const char* attr, GCancellable* cancel, GError** error)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);
    RETURN_VAL_IF_OK(strlen(gRootDir) > 0, NULL);

    GFile* f = NULL;
    GFileInfo* info = NULL;
    f = g_file_new_for_path (gRootDir);

    if (G_IS_FILE(f)) {
        info = g_file_query_info (f, attr, G_FILE_QUERY_INFO_NONE, cancel, error);
    }

    // g_print("fs info\n");

    return info;
}

static GFileInfo* vfs_file_query_info (GFile* file, const char* attr, GFileQueryInfoFlags flags, GCancellable* cancel, GError** error)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file), NULL);

    GFileInfo* info = NULL;

    char* uriTarget = NULL;
    GFile* fileTarget = NULL;
    GFileInfo* infoTarget = NULL;
    const char* nameTarget = NULL;
    const char* editNameTarget = NULL;
    const char* copyNameTarget = NULL;
    const char* dispNameTarget = NULL;

    char* uri = g_file_get_uri(file);
    char* path = g_file_get_path(file);

    // g_print(">>path: %s\n", path);

    if (path) {
        uriTarget = g_strdup_printf("file://%s/%s", gRootDir, path);
        if (uriTarget) {
            fileTarget = g_file_new_for_uri (uriTarget);
            if (G_IS_FILE(fileTarget)) {
                infoTarget = g_file_query_info(fileTarget, attr, flags, cancel, error);
                if (G_IS_FILE_INFO(infoTarget)) {
                    info = g_file_info_dup(infoTarget);
                    nameTarget = g_file_info_get_attribute_byte_string(infoTarget, G_FILE_ATTRIBUTE_STANDARD_NAME);
                    editNameTarget = g_file_info_get_attribute_string(infoTarget, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME);
                    copyNameTarget = g_file_info_get_attribute_string(infoTarget, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME);
                    dispNameTarget = g_file_info_get_attribute_string(infoTarget, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);

#if 0
                    g_print("\n");
                    g_print("name: %s\n", nameTarget);
                    g_print("edit: %s\n", editNameTarget);
                    g_print("copy: %s\n", copyNameTarget);
                    g_print("disp: %s\n", dispNameTarget);
                    g_print("\n");
#endif

                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL, TRUE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN, FALSE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP, FALSE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK,
                        g_file_info_get_attribute_boolean (infoTarget, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK));
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE, FALSE);

                    g_file_info_set_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                        g_file_info_get_attribute_uint32 (infoTarget, G_FILE_ATTRIBUTE_STANDARD_TYPE));

                    g_file_info_set_attribute_byte_string (info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, uriTarget);
                    g_file_info_set_attribute_byte_string(info, G_FILE_ATTRIBUTE_STANDARD_NAME, nameTarget);
                    g_file_info_set_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, editNameTarget);
                    g_file_info_set_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, copyNameTarget);
                    g_file_info_set_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, dispNameTarget);

                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, TRUE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, FALSE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, FALSE);
                    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, FALSE);
                }
            }
        }
    }

    STR_FREE(uri);
    STR_FREE(path);
    STR_FREE(uriTarget);
    NOT_NULL_RUN(fileTarget, g_object_unref);
    NOT_NULL_RUN(infoTarget, g_object_unref);

    // g_print("info: \n");

    return info;

    (void) attr;
    (void) flags;
    (void) error;
    (void) cancel;
}

static gboolean vfs_has_schema (GFile* file, const char* uriSchema)
{
    g_return_val_if_fail(LIMIT_IS_FILE(file) && uriSchema, FALSE);

    return g_str_has_prefix(uriSchema, LIMIT_STR);
}

static void file_path_format (char* filePath)
{
    g_return_if_fail(filePath);
    RETURN_IF_OK(filePath[0] == '/' && strlen(filePath) == 1);

    int i = 0;
    const int fLen = (int) strlen (filePath);
    for (i = 0; filePath[i]; ++i) {
        while (filePath[i] && '/' == filePath[i] && '/' == filePath[i + 1]) {
            for (int j = i; filePath[j] || j < fLen; filePath[j] = filePath[j + 1], ++j);
        }
    }

    if ((i - 1 >= 0) && filePath[i - 1] == '/') {
        filePath[i - 1] = '\0';
    }
}