#include "dedup.h"
#include <stdbool.h>
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define DUPLICATE_FOUND (NB_ERR + 10)
#define DUPLICATE_NOT_FOUND (NB_ERR + 20)

/**
 * @brief Compares two SHAs, determines whether they're equal or not
 *
 * @param sha_1
 * @param sha_2
 * @return bool
 */
static bool are_equal(const unsigned char *sha_1, const unsigned char *sha_2);

/**
 * @brief Transfers all attributes from src to target except SHA, id and is_valid (as we already know they are equal in
 *        do_name_and_content_dedup
 *
 * @param target img_metadata to modify
 * @param src img_metadata to copy attributes from
 */
static void copy_attributes(img_metadata *target, const img_metadata *src);

/**
 * @brief Finds the potential duplicate of target_metadata, stores its index in content_duplicate_index
 *
 * @param content_duplicate_index Pointer to store duplicate's index
 * @param imgst_file Database structure
 * @param index Index of the target metadata in imgst_file's metadata buffer
 * @return predefined values indicating a duplicate was found or not, or an error code
 */
static int find_duplicate(size_t *content_duplicate_index, imgst_file *imgst_file, uint32_t index);

/**
 * @brief Avoids content duplication in the img database.
 */
int do_name_and_content_dedup(imgst_file *imgst_file, uint32_t index) {
    M_REQUIRE_NON_NULL(imgst_file);
    M_REQ(0 <= index && index < imgst_file->header.max_files, ERR_INVALID_ARGUMENT, "out of bounds index in dedup");

    size_t content_duplicate_index = 0;
    int content_duplicated = find_duplicate(&content_duplicate_index, imgst_file, index);

    switch (content_duplicated) {
        case DUPLICATE_FOUND:
            copy_attributes(&imgst_file->metadata[index], &imgst_file->metadata[content_duplicate_index]); break;
        case DUPLICATE_NOT_FOUND:
            imgst_file->metadata[index].offset[RES_ORIG] = 0; break;
        default:
            return content_duplicated;
    }

    return ERR_NONE;
}

static bool are_equal(const unsigned char *sha_1, const unsigned char *sha_2) {
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        if (sha_1[i] != sha_2[i]) {
            return false;
        }
    } // converting SHAs to strings then applying strncmp, like in tools.c's sha_to_string felt overkill

    return true;
}

static void copy_attributes(img_metadata *target, const img_metadata *src) {
    M_REQUIRE_NON_NULL_RET_VOID(target, "null argument in dedup: copy_attributes");
    M_REQUIRE_NON_NULL_RET_VOID(src, "null argument in dedup: copy_attributes");

    target->unused_16 = src->unused_16;
    target->res_orig[0] = src->res_orig[0];
    target->res_orig[1] = src->res_orig[1];

    for (size_t i = 0; i < NB_RES; ++i) {
        target->size[i] = src->size[i]; //copies original size too for code concision
        target->offset[i] = src->offset[i];
    }
}

static int find_duplicate(size_t *content_duplicate_index, imgst_file *imgst_file, uint32_t index) {
    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE_NON_NULL(content_duplicate_index);

    *content_duplicate_index = 0;
    for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
        img_metadata *curr_metadata = &imgst_file->metadata[i];
        if (curr_metadata->is_valid == NON_EMPTY && i != index) {

            M_REQ(strncmp(curr_metadata->img_id, imgst_file->metadata[index].img_id, MAX_IMG_ID) != 0, ERR_DUPLICATE_ID,
                  "two images with the same id located in dedup");

            if (are_equal(curr_metadata->SHA, imgst_file->metadata[index].SHA)) {
                return DUPLICATE_FOUND;
            }
        }

        ++*content_duplicate_index;
    }

    return DUPLICATE_NOT_FOUND;
}
