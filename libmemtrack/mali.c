/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mman.h>

#include <hardware/memtrack.h>

#include "memtrack_exynos5.h"

/* Set a 512 MB limit */
#define MAX_MEMTRACK_SIZE 0x20000000

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

int mali_memtrack_get_memory(pid_t pid, int type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
    size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));
    struct dirent *de;
    DIR *dp;
    FILE *fp;
    char pid_str[8];
    char mali_file[32];
    char line[1024];
    int len;
    int total = 0;
    int native_buffer = 0;

    *num_records = ARRAY_SIZE(record_templates);

    /* fastpath to return the necessary number of records */
    if (allocated_records == 0) {
        return 0;
    }

    dp = opendir("/d/mali/mem/");
    if (dp == NULL) {
        return -errno;
    }

    len = snprintf(pid_str, sizeof(pid_str), "%d", pid);
    if (len <= 0) {
        closedir(dp);
        return -EINVAL;
    }

    for (de = readdir(dp); de != NULL; de = readdir(dp)) {
        if (strncmp(de->d_name, pid_str, len) != 0) {
            continue;
        }

        snprintf(mali_file, sizeof(mali_file), "/d/mali/mem/%s", pid_str);
        break;
    }
    closedir(dp);

    fp = fopen(mali_file, "r");
    if (fp == NULL) {
        return -errno;
    }

    memcpy(records, record_templates,
           sizeof(struct memtrack_record) * allocated_records);

    while (1) {
        int rc;
        char label1[16] = { 0 };
        char label2[16] = { 0 };
        unsigned int size = 0;

        if (fgets(line, sizeof(line), fp) == NULL) {
            break;
        }

        /* Format:
         * TODO: list out format, use more descriptive names
         */
        rc = sscanf(line, "%s %*s %*s %u \n", label1, &size);
        if (rc == 2) {
            if (strcmp(label1, "Total") == 0) {
                if (total + size > MAX_MEMTRACK_SIZE) {
                    break;
                }
                total += size;
            }
            continue;
        }

        /* Format:
         * TODO: list out format, use more descriptive names
         */
        rc = sscanf(line, "%*s %s %s %*s %*s %u \n", label1, label2, &size);
        if (rc == 3) {
            if (strcmp(label1, "Native") == 0 &&
                strcmp(label2, "Buffer") == 0) {
                if (native_buffer + size > MAX_MEMTRACK_SIZE) {
                    break;
                }
                native_buffer += size;
            }
        }
    }

    records[0].size_in_bytes = native_buffer;

    if (allocated_records == 2 && total >= native_buffer) {
        records[1].size_in_bytes = total - native_buffer;
    }

    fclose(fp);

    return 0;
}
