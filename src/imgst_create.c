/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 */

#include "imgStore.h"
#include "error.h"

#include <string.h> // for strncpy
#include <stdlib.h>

/**
 * Creates the imgStore called imgst_filename. Writes the header and the preallocated empty metadata array to
 * imgStore file.
 *
 */
int do_create(const char *imgst_filename, struct imgst_file *imgst_file) {

    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(imgst_file);

    strncpy(imgst_file->header.imgst_name, CAT_TXT, MAX_IMGST_NAME);
    imgst_file->header.imgst_name[MAX_IMGST_NAME] = '\0';

    FILE *file = fopen(imgst_filename, "wb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);

    imgst_file->file = file;
    imgst_file->header.unused_32 = 0;
    imgst_file->header.unused_64 = 0;
    imgst_file->header.imgst_version = 0;
    imgst_file->header.num_files = 0;

    size_t size_written = 0;
    M_EXIT_IF_ERR_DO_SOMETHING(fwrite(&imgst_file->header, sizeof(imgst_file->header), 1, file) == 1 ? ERR_NONE : ERR_IO,
                               fclose(file));
    size_written += 1;

    imgst_file->metadata = calloc(imgst_file->header.max_files, sizeof(struct img_metadata));
    M_EXIT_IF_ERR_DO_SOMETHING(imgst_file->metadata == NULL ? ERR_OUT_OF_MEMORY : ERR_NONE, fclose(file));

    for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
        M_EXIT_IF_ERR_DO_SOMETHING(fwrite(&imgst_file->metadata[i], sizeof(imgst_file->metadata[i]), 1, file) == 1 ? ERR_NONE : ERR_IO,
                                   GROUP_CALLS(fclose(file), FREE(imgst_file->metadata)));
        size_written += 1;
    }

    printf("%lu item(s) written \n", size_written);
    print_header(&imgst_file->header);
    return ERR_NONE;
}
