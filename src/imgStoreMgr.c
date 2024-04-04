/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpreter for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for _unused
#include "imgStore.h"
#include "error.h"
#include <string.h>
#include <vips/vips.h>

/**
 * @brief EXECUTE_COMMAND_EXPANDED opens an imgst_file, verifies something, performs an action, and then closes the file
 */
#define EXECUTE_COMMAND_EXPANDED(filename, open_mode, imgst_file, command, verification, err_code, fmt, err_value, n, ...) \
    do {                                                                                                 \
        (*err_value) = do_open((filename), (open_mode), &(imgst_file));                                  \
        if ((*err_value) == ERR_NONE) {                                                                  \
            M_REQ_CLEAN((verification), (err_code), (fmt), (n), __VA_ARGS__);                            \
            (*err_value) = (command);                                                                    \
            do_close(&(imgst_file));                                                                     \
        }                                                                                                \
    } while(0)

/**
 * @brief EXECUTE_COMMAND opens an imgst_file, performs an action on it, and then closes it. Also returns an error code.
 */
#define EXECUTE_COMMAND(filename, open_mode, imgst_file, command) \
    do {                                                          \
        int err_value = 0;                                        \
        EXECUTE_COMMAND_EXPANDED(filename, open_mode, imgst_file, command, 1, 0, "", &err_value, 0, NULL); \
        return err_value;                                                                                  \
    } while(0)

/**
 * @brief Reads an image from some filename
 *
 * @param filename Where to find the image
 * @param out_buffer Where to put the image - assumes unallocated out_buffer, need to free after call
 * @param size_ptr Size of the image
 * @return some error code, 0 if no error
 */
int read_disk_image(const char *filename, char **out_buffer, size_t *size_ptr) {

    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(size_ptr);

    FILE *file = NULL;
    M_REQ((file = fopen(filename, "r+b")) != NULL, ERR_IO, "couldn't open file in read_disk_image");
    M_EXIT_IF_ERR_DO_SOMETHING((fseek(file, 0, SEEK_END) == 0) ? ERR_NONE : ERR_IO, fclose(file));
    *size_ptr = ftell(file);
    M_REQ(fseek(file, 0, SEEK_SET) == 0, ERR_IO, "couldn't fseek file to start in read_disk_image");
    M_REQUIRE_NON_NULL(*out_buffer = calloc(*size_ptr, sizeof(char)));
    M_EXIT_IF_ERR_DO_SOMETHING((fread(*out_buffer, sizeof(char), *size_ptr, file) == *size_ptr) ? ERR_NONE : ERR_IO,
                               GROUP_CALLS(FREE(*out_buffer), fclose(file)));
    M_REQ_CLEAN(fclose(file) == 0, ERR_IO, "couldn't close file in read_disk_image", 1, *out_buffer, NULL);

    return ERR_NONE;
}

/**
 * @brief Saves an image to the given filename
 *
 * @param filename the path of the file where the image will be written
 * @param in_buffer the data i a simple byte array
 * @param size_ptr the size of the data written
 * @return some error code, 0 if no error
 */
int write_disk_image(const char *filename, char **in_buffer, size_t size_ptr) {
    M_REQUIRE_NON_NULL(filename);
    FILE *file;
    M_REQ((file = fopen(filename, "wb")) != NULL, ERR_IO, "couldn't open file in write_disk_image");
    M_EXIT_IF_ERR_DO_SOMETHING((fwrite(*in_buffer, 1, size_ptr, file) == size_ptr) ? ERR_NONE : ERR_IO, fclose(file));
    M_REQ(fclose(file) == 0, ERR_IO, "couldn't close file in write_disk_image");

    return ERR_NONE;
}

#define APPEND_CHARS 16
#define FORMAT_CHARS 5
/**
 * @brief Concatenates image id and resolution in one string
 *
 * @param imgID ID of image to save
 * @param resolution Resolution code of the image to be saved
 * @param buff Content of the image
 * @return error code, ERR_NONE if no error happened
 */
static int create_name(const char *imgID, const char *resolution, char *buff) {

    size_t length = strlen(imgID) + strlen(resolution) + FORMAT_CHARS;
    M_REQ(snprintf(buff, length + 1, "%s_%s.jpg", imgID, resolution) == length, ERR_INVALID_IMGID,
          "error in createname : name concatenation failed");
    return ERR_NONE;
}


/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int do_list_cmd(int args, char *argv[]) {

    M_REQ(!(args < 2), ERR_NOT_ENOUGH_ARGUMENTS, "Not enough args provided for list");
    const char *filename = argv[1];
    M_REQUIRE_NON_NULL(filename);

    struct imgst_file myfile;
    EXECUTE_COMMAND(filename, "rb", myfile, (do_list(&myfile, STDOUT), ERR_NONE));
}

/************************************************************************/

#define do_create_parse_optionN(N) \
static int do_create_parse_option ## N(int args, char* argv[], size_t *i, size_t nb_options, int error_code, uint ## N ## _t max, uint ## N ## _t *opts[]) { \
    M_REQ(*i + nb_options < args, ERR_NOT_ENOUGH_ARGUMENTS, "not enough args for option in create (args too small)"); \
    for (size_t k = 0; k < nb_options; ++k) { \
        *opts[k] = atouint ## N(argv[*i + k + 1]); \
        M_REQ(0 < *opts[k] && *opts[k] <= max, error_code, "arg out of bounds for option in create"); \
    } \
    *i = *i + nb_options + 1; \
    return ERR_NONE; \
}

/**
 * @brief Parses one option + its argument(s) of do_create_cmd
 *
 * @param args Number of total program arguments
 * @param argv Program arguments (char*[])
 * @param i Number of arguments read, to output in this pointer
 * @param error_code Error to signal
 * @param max Max value of uint16_t to be read
 * @param opts Arguments parsed, to output in this pointer
 *
 * @return some error code, ERR_NONE if none happened
 */
do_create_parse_optionN(16)

/**
 * @brief Parses one option + its argument(s) of do_create_cmd
 *
 * @param args Number of total program arguments
 * @param argv Program arguments (char*[])
 * @param i Number of arguments read, to output in this pointer
 * @param error_code Error to signal
 * @param max Max value of uint32_t to be read
 * @param opts Arguments parsed, to output in this pointer
 *
 * @return some error code, ERR_NONE if none happened
 */
do_create_parse_optionN(32)

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int args, char *argv[]) {

    M_REQ(args >= 2, ERR_NOT_ENOUGH_ARGUMENTS, "not enough args provided for create at start of function");
    const char *filename = argv[1];
    M_REQUIRE_NON_NULL(filename);

    uint32_t max_files = DEFAULT_MAX_FILES;
    uint16_t thumb_res_x = DEFAULT_RES_THUMB;
    uint16_t thumb_res_y = DEFAULT_RES_THUMB;
    uint16_t small_res_x = DEFAULT_RES_SMALL;
    uint16_t small_res_y = DEFAULT_RES_SMALL;
    uint32_t *max_file_tab[1] = {&max_files};
    uint16_t *thumb_res_tab[2] = {&thumb_res_x, &thumb_res_y};
    uint16_t *small_res_tab[2] = {&small_res_x, &small_res_y};

    size_t i = 2;
    while (i < args) {
        const char *option = argv[i];
        int possible_error = ERR_NONE;

        if (strncmp("-max_files", option, 10) == 0) {
            possible_error = do_create_parse_option32(args, argv, &i, 1, ERR_MAX_FILES, MAX_MAX_FILES, max_file_tab);
        } else if (strncmp("-thumb_res", option, 10) == 0) {
            possible_error = do_create_parse_option16(args, argv, &i, 2, ERR_RESOLUTIONS, MAX_RES_THUMB, thumb_res_tab);
        } else if (strncmp("-small_res", option, 10) == 0) {
            possible_error = do_create_parse_option16(args, argv, &i, 2, ERR_RESOLUTIONS, MAX_RES_SMALL, small_res_tab);
        } else {
            return ERR_INVALID_ARGUMENT;
        }

        if (possible_error != ERR_NONE) {
            return possible_error;
        }
    }

    puts("Create");

    imgst_file imgst_file = {
            NULL,
            { .max_files = max_files,
            .res_resized[2 * RES_SMALL] = small_res_x,
            .res_resized[2 * RES_SMALL + 1] = small_res_y,
            .res_resized[2 * RES_THUMB] = thumb_res_x,
            .res_resized[2 * RES_THUMB + 1] = thumb_res_y},
            NULL
    };

    int err_value = do_create(filename, &imgst_file);

    do_close(&imgst_file);
    return err_value;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int help(int args, char *argv[]) {
    printf("imgStoreMgr [COMMAND] [ARGUMENTS]\n");
    printf("  help: displays this help.\n");
    printf("  list <imgstore_filename>: list imgStore content.\n");
    printf("  create <imgstore_filename> [options]: create a new imgStore.\n");
    printf("      options are:\n");
    printf("          -max_files <MAX_FILES>: maximum number of files.\n");
    printf("                                  default value is %d\n", DEFAULT_MAX_FILES);
    printf("                                  maximum value is %d\n", MAX_MAX_FILES);
    printf("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n");
    printf("                                  default value is %dx%d\n", DEFAULT_RES_THUMB, DEFAULT_RES_THUMB);
    printf("                                  maximum value is %dx%d\n", MAX_RES_THUMB, MAX_RES_THUMB);
    printf("          -small_res <X_RES> <Y_RES>: resolution for small images.\n");
    printf("                                  default value is %dx%d\n", DEFAULT_RES_SMALL, DEFAULT_RES_SMALL);
    printf("                                  maximum value is %dx%d\n", MAX_RES_SMALL, MAX_RES_SMALL);
    printf("  read   <imgstore_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n");
    printf("      read an image from the imgStore and save it to a file.\n");
    printf("      default resolution is \"original\".\n");
    printf("  insert <imgstore_filename> <imgID> <filename>: insert a new image in the imgStore.\n");
    printf("  delete <imgstore_filename> <imgID>: delete image imgID from imgStore.\n");
    printf("gc <imgstore_filename> <tmp imgstore_filename>: performs garbage collecting on imgStore. Requires a "
           "temporary filename for copying the imgStore.\n");
    return ERR_NONE;
}

/********************************************************************//**
 * Deletes an image from the imgStore.
 */
int do_delete_cmd(int args, char *argv[]) {
    M_REQ(!(args < 3), ERR_NOT_ENOUGH_ARGUMENTS, "Not enough args provided for delete");
    const char *filename = argv[1];
    const char *imgID = argv[2];
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(imgID);

    const size_t len_img_ID = strlen(imgID);
    M_REQ(0 < len_img_ID && len_img_ID <= MAX_IMG_ID, ERR_INVALID_IMGID, "invalid imgid in do_delete_cmd");

    imgst_file imgst_file;
    EXECUTE_COMMAND(filename, "r+b", imgst_file, do_delete(imgID, &imgst_file));
}

/********************************************************************//**
 * Inserts an image into the imgStore.
 */
int do_insert_cmd(int args, char *argv[]) {

    M_REQ(!(args < 4), ERR_NOT_ENOUGH_ARGUMENTS, "Not enough args provided for insert");

    const char *imgst_filename = argv[1];
    const char *imgID = argv[2];
    const char *disk_filename = argv[3];

    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(imgID);
    M_REQUIRE_NON_NULL(disk_filename);

    char *buffer;
    size_t buffer_size = 0;
    int err_val = 0;
    M_REQ((err_val = read_disk_image(disk_filename, &buffer, &buffer_size)) == ERR_NONE, err_val, "IO error in do_insert");

    imgst_file imgst_file;
    EXECUTE_COMMAND_EXPANDED(imgst_filename, "r+b", imgst_file, do_insert(buffer, buffer_size, imgID, &imgst_file),
                             imgst_file.header.num_files < imgst_file.header.max_files, ERR_MAX_FILES,
                             "imgst_file full in do_insert_cmd", &err_val, 1, buffer);

    FREE(buffer);
    return err_val;
}

/********************************************************************//**
 * Reads an image from the imgStore.
 */
int do_read_cmd(int args, char *argv[]) {

    M_REQ(!(args < 3), ERR_NOT_ENOUGH_ARGUMENTS, "Not enough args provided for read");

    const char *imgst_filename = argv[1];
    const char *imgID = argv[2];
    const char *resolution = (args >= 4) ? argv[3] : "orig";


    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(imgID);
    M_REQUIRE_NON_NULL(resolution);

    char *buffer = NULL;
    uint32_t size;

    imgst_file imgst_file;
    int err;

    M_REQ((err = do_open(imgst_filename, "r+b", &imgst_file)) == ERR_NONE, err, "could not open file in do_read_cmd");
    M_EXIT_IF_ERR_DO_SOMETHING((err = resolution_atoi(resolution)) != ERR_RESOLUTIONS ? ERR_NONE : err, do_close(&imgst_file));
    int size_code = err;

    M_EXIT_IF_ERR_DO_SOMETHING((err = do_read(imgID, err, &buffer, &size, &imgst_file)), do_close(&imgst_file));

    char *disk_image_name = calloc(MAX_IMG_ID + APPEND_CHARS + 1, sizeof(char));
    const char * resolution_names[NB_RES];
    resolution_names[RES_ORIG] = "orig"; resolution_names[RES_THUMB] = "thumb"; resolution_names[RES_SMALL] = "small";
    M_EXIT_IF_ERR_DO_SOMETHING((err = create_name(imgID, resolution_names[size_code], disk_image_name)),
                               GROUP_CALLS(GROUP_CALLS(FREE(disk_image_name), FREE(buffer)), do_close(&imgst_file)));

    M_EXIT_IF_ERR_DO_SOMETHING((err = write_disk_image(disk_image_name, &buffer, size)),
                               GROUP_CALLS(GROUP_CALLS(FREE(disk_image_name), FREE(buffer)), do_close(&imgst_file)));

    FREE(buffer);
    FREE(disk_image_name);

    do_close(&imgst_file);
    return err;
}

int do_gc_cmd(int args, char *argv[]){
    M_REQ(!(args < 3), ERR_NOT_ENOUGH_ARGUMENTS, "Not enough args provided for collect");
    const char *imgst_filename = argv[1];
    const char *tmp_imgst_filename = argv[2];

    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(tmp_imgst_filename);

    int err;
    M_REQ((err = do_gbcollect(imgst_filename, tmp_imgst_filename) == ERR_NONE), err, "could not collect file in do_gc_cmd");

    return err;
}

/************************************************************************/

#define MAX_FUN_NAME_SIZE 32
#define NUM_FUNCTIONS 7

typedef int(*command)(int, char *[]);

/**
 * Database command abstraction.
 */
struct command_mapping {

    /**
     * Name of a command.
     */
    const char name[MAX_FUN_NAME_SIZE];

    /**
     * Corresponding function.
     */
    command function;
};

typedef struct command_mapping command_mapping;

static const command_mapping command_list[NUM_FUNCTIONS] = {
        {"list",   do_list_cmd},
        {"delete", do_delete_cmd},
        {"create", do_create_cmd},
        {"help",   help},
        {"read",   do_read_cmd},
        {"insert", do_insert_cmd},
        {"gc",     do_gc_cmd}
};

/********************************************************************//**
 * MAIN
 */
int main(int argc, char *argv[]) {
    int ret = ERR_NONE;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {

        if (vips_init(argv[0])) {
            vips_error_exit("unable to start vips");
        }

        argc--;
        argv++;
        int function_found = 0;
        for (; function_found < NUM_FUNCTIONS; ++function_found) {
            const char *name = command_list[function_found].name;
            if (!strncmp(name, argv[0], MAX_FUN_NAME_SIZE)) {
                ret = command_list[function_found].function(argc, argv);
                break;
            }
        }

        if (function_found == NUM_FUNCTIONS) {
            ret = ERR_INVALID_COMMAND;
        }

        vips_shutdown();
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help(argc, argv);
    }

    return ret;
}
