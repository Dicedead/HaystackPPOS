#include "imgStore.h"
#include "image_content.h"

/**
 * @brief Finds the first index of an image in the database with the same id as img_id
 *
 * @param imgst_file Main datastucture
 * @param img_id Name sought after
 * @return Index of the first image with id equal to img_id, -1 if none exists
 */
static size_t find_name_matching(imgst_file *imgst_file, const char *img_id);

/**
 * Reads an image given its ID, its resolution and the database file it is in
 * @param img_id the name of the image wanted
 * @param resolution the resolution in which the image is wanted
 * @param image_buffer the buffer that will hold the image
 * @param image_size will hold the size in memory of the image if it succeeds
 * @param imgst_file the database in which the image should be
 * @return an error code, ERR_NONE if everything worked
 */
int do_read(const char *img_id, int resolution, char **image_buffer, uint32_t *image_size, imgst_file *imgst_file) {
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgst_file);

    int err;
    size_t index;

    M_REQ((index = find_name_matching(imgst_file, img_id)) != -1, ERR_FILE_NOT_FOUND, "error in do_read : imgID not found");

    if (imgst_file->metadata[index].size[resolution] == 0) {
        M_REQUIRE((err = lazily_resize(resolution, imgst_file, index)) == ERR_NONE, err,
                  "error in do_read : lazily_resize failed with the image %s", img_id);
    }

    uint64_t offset = imgst_file->metadata[index].offset[resolution];
    *image_size = imgst_file->metadata[index].size[resolution];

    char *data = (char *) calloc(*image_size, sizeof(char));
    M_REQUIRE_NON_NULL_CUSTOM_ERR(data, ERR_OUT_OF_MEMORY);
    M_REQ_CLEAN(fseek(imgst_file->file, offset, SEEK_SET) == 0,
                ERR_IO, "unable to fseek in imgst read", 1, data);
    M_REQ_CLEAN(fread(data, *image_size, 1, imgst_file->file) == 1,
                ERR_IO, "unable to read wanted image in do_read", 1, data);

    *image_buffer = data;

    return ERR_NONE;
}

static size_t find_name_matching(imgst_file *imgst_file, const char *img_id) {

    for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
        if (imgst_file->metadata[i].is_valid == NON_EMPTY && strncmp(imgst_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            return i;
        }
    }
    return -1;
}
