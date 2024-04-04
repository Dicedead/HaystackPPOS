#include <stdbool.h>
#include "imgStore.h"
#include "dedup.h"
#include "image_content.h"

/**
 * @brief Finds the first metadata for which valid bit is 0, if none is: returns -1
 * @param imgst_file
 * @return size_t
 */
static uint32_t find_first_free_meta(const imgst_file *imgst_file);

/**
 * @brief Tests whether some passed image has a duplicate by checking its offset array
 * @param img Image's metadata
 * @return Test result
 */
static bool image_has_no_duplicate(const img_metadata *img);

/**
 * @brief Finish up initialisation of a metadata
 * @param metadata
 */
static void complete_init(img_metadata *target_img);

/**
 * @brief Insert image in the imgStore file
 */
int do_insert(const char *buffer, size_t size, const char *img_id, imgst_file *imgst_file) {

    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE_NON_NULL(buffer);
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgst_file->metadata);
    M_REQ(size >= 0, ERR_INVALID_ARGUMENT, "negative image size in do_insert");
    M_REQ(strlen(img_id) < MAX_IMG_ID, ERR_INVALID_IMGID, "too long img id");

    M_REQ(imgst_file->header.num_files < imgst_file->header.max_files, ERR_FULL_IMGSTORE, "imgStore full in do_insert");

    // I) Free spot finding and image loading
    size_t insertion_index = find_first_free_meta(imgst_file);
    M_REQ(insertion_index >= 0, ERR_FULL_IMGSTORE, "imgStore full in do_insert - detected after find_first_free_meta");
    img_metadata *target_img = &imgst_file->metadata[insertion_index];
    SHA256((const unsigned char *) buffer, size, target_img->SHA);
    strncpy(target_img->img_id, img_id, MAX_IMG_ID);
    target_img->size[RES_ORIG] = size;
    target_img->is_valid = NON_EMPTY;

    // II) Writing image, avoiding duplication
    int possible_err = do_name_and_content_dedup(imgst_file, insertion_index);
    M_REQ(possible_err == ERR_NONE, possible_err, "error in do_name_and_content_dedup, called by do_insert");
    if (image_has_no_duplicate(target_img)) {
        complete_init(target_img);
        M_REQ(fseek(imgst_file->file, 0, SEEK_END) == 0, ERR_IO, "couldn't fseek to end in line "__LINE__" in do_insert");
        target_img->offset[RES_ORIG] = ftell(imgst_file->file);
        M_REQ(fwrite(buffer, sizeof (char), size, imgst_file->file) == size, ERR_IO, "unable to write image content in do_insert");
    }

    // III) Updating database header & metadata information
    uint32_t height = -1;
    uint32_t width = -1;
    possible_err = get_resolution(&height, &width, buffer, size);
    M_REQ(possible_err == ERR_NONE, possible_err, "error in get_resolution called by do_insert");
    target_img->res_orig[0] = width;
    target_img->res_orig[1] = height;

    ++imgst_file->header.num_files;
    ++imgst_file->header.imgst_version;
    M_REQ(fseek(imgst_file->file, 0, SEEK_SET) == 0, ERR_IO, "couldn't fseek to start in line "__LINE__" in do_insert");
    M_WRITE(imgst_file->header, imgst_file->file, "unable to write header in do_insert");
    M_REQ(fseek(imgst_file->file, sizeof(img_metadata) * insertion_index, SEEK_CUR) == 0, ERR_IO, "couldn't fseek to metadata in do_insert");
    M_WRITE(*target_img, imgst_file->file, "unable to write metadata in do_insert");

    return ERR_NONE;
}

static uint32_t find_first_free_meta(const imgst_file *imgst_file) {

    for (uint32_t i = 0; i < imgst_file->header.max_files; ++i) {
        if (imgst_file->metadata[i].is_valid == EMPTY) {
            return i;
        }
    }

    return -1;
}

static bool image_has_no_duplicate(const img_metadata *img) {
    return img->offset[RES_ORIG] == 0;
}

static void complete_init(img_metadata *target_img) {
    target_img->size[RES_SMALL] = 0;
    target_img->size[RES_THUMB] = 0;
    target_img->offset[RES_THUMB] = 0;
    target_img->offset[RES_SMALL] = 0;
    target_img->unused_16 = 0;
}
