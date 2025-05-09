//
// Created by dingjing on 25-5-8.
//

#ifndef gvfs_limit_VFS_LIMIT_H
#define gvfs_limit_VFS_LIMIT_H
#define G_DISABLE_DEPRECATED
#include <gio/gio.h>

G_BEGIN_DECLS

#define LIMIT_STR                                       "andsec-yun"
#define STR_FREE(f)                                     G_STMT_START { if (f) { g_free (f); f = NULL; } } G_STMT_END

#define LIMIT_FILE_TYPE                                 (limit_file_get_type())
#define LIMIT_IS_FILE_CLASS(k)                          (G_TYPE_CHECK_CLASS_TYPE((k), LIMIT_FILE_TYPE))
#define LIMIT_IS_FILE(k)                                (G_TYPE_CHECK_INSTANCE_TYPE((k), LIMIT_FILE_TYPE))
#define LIMIT_FILE_CLASS(k)                             (G_TYPE_CHECK_CLASS_CAST((k), LIMIT_FILE_TYPE, LimitFileClass))
#define LIMIT_FILE(k)                                   (G_TYPE_CHECK_INSTANCE_CAST((k), LIMIT_FILE_TYPE, LimitFile))
#define LIMIT_FILE_GET_CLASS(k)                         (G_TYPE_INSTANCE_GET_CLASS((k), LIMIT_FILE_TYPE, LimitFileClass))

#define LIMIT_FILE_ENUM_TYPE                            (limit_file_enum_get_type())
#define LIMIT_IS_FILE_ENUM_CLASS(k)                     (G_TYPE_CHECK_CLASS_TYPE((k), LIMIT_FILE_ENUM_TYPE))
#define LIMIT_IS_FILE_ENUM(k)                           (G_TYPE_CHECK_INSTANCE_TYPE((k), LIMIT_FILE_ENUM_TYPE))
#define LIMIT_FILE_ENUM_CLASS(k)                        (G_TYPE_CHECK_CLASS_CAST((k), LIMIT_FILE_ENUM_TYPE, LimitFileEnumClass))
#define LIMIT_FILE_ENUM(k)                              (G_TYPE_CHECK_INSTANCE_CAST((k), LIMIT_FILE_ENUM_TYPE, LimitFileEnum))
#define LIMIT_FILE_ENUM_GET_CLASS(k)                    (G_TYPE_INSTANCE_GET_CLASS((k), LIMIT_FILE_ENUM_TYPE, LimitFileEnumClass))

G_DECLARE_FINAL_TYPE(LimitFile, limit_file, LimitFile, LIMIT_FILE_TYPE, GObject)
G_DECLARE_FINAL_TYPE(LimitFileEnum, limit_file_enum, LimitFileEnum, LIMIT_FILE_ENUM_TYPE, GFileEnumerator)


const char*             limit_get_root_uri              ();
void                    limit_file_register             ();
void                    limit_set_root_dir              (const char* rootDir, gint32 pathLen);

GType                   limit_file_get_type             (void) G_GNUC_CONST;
GType                   limit_file_enum_get_type        (void) G_GNUC_CONST;
GFile*                  limit_file_new_for_path         (const gchar* path);

G_END_DECLS

#endif // gvfs_limit_VFS_LIMIT_H
