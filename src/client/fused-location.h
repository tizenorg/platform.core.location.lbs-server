/*
 *  Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @file
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Fused Location API
 */

#pragma once

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration for error codes for Fused Location.
 */
typedef enum {
    FUSED_LOCATION_ERROR_NONE = 0,      /**< Successful */
    FUSED_LOCATION_ERROR_ANY = -1       /**< Any error */
} fused_location_error_e;


/**
 * @brief Enumeration for the accuracy mode
 */
typedef enum {
    FUSED_LOCATION_MODE_NO_POWER = 0,	/**< No strict requirement. Only occasional updates. */
    FUSED_LOCATION_MODE_BALANCED_POWER,	/**< Balance between accuracy and power consumption */
    FUSED_LOCATION_MODE_HIGH_ACCURACY   /**< Highest accuracy and power consumption */
} fused_location_mode_e;

/**
 * @brief Called with updated position information.
 * @param[in] latitude		The updated latitude [-90.0 ~ 90.0] (degrees)
 * @param[in] longitude		The updated longitude [-180.0 ~ 180.0] (degrees)
 * @param[in] altitude		The updated altitude (meters)
 * @param[in] timestamp		The timestamp (time when measurement took place or @c 0 if valid)
 * @param[in] user_data		The user data passed from the call registration function
 * @pre fused_location_start() registers this callback and subscribes to position updates
 * @see fused_location_start()
 */
typedef void(*fused_location_position_updated_cb)(double latitude,
                                                  double longitude,
                                                  double altitude,
                                                  time_t timestamp,
                                                  void *user_data);

/**
 * @brief Starts the fused location subscribtion.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE		Successful
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @post It invokes location_position_updated_cb(), location_service_state_changed_cb().
 * @see fused_location_position_updated_cb()
 */
int fused_location_start(fused_location_position_updated_cb position_callback,
                         void *user_data);

/**
 * @brief Stops the fused location subscribtion.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE		Successful
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @see fused_location_start()
 */
int fused_location_stop();

/**
 * @brief Stops the fused location subscribtion.
 *
 * @param[in] mode		                    Accuracy mode
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE		Successful
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @see fused_location_start()
 * @see fused_location_mode_e
 */
int fused_location_set_mode(fused_location_mode_e mode);

#ifdef __cplusplus
}
#endif
