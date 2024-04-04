#include "imgStore.h"
#include "error.h"
#include <stdio.h>

/**
 * @brief Removes the deleted images by moving the existing ones
 */
int do_gbcollect(const char *imgst_path, const char *imgst_tmp_bkp_path) {
    M_REQUIRE_NON_NULL(imgst_path);
    M_REQUIRE_NON_NULL(imgst_tmp_bkp_path);

    imgst_file old, temp;
    int err;

    M_REQUIRE((err = do_open(imgst_path, "r+b", &old))==ERR_NONE, err,
              "Failed to open imgst file to collect at %s", imgst_path);
    M_REQUIRE((err=do_create(imgst_tmp_bkp_path, &temp))==ERR_NONE, err,
              "Failed to create temporary imgst file at %s", imgst_tmp_bkp_path);

    uint32_t    im_size;
    char*       image_buffer;

    for (size_t i = 0; i < old.header.num_files; ++i) {

        char* name;
        M_REQUIRE_NON_NULL(name = old.metadata[i].img_id);
        if (old.metadata[i].is_valid == EMPTY)
            continue;
        for (size_t j = RES_ORIG; j < NB_RES;  ++j) {
            if (old.metadata[i].size[j] != 0) {

                M_REQUIRE((err = do_read(name, j, &image_buffer, &im_size, &old)) == ERR_NONE,
                          err, "Failed to read image : %s", old.metadata[i].img_id);

                M_REQUIRE((err = do_insert(image_buffer, im_size, name, &temp)) == ERR_NONE, err,
                          "Failed to insert image : %s", old.metadata[i].img_id);
            }
        }
    }

    FREE(image_buffer);

    M_REQUIRE(remove(imgst_path) != 0, ERR_IO, "Failed to remove old file at : %s", imgst_path);
    M_REQUIRE(rename(imgst_tmp_bkp_path, imgst_path) != 0, ERR_IO, "Failed to rename temp file at : %s", imgst_tmp_bkp_path);
    do_close(&old) ;

    return ERR_NONE;
}
