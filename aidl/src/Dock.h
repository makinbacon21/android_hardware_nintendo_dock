/*
 * Copyright (C) 2022 The Android Open Source Project
 * Copyright (C) 2024 Thomas Makin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/hardware/nintendo/dock/BnDock.h>
#include <algorithm>
#include <vector>

using namespace std;

#define CONFIG_PREFIX "/vendor/etc/"
#define SYSFS_POWERSUPPLY "/sys/class/power_supply/battery/status"
#define MAX_EVENTS 5
#define READ_SIZE 10

#define UEVENT_MSG_LEN     2048
#define UEVENT_MAX_EVENTS  64

namespace android {
namespace hardware {
namespace nintendo {
namespace dock {

using ::aidl::android::hardware::nintendo::dock::BnDock;
using ::aidl::android::hardware::nintendo::dock::IDock;
using ::aidl::android::hardware::nintendo::dock::PowerMode;

struct Dock : public BnDock {
    Dock();
    ~Dock();

    ::ndk::ScopedAStatus setPowerMode(PowerMode mode);

    ::ndk::ScopedAStatus getPowerMode(PowerMode* _aidl_return);

    ::ndk::ScopedAStatus getAvailableModes(vector<PowerMode>* _aidl_return);

    ::ndk::ScopedAStatus getAvailableCpuFreqs(vector<int32_t>* _aidl_return);

    ::ndk::ScopedAStatus getAvailableGpuFreqs(vector<int32_t>* _aidl_return);

    ::ndk::ScopedAStatus getDockedState(bool* _aidl_return);

  private:
    int parseConfig();

    string hardware;
    string sku;
    bool docked;
    map<PowerMode, vector<int32_t>> supportedModes;
    PowerMode profile;
    pthread_t mPoll;
};
}  // namespace dock
}  // namespace nintendo
}  // namespace hardware
}  // namespace android
