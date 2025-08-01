/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef BASE_EVENTHANDLER_FRAMEWORKS_ANI_DESERIALIZE_H
#define BASE_EVENTHANDLER_FRAMEWORKS_ANI_DESERIALIZE_H

#include <memory>
#include "serialize.h"

namespace OHOS {
namespace AppExecFwk {

class AniDeserialize {
public:
    /**
     * Deserialize from ani to ani.
     *
     * @param env A pointer to the environment structure.
     * @param peerData Object to store deserialized data.
     * @param serializeData Object to store serialized data.
     * @return Returns true if deserialize successfully.
     */
    static bool PeerDeserialize(ani_env* env, ani_ref* peerData, std::shared_ptr<SerializeData> serializeData);

    /**
     * Deserialize from napi to ani.
     *
     * @param env A pointer to the environment structure.
     * @param crossData Object to store deserialized data.
     * @param serializeData Object to store serialized data.
     * @return Returns true if deserialize successfully.
     */
    static bool CrossDeserialize(ani_env* env, ani_ref* crossData, std::shared_ptr<SerializeData> serializeData);
};

} // namespace AppExecFwk
} // namespace OHOS

#endif  // BASE_EVENTHANDLER_FRAMEWORKS_ANI_DESERIALIZE_H