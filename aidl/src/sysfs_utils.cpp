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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LENGTH 128

static int sysfs_read(char *path, char *out) {
    int ret;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    ret = read(fd, out, MAX_LENGTH - 1);
    if (ret) {
        return -2;
    }

    out[MAX_LENGTH - 1] = '\0';
    if (close(fd)) {
        return -3;
    }

    return 0;
}

[[maybe_unused]]
static int sysfs_read_int(char *path, int *out) {
    char buf[128];
    int ret = sysfs_read(path, buf);
    if(ret) {
        return ret;
    }

    *out = atoi(buf);

    return 0;
}

static int sysfs_write_size(char *path, char *buf, size_t size) {
    int ret;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        return -1;
    }

    if (strlen(buf) > size) buf[size - 1] = '\0';

    ret = write(fd, buf, size);
    if (ret) {
        return -2;
    }

    if (close(fd)) {
        return -3;
    }

    return 0;
}

[[maybe_unused]]
static int sysfs_write(char *path, char *buf) {
    return sysfs_write_size(path, buf, MAX_LENGTH);
}
