/**
 * @file imgst_content.c
 * @brief imgStore library: lazily_resize implementation.
 */

#include "image_content.h"

#include <stdbool.h>
#include <vips/vips.h>
#include <malloc.h>
#include <unistd.h>

/**
 * @brief Determines whether or not a size has already been computed
 *
 * @param possible_offset
 * @return 0 if size doesn't, else 1
 */
static bool size_already_exists(uint64_t possible_offset);

/**
 * @brief Computes the shrinking factor (keeping aspect ratio)
 *
 * @param image The image to be resized.
 * @param max_thumbnail_width The maximum width allowed for thumbnail creation.
 * @param max_thumbnail_height The maximum height allowed for thumbnail creation.
 */
static double shrink_value(const VipsImage *image, uint32_t max_thumbnail_width, uint32_t max_thumbnail_height);

/**
 * @brief Reads and computes a new, resized image
 *
 * @param data Pointer to image data
 * @param len Pointer to (where to) save image size after computation
 * @param position Index of the image in metadata pointer
 * @param imgst_file Main datastructure
 * @param size_code Image's size_code
 * @return error code, ERR_NONE if no error happened
 */
static int load_and_compute_image(size_t *len, size_t position, imgst_file *imgst_file, size_t size_code, void **out_data);

/**
 * @brief Create a resized (smaller) version of an image lazily, and store it in the database.
 */
int lazily_resize(int size_code, imgst_file *imgst_file, size_t position) {

    //-------------------------------------------------------------
    // I) Error handling

    M_EXIT_NO_ERR_IF(size_code == RES_ORIG);

    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE_NON_NULL(imgst_file->metadata);
    M_REQUIRE_NON_NULL(imgst_file->file);

    M_REQ(0 <= position && position < imgst_file->header.max_files, ERR_INVALID_ARGUMENT,
          "position out of bounds in lazily_resize");

    M_REQ(size_code == RES_THUMB || size_code == RES_SMALL, ERR_RESOLUTIONS, "invalid resolutions in lazily_resize");

    M_EXIT_NO_ERR_IF(size_already_exists(imgst_file->metadata[position].offset[size_code]));

    //-------------------------------------------------------------
    // II) Read original image from file and compute new image

    void *out_data;
    size_t len;
    int err;

    M_REQ((err = load_and_compute_image(&len, position, imgst_file, size_code, &out_data)) == ERR_NONE, err,
          "error while computing image in lazily_resize");

    //-------------------------------------------------------------
    // III) Save new image in disk

    M_REQ_CLEAN(fseek(imgst_file->file, 0, SEEK_END) == 0, ERR_IO, "couldn't fseek in lazily resize", 1, out_data);
    const long offset_new_image = ftell(imgst_file->file);

    M_REQ_CLEAN(fwrite(out_data, len, 1, imgst_file->file) == 1, ERR_IO,
                "unable to write new image to file in lazily_resize", 1, out_data);

    imgst_file->metadata[position].offset[size_code] = offset_new_image;
    imgst_file->metadata[position].size[size_code] = ftell(imgst_file->file) - offset_new_image;

    M_REQ_CLEAN(fseek(imgst_file->file, sizeof(imgst_file->header) + position * sizeof(struct img_metadata), SEEK_SET) == 0,
            ERR_IO, "couldn't fseek in lazily resize", 1, out_data);

    M_REQ_CLEAN(fwrite(&imgst_file->metadata[position], sizeof(struct img_metadata), 1, imgst_file->file) == 1,
                ERR_IO, "unable to write updated metadata to file in lazily_resize", 1, out_data);

    FREE(out_data);
    return ERR_NONE;
}

/**
 * @brief Gets resolution of some input image
 */
int get_resolution(uint32_t *height, uint32_t *width, const char *image_buffer, size_t image_size) {

    VipsImage *original = NULL;
    M_EXIT_IF_ERR_DO_SOMETHING((vips_jpegload_buffer((void *) image_buffer, image_size, &original, 0, NULL) == VIPS_ERR_NONE) ? ERR_NONE : ERR_IMGLIB,
                               g_object_unref(original));
    *height = vips_image_get_height(original);
    *width = vips_image_get_width(original);
    g_object_unref(original);

    return ERR_NONE;
}

static bool size_already_exists(uint64_t possible_offset) {
    return possible_offset != 0;
}

static double shrink_value(const VipsImage *image, uint32_t max_thumbnail_width, uint32_t max_thumbnail_height) {
    const double h_shrink = (double) max_thumbnail_width / (double) vips_image_get_height(image);
    const double v_shrink = (double) max_thumbnail_height / (double) vips_image_get_width(image);
    return h_shrink > v_shrink ? v_shrink : h_shrink;
}

static int load_and_compute_image(size_t *len, size_t position, imgst_file *imgst_file, size_t size_code, void **out_data) {

    const long offset_orig_imag     = (long) imgst_file->metadata[position].offset[RES_ORIG];
    const uint32_t size_orig_image  = imgst_file->metadata[position].size[RES_ORIG];

    void *data_ptr        = calloc(size_orig_image, sizeof(char));
    M_REQUIRE_NON_NULL_CUSTOM_ERR(data_ptr, ERR_OUT_OF_MEMORY);
    M_REQ(fseek(imgst_file->file, offset_orig_imag, SEEK_SET) == 0, ERR_IO, "couldn't fseek in load & compute image");
    M_REQ_CLEAN(fread(data_ptr, size_orig_image, 1, imgst_file->file) == 1, ERR_IO,
                "unable to read original image in lazily_resize", 1, data_ptr);

    VipsImage *original = NULL;
    VipsImage *resized  = NULL;

    M_REQ_CLEAN(vips_jpegload_buffer(data_ptr, size_orig_image, &original, NULL) == VIPS_ERR_NONE, ERR_IMGLIB,
                "error_imglib in lazily_resize: vips_jpegload_buffer", 1, data_ptr);

    const double ratio = shrink_value(original, imgst_file->header.res_resized[2 * size_code],
                                      imgst_file->header.res_resized[2 * size_code + 1]);

    M_EXIT_IF_ERR_DO_SOMETHING((vips_resize(original, &resized, ratio, NULL) == VIPS_ERR_NONE) ? ERR_NONE : ERR_IMGLIB,
                               GROUP_CALLS(FREE(data_ptr), g_object_unref(original)));

    g_object_unref(original);

    M_EXIT_IF_ERR_DO_SOMETHING((vips_jpegsave_buffer(resized, out_data, len, NULL) == VIPS_ERR_NONE) ? ERR_NONE : ERR_IMGLIB,
                               GROUP_CALLS(FREE(data_ptr), g_object_unref(resized)));

    FREE(data_ptr);
    g_object_unref(resized);

    return ERR_NONE;
}
