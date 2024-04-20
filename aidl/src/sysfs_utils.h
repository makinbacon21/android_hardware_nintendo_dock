#pragma once

int sysfs_read(std::string path, char *out);
int sysfs_read_int(std::string path, int *out);
int sysfs_write_size(std::string path, std::string buf, size_t size);
int sysfs_write(std::string path, std::string buf);
