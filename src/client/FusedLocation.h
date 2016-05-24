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
	FUSED_LOCATION_ERROR_NONE = 0,          /**< Successful */
	FUSED_LOCATION_ERROR_NOT_READY,         /**< Server connection not established */
	FUSED_LOCATION_ERROR_INVALID_ARGUMENT,  /**< Server connection not established */
	FUSED_LOCATION_ERROR_ANY = -1           /**< Any error */
} fused_location_error_e;

/**
 * @brief Enumeration for the client-server connection status
 */
typedef enum {
	FUSED_LOCATION_STATE_DISABLED = -1,     /**< Connection is inactive */
	FUSED_LOCATION_STATE_ENABLED = 0        /**< Connection is active */
} fused_location_status_e;

/**
 * @brief Enumeration for the accuracy mode
 */
typedef enum {
	FUSED_LOCATION_MODE_NO_POWER = 0,       /**< No strict requirement. Only occasional updates. */
	FUSED_LOCATION_MODE_BALANCED_POWER,     /**< Balance between accuracy and power consumption */
	FUSED_LOCATION_MODE_HIGH_ACCURACY       /**< Highest accuracy and power consumption */
} fused_location_mode_e;

/**
 * @brief The Fused Location proxy handle.
 */
typedef struct fused_location_s *fused_location_h;

/**
 * @brief Called when the client-server proxy is prepared after calling fused_location_prepare()
 * @param[in] state         State of the connection
 * @param[in] user_data		The user data passed from the call registration function
 * @pre fused_location_prepare() registers this callback and prepares the client-server proxy
 * @see fused_location_prepare()
 */
typedef void(*fused_location_state_updated_cb)(fused_location_status_e state,
                                               void *user_data);

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
typedef void(*fused_location_position_updated_cb) (double latitude,
                                                   double longitude,
                                                   double altitude,
                                                   double speed,
                                                   double direction,
                                                   double climb,
                                                   double horizontal_accuracy,
                                                   double vertical_accuracy,
                                                   time_t timestamp,
                                                   void *user_data);

/**
 * @brief Sets the Fused Location position callback.
 *
 * @param[in] handle                The Fused Location handle.
 * @param[in] position_callback     New callback. Use NULL to clear existing callback.
 * @param[in] user_data             Data to be passed to the callback.
 *
 * @return 0 on success, otherwise a negative error value
 * @see fused_location_position_updated_cb()
 */
int fused_location_set_position_callback(fused_location_h handle,
                                         fused_location_position_updated_cb position_callback,
                                         void *user_data);

/**
 * @brief Sets the Fused Location state callback.
 *
 * @param[in] handle                The Fused Location handle.
 * @param[in] state_callback        New callback. Use NULL to clear existing callback.
 * @param[in] user_data             Data to be passed to the callback.
 *
 * @return 0 on success, otherwise a negative error value
 * @see fused_location_state_updated_cb()
 */
int fused_location_set_state_callback(fused_location_h handle,
                                      fused_location_state_updated_cb state_callback,
                                      void *user_data);

/**
 * @brief Prepares fused location proxy.
 *
 * This function should be called before any other fused location methods. If state callback is
 * set using fused_location_set_state_callback() it is invoked to pass the result.
 *
 * @param[in] handle                        The Fused Location handle.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE       Success
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @post location_service_state_changed_cb().
 * @see fused_location_position_updated_cb()
 */
int fused_location_create(fused_location_h *handle);

/**
 * @brief Starts the fused location subscribtion.
 *
 * @param[in] handle                        The Fused Location handle.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE       Success
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @post It invokes fused_location_position_updated_cb(), fused_location_state_updated_cb().
 * @see fused_location_position_updated_cb()
 */
int fused_location_start(fused_location_h handle);

/**
 * @brief Stops the Fused Location subscribtion.
 *
 * @param[in] handle                        The Fused Location handle.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE       Success
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @see fused_location_start()
 */
int fused_location_stop(fused_location_h handle);

/**
 * @brief Destroys the Fused Location handle.
 *
 * @param[in] handle                The Fused Location handle.
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE       Success
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @see fused_location_start()
 */
int fused_location_destroy(fused_location_h handle);

/**
 * @brief Sets the Fused Location accuracy mode
 *
 * @param[in] handle                        The Fused Location handle.
 * @param[in] mode                          Accuracy mode
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #FUSED_LOCATION_ERROR_NONE       Success
 * @retval #FUSED_LOCATION_ERROR_ANY        An error occured
 * @see fused_location_start()
 * @see fused_location_mode_e
 */
int fused_location_set_mode(fused_location_h handle, fused_location_mode_e mode);

#ifdef __cplusplus
}
#endif
