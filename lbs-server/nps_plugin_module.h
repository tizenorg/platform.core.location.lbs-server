/*
 * lbs-server
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Dong Wei <d.wei@samsung.com>,
 *          Genie Kim <daejins.kim@samsung.com>, Minjune Kim <sena06.kim@samsung.com>,
 *          Ming Zhu <mingwu.zhu@samsung.com>, Congyi Shi <congyi.shi@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _NPS_PLUGIN_MODULE_H_
#define _NPS_PLUGIN_MODULE_H_

#include "nps_plugin_intf.h"

int nps_load_plugin_module(void **plugin_handle);
int nps_unload_plugin_module(void *plugin_handle);
int nps_is_dummy_plugin_module();
const nps_plugin_interface *get_nps_plugin_module(void);

#endif				/* _NPS_PLUGIN_MODULE_H_ */
