/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BASE_EVENTHANDLER_FRAMEWORKS_NAPI_INCLUDE_JS_EMITTER_H
#define BASE_EVENTHANDLER_FRAMEWORKS_NAPI_INCLUDE_JS_EMITTER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "event_handler.h"
#include "event_runner.h"
#include "event_queue.h"
#include "inner_event.h"
#include <unordered_set>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace AppExecFwk {
using Priority = EventQueue::Priority;
static const int32_t ARGC_NUM = 2;
static const int32_t NAPI_VALUE_STRING_LEN = 10240;

enum class DataType: uint32_t {
    BOOL = 0,
    INT,
    STRING,
};

struct Val {
    DataType type;

    union {
        bool bValue;
        int32_t nValue;
        char cValue[NAPI_VALUE_STRING_LEN] = {0};
    } value;
};

napi_value EmitterInit(napi_env env, napi_value exports);
napi_value JS_On(napi_env env, napi_callback_info cbinfo);
napi_value JS_Off(napi_env env, napi_callback_info cbinfo);
napi_value JS_Once(napi_env env, napi_callback_info cbinfo);
napi_value JS_Emit(napi_env env, napi_callback_info cbinfo);

static inline napi_valuetype GetNapiType(napi_env env, napi_value param)
{
    napi_valuetype type;
    napi_typeof(env, param, &type);
    return type;
}
} // namespace AppExecFwk
} // namespace OHOS

#endif  // BASE_EVENTHANDLER_FRAMEWORKS_NAPI_INCLUDE_JS_EMITTER_H
