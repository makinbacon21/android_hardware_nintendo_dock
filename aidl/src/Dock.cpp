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
#include <sys/epoll.h>
#include <utils/Log.h>

#include <fstream>
#include <sstream>
#include <map>

#include "Dock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace nintendo {
namespace dock {
namespace implementation {

volatile bool polling;
pthread_mutex_t mLock;

ScopedAStatus Dock::setPowerMode(PowerMode mode) {
    profile = mode;  // TODO: impl
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
        supportedModes[index].push_back(stoi(tokens[2]));

        if (tokens.size() == 4)  // emc supported var???? prolly not
            supportedModes[index].push_back(stoi(tokens[4]));
    }

    return 0;
}

/**
 * Loosely referencing https://suchprogramming.com/epoll-in-3-easy-steps/ and
 * IUsb default implementation
 */
void *pollWork([[maybe_unused]]void *param) {
    ALOGI("Polling thread successfully launched\n");
    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        ALOGE("Failed to create epoll file descriptor\n");
        return NULL;
    }

    int fd = open(SYSFS_POWERSUPPLY, O_RDONLY | O_NONBLOCK);

    event.events = EPOLLIN;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)) {
        ALOGE("Failed to add file descriptor to epoll\n");
        close(epoll_fd);
        return NULL;
    }

    pthread_mutex_lock(&mLock);
    while (polling) {
        pthread_mutex_unlock(&mLock);
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
        ALOGI("Ready events: %d\n", event_count);

        for (int i = 0; i < event_count; i++) {
            ALOGI("Reading file descriptor '%d' -- \n", events[i].data.fd);
            char read_buffer[READ_SIZE+1];
            size_t bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
            ALOGI("%zd bytes read.\n", bytes_read);
            read_buffer[bytes_read] = '\0';
            ALOGI("Read '%s'\n", read_buffer);
        }
        pthread_mutex_lock(&mLock);
    }
    pthread_mutex_unlock(&mLock);

    if (close(epoll_fd)) {
        ALOGE("Failed to close epoll file descriptor\n");
        return NULL;
    }

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

}  // namespace implementation
}  // namespace dock
}  // namespace nintendo
}  // namespace hardware
}  // namespace android
