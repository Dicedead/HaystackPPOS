/**
 * @file imgStore.h
 * @brief Main header file for imgStore core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The image imgStore starts with exactly one header structure
 * followed by exactly imgst_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * imgStore file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 */

#ifndef IMGSTOREPRJ_IMGSTORE_H
#define IMGSTOREPRJ_IMGSTORE_H

#include "error.h" /* not needed in this very file,
                    * but we provide it here, as it is required by
                    * all the functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define CAT_TXT "EPFL ImgStore binary"

/* constraints */
#define MAX_IMGST_NAME  31      // max. size of a ImgStore name
#define MAX_IMG_ID      127     // max. size of an image id
#define MAX_MAX_FILES   100000
#define DEFAULT_MAX_FILES 10

/* For is_valid in imgst_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// imgStore library internal codes for different image resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

#define DEFAULT_RES_THUMB 64
#define MAX_RES_THUMB 128

#define DEFAULT_RES_SMALL 256
#define MAX_RES_SMALL 512

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configuration information of an imgStore.
 */
struct imgst_header {

    /**
     * Name of the image database.
     */
    char imgst_name[MAX_IMGST_NAME + 1];

    /**
     * Version of the image database, to be incremented after each modification.
     */
    uint32_t imgst_version;

    /**
     * Number of valid images in this database.
     */
    uint32_t num_files;

    /**
     * Max number of images in this database, must not be modified after init.
     */
    const uint32_t max_files;

    /**
     * Array of maximum resolutions of "small" and "thumbnail" formats of images.
     */
    const uint16_t res_resized[2 * (NB_RES - 1)];

    uint32_t unused_32;
    uint64_t unused_64;

};

typedef struct imgst_header imgst_header;

/**
 * The metadata of an image.
 */
struct img_metadata {

    /**
     * This image's unique ID.
     */
    char img_id[MAX_IMG_ID + 1];

    /**
     * This image's hashcode.
     */
    unsigned char SHA[SHA256_DIGEST_LENGTH];

    /**
     * Original image's resolution.
     */
    uint32_t res_orig[2];

    /**
     * Array of all the sizes in memory, for each resolution.
     */
    uint32_t size[NB_RES];

    /**
     * Array of all the resolutions' positions in imgStore database.
     */
    uint64_t offset[NB_RES];

    /**
     * Value is NON_EMPTY if image is in use, EMPTY else.
     */
    uint16_t is_valid;

    uint16_t unused_16;

};

typedef struct img_metadata img_metadata;

/**
 * File information of this database.
 */
struct imgst_file {

    /**
     * Pointer to file containing database on this disk.
     */
    FILE *file;

    /**
     * This database's header.
     */
    imgst_header header;

    /**
     * Array of metadata of images in the imgStore database.
     */
    img_metadata *metadata;
};

typedef struct imgst_file imgst_file;

/**
 * @brief Prints imgStore header information.
 *
 * @param header The header to be displayed.
 */
void print_header(const struct imgst_header *header);

/**
 * @brief Prints image metadata informations.
 *
 * @param metadata The metadata of one image.
 */
void print_metadata(const struct img_metadata *metadata);

/**
 * @brief Open imgStore file, read the header and all the metadata.
 *
 * @param imgst_filename Path to the imgStore file
 * @param open_mode Mode for fopen(), eg.: "rb", "rb+", etc.
 * @param imgst_file Structure for header, metadata and file pointer.
 */
int do_open(const char *imgst_filename, const char *open_mode, struct imgst_file *imgst_file);

/**
 * @brief Do some clean-up for imgStore file handling.
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
void do_close(struct imgst_file *imgst_file);

/**
 * @brief List of possible output modes for do_list
 *
 */
enum do_list_mode {
    STDOUT,
    JSON
};

typedef enum do_list_mode do_list_mode;

/**
 * @brief Displays (on stdout) imgStore metadata.
 *
 * @param imgst_file In memory structure with header and metadata.
 * @return error code
 */
char *do_list(const struct imgst_file *imgst_file, do_list_mode mode);

/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 *
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 */
int do_create(const char *imgst_filename, struct imgst_file *mem);

/**
 * @brief Deletes an image from a imgStore imgStore.
 *
 * Effectively, it only invalidates the is_valid field and updates the
 * metadata.  The raw data content is not erased, it stays where it
 * was (and  new content is always appended to the end; no garbage
 * collection).
 *
 * @param img_id The ID of the image to be deleted.
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_delete(const char *imgID, struct imgst_file *imgst_file);

/**
 * @brief Transforms resolution string to its int value.
 *
 * @param resolution The resolution string. Shall be "original",
 *        "orig", "thumbnail", "thumb" or "small".
 * @return The corresponding value or -1 if error.
 */
int resolution_atoi(const char *resolution);

/**
 * @brief Reads the content of an image from a imgStore.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char *img_id, int resolution, char **image_buffer, uint32_t *image_size, imgst_file *imgst_file);

/**
 * @brief Insert image in the imgStore file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @param imgst_file Image database
 * @return Some error code. 0 if no error.
 */
int do_insert(const char *buffer, size_t size, const char *img_id, imgst_file *imgst_file);

/**
 * @brief Removes the deleted images by moving the existing ones
 *
 * @param imgst_path The path to the imgStore file
 * @param imgst_tmp_bkp_path The path to the a (to be created) temporary imgStore backup file
 * @return Some error code. 0 if no error.
 */
int do_gbcollect(const char *imgst_path, const char *imgst_tmp_bkp_path);

#ifdef __cplusplus
}
#endif
#endif
