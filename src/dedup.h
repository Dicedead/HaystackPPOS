/**
 * @file dedup.h
 * @brief Optimization function: avoids having 2 images with the same content.
 */
#pragma once

#include "imgStore.h"

/**
 * @brief Avoids content duplication in the img database. If passed image has no duplicate,
 *        RES_ORIG offset is updated to 0.
 *
 * @param imgst_file Database being worked on
 * @param index Index of the image's metadata to de-duplicate
 * @return error_code, ERR_NONE if no error happens
 */
int do_name_and_content_dedup(imgst_file * imgst_file, uint32_t index);
