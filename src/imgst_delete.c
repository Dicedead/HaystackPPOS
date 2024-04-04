#include "imgStore.h"
#include "error.h"

#include <stdio.h> // for sprintf

/********************************************************************//**
 * Delete an image from an imgst_file.
 */
int do_delete(const char *imgID, struct imgst_file *imgst_file) {

    M_REQUIRE_NON_NULL(imgID);
    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE_NON_NULL(imgst_file);

    if (imgst_file->header.num_files == 0) {
        return ERR_FILE_NOT_FOUND;
    }

    for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
        if (imgst_file->metadata[i].is_valid == NON_EMPTY && strcmp(imgst_file->metadata[i].img_id, imgID) == 0) {

            imgst_file->metadata[i].is_valid = EMPTY;

            M_REQ(fseek(imgst_file->file, sizeof(imgst_file->header) + i * sizeof(struct img_metadata), SEEK_SET) == 0,
                    ERR_IO, "fseek for metadata failed in do_delete");
            M_WRITE(imgst_file->metadata[i], imgst_file->file, "unable to write metadata in do_delete");

            imgst_file->header.imgst_version += 1;
            imgst_file->header.num_files -= 1;

            M_REQ(fseek(imgst_file->file, 0, SEEK_SET) == 0, ERR_IO, "fseek for header failed in do_delete");
            M_WRITE(imgst_file->header, imgst_file->file, "unable to write header in do_delete");

            return ERR_NONE; //since we only delete the first image
        }
    }

    return ERR_FILE_NOT_FOUND;
}
