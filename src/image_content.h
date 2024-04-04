#pragma once

/**
 * @file image_content.h
 * @brief Exports a function (lazily) resizing images from the database
 *
 */

#include "imgStore.h"

/**
 * @brief Create a resized (smaller) version of an image lazily, and store it in the database.
 *
 * @param size_code Encodes the size of the new image. If size_code == RES_ORIG, the function does nothing.
 * @param imgst_file Database to modify.
 * @param position Position of the image to resize in the database.
 * @return (int) Possible error code, ERR_NONE if no error happened
 */
int lazily_resize(int size_code, imgst_file *imgst_file, size_t position);

/**
 * @brief Gets resolution of some input image
 *
 * @param height Pointer to save height
 * @param width Pointer to save width
 * @param image_buffer Pointer to image
 * @param image_size Memory size of the image
 * @return error_code, ERR_NONE if no error happened
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);
