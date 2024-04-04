#include "libmongoose/mongoose.h"
#include "imgStore.h"

static const char *s_listening_address = "http://localhost:8000";

static int s_signo;

static void signal_handler(int signo) {
    s_signo = signo;
}

static const char *s_web_directory = ".";

#define ERROR_STATUS_CODE 500
#define DEF_STATUS_CODE 200

/**
 * @brief Handle wrong arguments
 *
 * @param nc Incoming connection
 * @param error Error code
 */
void mg_error_msg(struct mg_connection* nc, int error) {
    mg_http_reply(nc, ERROR_STATUS_CODE, "Error: ", "%d\n", error);
}

//TODO this trash
static int mg_parse_arg();
/*
 *M_REQUIRE_CUSTOM_RET(res_buffer != NULL,, mg_error_msg(nc, ERR_OUT_OF_MEMORY));
    M_REQUIRE_CUSTOM_RET(mg_http_get_var(&hm->query, "res", res_buffer, RES_STRING_MAX_SIZE + 1) > 0,,
                         GROUP_CALLS(FREE(res_buffer), mg_error_msg(nc, ERR_INVALID_ARGUMENT)));

 */

// ======================================================================
/**
 * @brief Creates shortcut functions to match URIs
 */
#define create_match_cmd(cmd) \
static bool match_ ## cmd(struct mg_http_message *hm) { \
    return mg_http_match_uri(hm, "/imgStore/"#cmd); \
}

create_match_cmd(list)

create_match_cmd(read)

create_match_cmd(delete)

/**
 * @brief Call do_list and send result to incoming connection
 *
 * @param nc Incoming connection
 * @param imgst_file Main data structure
 */
static void handle_list_call(struct mg_connection *nc, imgst_file *imgst_file) {
    mg_http_reply(nc, DEF_STATUS_CODE, "", "%s", do_list(imgst_file, JSON));
}

#define RES_STRING_MAX_SIZE 12
/**
 * @brief Read an image from given database, send result to incoming connection
 *
 * @param nc Incoming connection
 * @param imgst_file Main data structure
 * @param hm HTTP message received
 */
static void handle_read_call(struct mg_connection *nc, imgst_file *imgst_file, struct mg_http_message *hm) {

    char *res_buffer = calloc(RES_STRING_MAX_SIZE + 1, sizeof(char));
    M_REQUIRE_CUSTOM_RET(res_buffer != NULL,, mg_error_msg(nc, ERR_OUT_OF_MEMORY));
    M_REQUIRE_CUSTOM_RET(mg_http_get_var(&hm->query, "res", res_buffer, RES_STRING_MAX_SIZE + 1) > 0,,
                         GROUP_CALLS(FREE(res_buffer), mg_error_msg(nc, ERR_INVALID_ARGUMENT)));
    int size_code = resolution_atoi(res_buffer);
    FREE(res_buffer);


}


/**
 * @brief Handles server events (eg HTTP requests).
 * For more check https://cesanta.com/docs/#event-handler-function
 */
static void imgst_event_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct imgst_file *imgst_file = (struct imgst_file *) fn_data;
    switch (ev) {
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            if (match_list(hm)) {
                handle_list_call(nc, imgst_file);  // Serve REST
            } else if (match_read(hm)) {

            } else {
                struct mg_http_serve_opts opts = {.root_dir = s_web_directory};
                mg_http_serve_dir(nc, ev_data, &opts);
            }
        }
    }
}

// ======================================================================
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: usage: %s imgstore_database\n", argv[0]);
        return 1;
    }
    const char *imgst_filename = argv[1];
    imgst_file database;
    int err;
    M_REQ((err = do_open(imgst_filename, "r+b", &database)) == ERR_NONE, err,
          "could not open file in main_webserver");    /* Create server */

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    M_EXIT_IF_ERR_DO_SOMETHING(mg_http_listen(&mgr, s_listening_address, imgst_event_handler, &database) != NULL ? ERR_NONE : ERR_IO,
                               GROUP_CALLS(mg_mgr_free(&mgr), do_close(&database)));
    printf("Starting imgStore server on %s", s_listening_address);
    print_header(&database.header);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    /* Poll */
    while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
    /* Cleanup */
    mg_mgr_free(&mgr);
    do_close(&database);

    return 0;
}