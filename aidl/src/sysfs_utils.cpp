/*
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

#include <android-base/logging.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <utils/Log.h>

#include <string>

#include "sysfs_utils.h"

#define MAX_LENGTH 128

int sysfs_read(std::string path, char *out) {
    int ret;
    int fd = open(path.c_str(), O_RDONLY);

    if (fd < 0) {
        ALOGE("Failed to open!\n");
        return -1;
    }

    ret = read(fd, out, MAX_LENGTH - 1);
    if (ret < 0) {
        ALOGE("Failed to read!\n");
        return -2;
    }

    out[MAX_LENGTH - 1] = '\0';
    if (close(fd)) {
        ALOGE("Failed to close!\n");
        return -3;
    }

    return 0;
}

[[maybe_unused]]
int sysfs_read_int(std::string path, int *out) {
    char buf[128];
    int ret = sysfs_read(path.c_str(), buf);
    if(ret) {
        return ret;
    }

    *out = atoi(buf);

    return 0;
}

int sysfs_write_size(std::string path, std::string buf, size_t size) {
    int ret;
    int fd = open(path.c_str(), O_WRONLY);

    if (fd < 0) {
        ALOGE("Failed to open! %s %s\n", path.c_str(), strerror(errno));
        return -1;
    }

    if (buf.length() > size) buf[size - 1] = '\0';

    ret = write(fd, buf.c_str(), size);
    if (ret < 0) {
        ALOGE("Failed to write!\n");
        return -2;
    }

    if (close(fd)) {
        ALOGE("Failed to close!\n");
        return -3;
    }

    return 0;
}

[[maybe_unused]]
int sysfs_write(std::string path, std::string buf) {
    return sysfs_write_size(path, buf, MAX_LENGTH);
}
