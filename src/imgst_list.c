#include "imgStore.h"

#include <stdbool.h>
#include <json-c/json.h>

/********************************************************************//**
 * @brief Displays (on stdout) imgStore metadata.
 */
char *do_list(const struct imgst_file *imgst_file, do_list_mode mode) {

    M_REQUIRE_CUSTOM_RET(imgst_file != NULL, NULL, /**/);

    switch (mode) {
        case STDOUT:

            print_header(&imgst_file->header);
            bool res = false;

            for (size_t i = 0; i < imgst_file->header.max_files; ++i) {
                if (imgst_file->metadata[i].is_valid == NON_EMPTY) {
                    res = true;
                    print_metadata(&imgst_file->metadata[i]);
                }
            }

            if (!res) {
                fprintf(stdout, "<< empty imgStore >>\n");
            }
            return NULL;

        case JSON: {

            json_object *arr_strings = json_object_new_array();
            M_REQUIRE_CUSTOM_RET(arr_strings != NULL, "", /**/);

            const img_metadata *end = imgst_file->metadata + imgst_file->header.max_files;
            for (img_metadata *curr_meta = imgst_file->metadata; curr_meta < end; ++curr_meta) {
                if (curr_meta->is_valid == NON_EMPTY)
                    M_REQUIRE_CUSTOM_RET(
                            json_object_array_add(arr_strings, json_object_new_string(curr_meta->img_id)) ==
                            JSON_ERR_NONE,
                            "", json_object_put(arr_strings));
            }
            json_object *obj_json = json_object_new_object();
            M_REQUIRE_CUSTOM_RET(obj_json != NULL, "", json_object_put(obj_json));

            M_REQUIRE_CUSTOM_RET(json_object_object_add(obj_json, "Images", arr_strings) == JSON_ERR_NONE, "",
                                 GROUP_CALLS(json_object_put(obj_json), json_object_put(arr_strings)));

            const char *json_string = json_object_to_json_string(obj_json);
            size_t string_length = strlen(json_string);
            char *ret_string = calloc(string_length + 1, sizeof(char));
            M_REQUIRE_CUSTOM_RET(ret_string != NULL, NULL, json_object_put(obj_json));
            strncpy(ret_string, json_string, string_length);

            json_object_put(obj_json);
            return ret_string;
        }

        default:
            return NULL;
    }
}
