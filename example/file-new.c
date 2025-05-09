//
// Created by dingjing on 25-5-8.
//

#include "../src/vfs-limit.h"

int main (int argc, char* argv[])
{
#define ROOT_DIR "/usr/local"

    g_autoptr(GError) error = NULL;
    limit_file_register();
    limit_set_root_dir(ROOT_DIR, strlen(ROOT_DIR));

    g_autoptr(GFile) file = g_file_new_for_uri (limit_get_root_uri());
    g_return_val_if_fail(G_IS_FILE(file), -1);

    g_autoptr(GFileEnumerator) fileEnum = g_file_enumerate_children(file, "standard::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);
    if (error) {
        g_info("get enumerator error: %s\n", error->message);
        return -2;
    }

    g_autoptr(GFileInfo) fileInfo = NULL;
#define E(f, c, e) g_file_enumerator_next_file(f, c, e)
    for (fileInfo = E(fileEnum, NULL, &error); fileInfo; fileInfo = E(fileEnum, NULL, &error)) {
        const char* name = g_file_info_get_display_name(fileInfo);
        g_autoptr(GFile) fileT = g_file_resolve_relative_path(file, name);
        g_autofree char* fileStrT = g_file_get_uri(fileT);
        const char* dspStr = g_file_info_get_display_name(fileInfo);
        g_print("==>file: %s\n"
                "   uri: %s\n"
                "   display: %s\n", name, fileStrT, dspStr);
    }
#undef E


    return 0;

    (void) argc;
    (void) argv;
}