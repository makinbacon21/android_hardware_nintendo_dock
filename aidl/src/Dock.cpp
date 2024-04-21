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

#define LOG_TAG "android.hardware.nintendo.dock"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <fcntl.h>
#include <hardware/hardware.h>
#include <pthread.h>
#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <utils/Log.h>

#include <fstream>
#include <sstream>
#include <map>
#include <regex>

#include "sysfs_utils.h"
#include "Dock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace nintendo {
namespace dock {

volatile bool polling;
pthread_mutex_t mLock;

/**
 * 130|nx:/sys $ cat ./devices/system/cpu/cpufreq/policy0/                                                      
 * affected_cpus                       related_cpus                        scaling_governor
 * cpuinfo_cur_freq                    scaling_available_frequencies       scaling_max_freq
 * cpuinfo_max_freq                    scaling_available_governors         scaling_min_freq
 * cpuinfo_min_freq                    scaling_cur_freq                    scaling_setspeed
 * cpuinfo_transition_latency          scaling_driver                      stats/
 * 
 * performance	Run the CPU at the maximum frequency, obtained from /sys/devices/system/cpu/cpuX/cpufreq/scaling_max_freq.
 * powersave	Run the CPU at the minimum frequency, obtained from /sys/devices/system/cpu/cpuX/cpufreq/scaling_min_freq.
 * userspace	Run the CPU at user specified frequencies, configurable via /sys/devices/system/cpu/cpuX/cpufreq/scaling_setspeed.
 * ondemand	Scales the frequency dynamically according to current load. Jumps to the highest frequency and then possibly back off as the idle time increases.
 * conservative	Scales the frequency dynamically according to current load. Scales the frequency more gradually than ondemand.
 * schedutil	Scheduler-driven CPU frequency selection [4], [5].
*/

/**
 * nx:/sys # cat ./devices/57000000.gpu/                                                                        
 * aelpg_enable          elcg_enable           gpc_fs_mask           nvidia-gpu-power/     slcg_enable
 * aelpg_param           elpg_enable           gpc_pg_mask           nvidia-gpu-v2-power/  subsystem/
 * allow_all             emc3d_ratio           gpu_powered_on        nvidia-gpu-v2/        tpc_fs_mask
 * blcg_enable           emulate_mode          iommu_group/          nvidia-gpu/           tpc_pg_mask
 * comptag_mem_deduct    enable_3d_scaling     is_railgated          of_node/              tsg_timeslice_max_us
 * counters              fbp_fs_mask           ldiv_slowdown_factor  power/                tsg_timeslice_min_us
 * counters_reset        fbp_pg_mask           load                  ptimer_ref_freq       uevent
 * devfreq/              fmax_at_vmin_safe     mig_mode_config       ptimer_scale_factor   user
 * devfreq_dev/          force_idle            mig_mode_config_list  ptimer_src_freq
 * driver/               freq_request          modalias              railgate_delay
 * driver_override       golden_img_status     mscg_enable           railgate_enable

*/

int Dock::__setPowerMode(PowerMode mode) {
    if(supportedModes.find(mode) == supportedModes.end()) {
        ALOGE("Mode not defined! Check your config file.\n");
    }

    ALOGI("Setting cpu max freq <%d>\n", supportedModes[mode][0]);
    sysfs_write(cpufreqPath + "/scaling_max_freq", std::to_string(supportedModes[mode][0]));
    ALOGI("Setting gpu max freq <%d>\n", supportedModes[mode][1]);
    sysfs_write(gpuDevfreqPath + "/max_freq", std::to_string(supportedModes[mode][1]));
    profile = mode;
    return 0;
}

int Dock::__clearForcedFreq() {
    freqForced = false;
    ALOGI("Resetting cpufreq gov to schedutil\n");
    sysfs_write(cpufreqPath + "/scaling_governor", "schedutil");
    ALOGI("Resetting gpu devfreq gov to nvhost_podgov\n");
    sysfs_write(gpuDevfreqPath + "/governor", "nvhost_podgov");
    return 0;
}

ScopedAStatus Dock::setPowerMode(PowerMode mode) {
    int ret;

    if(freqForced)
        __clearForcedFreq();

    if (mode != profile) {
        ret = __setPowerMode(mode);
        if (ret) return ScopedAStatus::fromServiceSpecificError(ret);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus Dock::forceModeFreq(PowerMode mode) {
    int ret;
    freqForced = true;

    if (mode != profile) {
        ret = __setPowerMode(mode);
        if (ret) return ScopedAStatus::fromServiceSpecificError(ret);
    }

    ALOGI("Forcing frequency!\n");

    sysfs_write(cpufreqPath + "/scaling_governor", "performance");
    sysfs_write(gpuDevfreqPath + "/governor", "userspace");
    sysfs_write(gpuDevfreqPath + "/userspace/set_freq", std::to_string(supportedModes[mode][1]));
    return ScopedAStatus::ok();
}

ScopedAStatus Dock::getPowerMode(PowerMode *_aidl_return) {
    *_aidl_return = profile;
    return ScopedAStatus::ok();
}

ScopedAStatus Dock::getAvailableModes(std::vector<PowerMode> *_aidl_return) {
    for (std::map<PowerMode, vector<int32_t>>::iterator it =
             supportedModes.begin();
         it != supportedModes.end(); it++) {
        _aidl_return->push_back(it->first);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Dock::getAvailableCpuFreqs(std::vector<int32_t> *_aidl_return) {
    for (std::map<PowerMode, vector<int32_t>>::iterator it =
             supportedModes.begin();
         it != supportedModes.end(); it++) {
        _aidl_return->push_back(it->second[1]);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Dock::getAvailableGpuFreqs(std::vector<int32_t> *_aidl_return) {
    for (std::map<PowerMode, vector<int32_t>>::iterator it =
             supportedModes.begin();
         it != supportedModes.end(); it++) {
        _aidl_return->push_back(it->second[0]);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Dock::getDockedState(bool *_aidl_return) {
    *_aidl_return = docked;
    return ScopedAStatus::ok();
}

int Dock::parseConfig() {
    std::ostringstream path;
    path << CONFIG_PREFIX << "dock." << hardware << "." << sku << ".txt";
    std::ifstream file(path.str());

    std::string line;
    while (getline(file, line)) {
        if (line[0] == '#')  // comment
            continue;

        if (line.empty())  // ignore blanks
            continue;

        std::stringstream lstream(line);
        std::string term;
        std::vector<string> tokens;

        while (getline(lstream, term, ' ')) {
            tokens.push_back(term);
        }

        if (tokens.size() > 4 || tokens.size() < 3) { /* only emc is optional */
            return 1;
        }

        PowerMode index = PowerMode(stoi(tokens[0]));

        supportedModes.insert(std::pair<PowerMode, std::vector<int32_t>>(
            index, std::vector<int32_t>()));

        supportedModes[index].push_back(stoi(tokens[1]));
        supportedModes[index].push_back(stoi(tokens[2]) * 1000); // GPU freqs require conversion

        ALOGI("Added profile idx <%d>, cpu freq <%d>, gpu freq <%d>\n", stoi(tokens[0]), stoi(tokens[1]), stoi(tokens[2]));

        if (tokens.size() == 4) {  // emc supported var???? prolly not
            supportedModes[index].push_back(stoi(tokens[3]));
            ALOGI(".. emc freq <%d>\n", stoi(tokens[3]));
        }
    }

    return 0;
}

struct data {
    int uevent_fd;
    Dock *dock;
};

static void uevent_event(uint32_t /*epevents*/, struct data *payload) {
    const std::string usbExtconPath = "/sys/devices/usb_cd/extcon/extcon3";
    char msg[UEVENT_MSG_LEN + 2];
    char *cp;
    int n;

    n = uevent_kernel_multicast_recv(payload->uevent_fd, msg, UEVENT_MSG_LEN);
    if (n <= 0) return;
    if (n >= UEVENT_MSG_LEN) /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {
        if (std::regex_match(cp, std::regex(".*usb_cd.*"))) {
            ALOGI("USB-C event detected\n");

            int out;
            if(sysfs_read_int(usbExtconPath + "/cable.0/state", &out)) {
                ALOGE("ERROR: Failed to read cable state!\n");
                break;
            }

            if(out) {
                payload->dock->setPowerMode(PowerMode::MAX_PERF);
                docked = true;
            } else {
                payload->dock->setPowerMode(PowerMode::ECO);
                docked = false;
            }
            
            break;
        }
        while (*cp++) {
        }
    }
}

/**
 * Loosely referencing https://suchprogramming.com/epoll-in-3-easy-steps/ and
 * IUsb default implementation
 */
void *pollWork(void *param) {
    ALOGI("Polling thread successfully launched\n");
    int epoll_fd, uevent_fd;
    struct epoll_event ev;
    int nevents = 0;
    struct data payload;

    uevent_fd = uevent_open_socket(UEVENT_MAX_EVENTS * UEVENT_MSG_LEN, true);

    if (uevent_fd < 0) {
        ALOGE("uevent_init: uevent_open_socket failed\n");
        return NULL;
    }

    payload.uevent_fd = uevent_fd;
    payload.dock = (Dock *)param;

    fcntl(uevent_fd, F_SETFL, O_NONBLOCK);

    ev.events = EPOLLIN;
    ev.data.ptr = (void *)uevent_event;

    epoll_fd = epoll_create(UEVENT_MAX_EVENTS);
    if (epoll_fd == -1) {
        ALOGE("epoll_create failed; errno=%d", errno);
        goto error;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uevent_fd, &ev) == -1) {
        ALOGE("epoll_ctl failed; errno=%d", errno);
        goto error;
    }

    while (polling) {
        struct epoll_event events[UEVENT_MAX_EVENTS];

        nevents = epoll_wait(epoll_fd, events, UEVENT_MAX_EVENTS, -1);
        if (nevents == -1) {
            if (errno == EINTR) continue;
            ALOGE("usb epoll_wait failed; errno=%d", errno);
            break;
        }

        for (int n = 0; n < nevents; ++n) {
            if (events[n].data.ptr)
                (*(void (*)(int, struct data *payload))events[n].data.ptr)(
                    events[n].events, &payload);
        }
    }

    ALOGI("exiting worker thread");
error:
    close(uevent_fd);

    if (epoll_fd >= 0) close(epoll_fd);

    return NULL;
}

Dock::Dock() {
    ALOGI("Starting DockService...\n");

    hardware = base::GetProperty("ro.hardware", "");

    sku = base::GetProperty("ro.boot.hardware.sku", "");
    if (hardware.empty() || sku.empty()) {
        ALOGE(
            "ERROR: No sku detected. Ensure ro.hardware and ro.boot.hardware.sku \
                are getting set.\n");
        return;  // TODO: kill self
    }
    ALOGI("Detected sku: %s / %s", hardware.c_str(), sku.c_str());

    if (parseConfig()) {
        ALOGE("ERROR: Failed to parse config. Check syntax.\n");
        return;  // TODO: kill self
    }

    profile = PowerMode::HOS_STOCK;

    // CHANGEME: check if docked
    docked = false;

    pthread_mutex_init(&mLock, NULL);

    // start polling thread i guess
    polling = true;
    if (pthread_create(&mPoll, NULL, pollWork, this)) {
        ALOGE("pthread creation failed %d", errno);
    }
}

Dock::~Dock() {
    ALOGE("DockService shutting down.\n");

    pthread_mutex_lock(&mLock);
    polling = false;
    pthread_mutex_unlock(&mLock);

    if (!pthread_kill(mPoll, SIGUSR1)) {
        pthread_join(mPoll, NULL);
        ALOGI("pthread destroyed");
    }

    pthread_mutex_destroy(&mLock);
}

}  // namespace dock
}  // namespace nintendo
}  // namespace hardware
}  // namespace android
