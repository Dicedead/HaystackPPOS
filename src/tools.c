/* ** NOTE: undocumented in Doxygen
 * @file tools.c
 * @brief implementation of several tool functions for imgStore
 *
 * @author Mia Primorac
 */

#include "imgStore.h"
#include "error.h"

#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <stdlib.h>

/********************************************************************//**
 * Human-readable SHA
 */
static void sha_to_string(const unsigned char *SHA, char *sha_string) {

    M_REQUIRE_NON_NULL_RET_VOID(SHA, "null argument in sha_to_string");

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * imgStore header display.
 */
void print_header(const struct imgst_header *header) {

    M_REQUIRE_NON_NULL_RET_VOID(header, "null argument in print_header");

    FILE *out = stdout;

    fprintf(out, "*****************************************\n");
    fprintf(out, "**********IMGSTORE HEADER START**********\n");

    fprintf(out, "TYPE: %31s\n", header->imgst_name);
    fprintf(out, "VERSION: %"      PRIu32 "\n", header->imgst_version);
    fprintf(out, "IMAGE COUNT: %"  PRIu32 "\t\tMAX IMAGES: %" PRIu32 "\n",
            header->num_files, header->max_files);

    fprintf(out, "THUMBNAIL: %" PRIu16 " x %" PRIu16 "\tSMALL: %" PRIu16 " x %" PRIu16 "\n",
            header->res_resized[2 * RES_THUMB], header->res_resized[2 * RES_THUMB + 1],
            header->res_resized[2 * RES_SMALL], header->res_resized[2 * RES_SMALL + 1]);

    fprintf(out, "***********IMGSTORE HEADER END***********\n");
    fprintf(out, "*****************************************\n");

}

/********************************************************************//**
 * Metadata display.
 */
void print_metadata(const struct img_metadata *metadata) {

    M_REQUIRE_NON_NULL_RET_VOID(metadata, "null argument in print_metadata");

    FILE *out = stdout;
    char sha_string[2 * SHA256_DIGEST_LENGTH + 1] = "";
    sha_to_string(metadata->SHA, sha_string);

    fprintf(out, "IMAGE ID: %s\n", metadata->img_id);
    fprintf(out, "SHA: %s\n", sha_string);
    fprintf(out, "VALID: %"          PRIu16 "\n", metadata->is_valid);
    fprintf(out, "UNUSED: %"         PRIu16"\n", metadata->unused_16);

    fprintf(out, "OFFSET ORIG. : %"  PRIu64 "\t\tSIZE ORIG. : %" PRIu16"\n", metadata->offset[RES_ORIG],
            metadata->size[RES_ORIG]);
    fprintf(out, "OFFSET THUMB.: %"  PRIu64 "\t\tSIZE THUMB.: %" PRIu16"\n", metadata->offset[RES_THUMB],
            metadata->size[RES_THUMB]);
    fprintf(out, "OFFSET SMALL : %"  PRIu64 "\t\tSIZE SMALL : %" PRIu16"\n", metadata->offset[RES_SMALL],
            metadata->size[RES_SMALL]);
    fprintf(out, "ORIGINAL: %"       PRIu32" x %"                PRIu32"\n", metadata->res_orig[0],
            metadata->res_orig[1]);

    fprintf(out, "*****************************************\n");

}

/********************************************************************//**
 * Read a header and metadata into an imgst_file.
 */
int do_open(const char *imgst_filename, const char *open_mode, struct imgst_file *imgst_file) {

    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE_NON_NULL(open_mode);

    FILE *file = fopen(imgst_filename, open_mode);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);

    M_EXIT_IF_ERR_DO_SOMETHING(fread(&imgst_file->header, sizeof(imgst_file->header), 1, file) == 1 ? ERR_NONE : ERR_IO,
                               fclose(file));

    imgst_file->metadata = calloc(sizeof(struct img_metadata), imgst_file->header.max_files);
    M_EXIT_IF_ERR_DO_SOMETHING(imgst_file->metadata == NULL ? ERR_OUT_OF_MEMORY : ERR_NONE, fclose(file));


    for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
        M_EXIT_IF_ERR_DO_SOMETHING(
                fread(&imgst_file->metadata[i], sizeof(imgst_file->metadata[i]), 1, file) == 1 ? ERR_NONE : ERR_IO,
                GROUP_CALLS(fclose(file), FREE(imgst_file->metadata)));
    }

    imgst_file->file = file;
    return ERR_NONE;
}

/********************************************************************//**
 * Close an imgst_file's FILE attribute.
 */
void do_close(struct imgst_file *imgst_file) {

    M_REQUIRE_NON_NULL_RET_VOID(imgst_file, "null argument in do_close");
    M_REQUIRE_NON_NULL_RET_VOID(imgst_file->file, "null file in do_close");

    fclose(imgst_file->file);

    if (imgst_file->metadata != NULL) {
        free(imgst_file->metadata);
        imgst_file->metadata = NULL;
    }
}

#define MAX_SIZE_WORD 9
#define STR_COMP(str1, str2) strncmp(str1, str2, MAX_SIZE_WORD)
/**
 * Transforms a string representing resolution
 * @param resolution the string containing the ame of the given orientation
 * @return ERR_RESOLUTIONS if it does not contain a valid name, or the code for the resolution
 */
int resolution_atoi(const char *resolution) {
    if (resolution == NULL) return ERR_RESOLUTIONS;

    if (STR_COMP(resolution, "thumb") == 0 || STR_COMP(resolution, "thumbnail") == 0) {
        return RES_THUMB;
    } else if (STR_COMP(resolution, "small") == 0) {
        return RES_SMALL;
    } else if (STR_COMP(resolution, "orig") == 0 || STR_COMP(resolution, "original") == 0) {
        return RES_ORIG;
    } else {
        return ERR_RESOLUTIONS;
    }

}
