#include "astc_cache.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef ANDROID
#include <bsd/stdlib.h>
#endif
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "util/format/u_formats.h"

static struct astc_tcc_mgr* get_tcc_mgr(void);

static inline uint16_t make_type(uint16_t fmt, uint8_t max_level) {
    return (fmt - PIPE_FORMAT_ASTC_4x4) + (max_level << 8);
}

static inline uint16_t get_format(uint16_t type) {
    return PIPE_FORMAT_ASTC_4x4 + (type & 0xff);
}

static inline uint16_t get_max_level(uint16_t type) {
    return (type >> 8);
}

static inline bool is_valid_type(uint16_t type) {
    return (get_format(type) <= PIPE_FORMAT_ASTC_12x12_SRGB) &&
            (get_max_level(type) >= MIN_TEX_SIZE_POW2) &&
            (get_max_level(type) <= MAX_TEX_SIZE_POW2);
}

static inline uint16_t get_level_4x4_width(uint16_t type, uint8_t level) {
    uint16_t max_level = get_max_level(type);
    uint16_t p2 = (max_level - level) > 2 ? (max_level - level - 2) : 0;

    return 1 << p2;
}

static inline uint16_t get_level_4x4_height(uint16_t type, uint8_t level) {
    return get_level_4x4_width(type, level);
}

static inline uint16_t get_level_4x4_stride(uint16_t type, uint8_t level) {
    uint16_t fmt = get_format(type);
#if 0 //current shader path is always dxt5
    bool to_dxt5 = (fmt >= PIPE_FORMAT_ASTC_4x4 && fmt <= PIPE_FORMAT_ASTC_6x5) ||
            (fmt >= PIPE_FORMAT_ASTC_4x4_SRGB && fmt <= PIPE_FORMAT_ASTC_6x5_SRGB);
#else
    bool to_dxt5 = true;
#endif
    uint32_t blk4x4_size = to_dxt5 ? 16 : 8;
    uint16_t blk4x4_width = get_level_4x4_width(type, level);

    return blk4x4_width * blk4x4_size;
}

static inline uint32_t get_level_size(uint16_t type, uint8_t level) {
    return get_level_4x4_stride(type, level) * get_level_4x4_height(type, level);
}

static inline uint32_t get_level_offset(uint16_t type, uint8_t level) {
    uint32_t offset = 0;

    for (uint8_t i = 1; i < level; i++) {
        offset += get_level_size(type, i);
    }
    return offset;
}

static inline uint32_t get_data_size(uint16_t type) {
    uint32_t size = 0;
    uint8_t max_level = get_max_level(type);

    for (uint8_t i = 0; i <= max_level; i++) {
        size += get_level_size(type, i);
    }
    return size;
}

static inline uint32_t get_base_size(uint16_t type) {
    return get_level_size(type, 0);
}

static inline uint32_t get_mipmap_size(uint16_t type) {
    uint32_t size = 0;
    uint8_t max_level = get_max_level(type);

    for (uint8_t i = 1; i <= max_level; i++) {
        size += get_level_size(type, i);
    }
    return size;
}

static inline struct astc_tcc_cache* get_cache(struct astc_tcc_mgr* mgr, uint16_t type) {
    int i;

    for (i = 0; i< mgr->type_count; i++) {
        if (type == mgr->caches[i].type)
            return &mgr->caches[i];
    }
    return NULL;
}

static inline int get_cache_slot(struct astc_tcc_cache* pc, uint64_t key) {
    int i;

    for (i = 0; i < pc->count; i++) {
        if (key == pc->slots[i].key)
            return i;
    }
    return -1;
}

static inline bool valid_cache_level(struct astc_tcc_cache* pc, int slot, uint8_t level) {
    return pc->slots[slot].levels & (1 << level);
}

static inline uint64_t checksum(const void * pixels, uint32_t size) {
   uint64_t sum = 0;
   const uint64_t* p = (const uint64_t*)pixels;
   uint32_t sizeInU64 = size / 8;
   for (uint32_t i = 0; i < sizeInU64; i++) {
      sum += p[i];
   }
   return sum;
}

static int astc_tcc_load_key(struct astc_tcc_mgr* mgr, uint16_t type, struct astc_tcc_cache* pc) {
    if (!mgr || !pc)
        return -1;

    int err = -1;
    FILE* fp = 0;
    char path[256];

    snprintf(path, 256, "%s/key-%.4x", mgr->cache_dir, type);
    fp = fopen(path, "rb");
    if (!fp) {
        TCC_LOGE("failed to open %s", path);
        goto out;
    }
    size_t n;
    n = fread(pc, 4, 4, fp);
    if (n < 4) {
        TCC_LOGE("Too few data in key file %s", path);
        goto out;
    }
    // verify type
    if (pc->type != type) {
        TCC_LOGE("Mismatched data type for %s", path);
        goto out;
    }
    // verify size
    if (pc->base_size != get_base_size(type) || pc->mipmap_size != get_mipmap_size(type)) {
        TCC_LOGE("mismatch data size for %s", path);
        goto out;
    }
    // verify data size
    if (pc->count == 0 || pc->total_size < pc->count * pc->base_size) {
        TCC_LOGE("Too few data in %s", path);
    }
    // check slot count
    if (pc->count > MAX_SLOTS_PER_TYPE) {
        TCC_LOGE("Too many slot in %s", path);
        goto out;
    }

    uint32_t slot_size = offsetof(struct astc_tcc_cache_slot, base);
    for (uint16_t i = 0; i < pc->count; i++) {
        n = fread(&pc->slots[i], slot_size, 1, fp);
        if (n < 1) {
            TCC_LOGE("Too few keys in %s", path);
            goto out;
        }
    }
    err = 0;
    //TCC_LOGD("load %s successfully", path);
out:
    if (fp)
        fclose(fp);
    return err;
}

static int astc_tcc_load_data(struct astc_tcc_mgr* mgr, uint16_t type, struct astc_tcc_cache* pc) {
    if (!mgr || !pc)
        return -1;

    FILE* fp = 0;
    char path[256];

    snprintf(path, 256, "%s/data-%.4x", mgr->cache_dir, type);
    fp = fopen(path, "rb");
    if (!fp) {
        TCC_LOGE("failed to open %s", path);
        goto out;
    }

    size_t n;
    uint64_t key = 0;

    for (uint16_t i = 0; i < pc->count; i++) {
        uint32_t fp_off = ftell(fp);
        if (fp_off != pc->slots[i].offset) {
            TCC_LOGE("Mismatch read offset, cur=%d, expect=%d", fp_off, pc->slots[i].offset);
        }
        n = fread(&key, 8, 1, fp);
        if (n < 1) {
            TCC_LOGE("Failed to read key for slot %d from %s", i, path);
            goto out;
        }
        if (key != pc->slots[i].key) {
           TCC_LOGE("Mismatch key read for slot %d from %s", i, path);
            goto out;
        }
        pc->slots[i].base = (uint8_t*)malloc(pc->base_size);
        if (!pc->slots[i].base) {
            TCC_LOGE("Failed to alloc astc cache base memory for slot %d in %s, OOM", i, path);
            goto out;
        }
        mgr->total_alloc += pc->base_size;
        n = fread(pc->slots[i].base, pc->base_size, 1, fp);
        if (n < 1) {
            TCC_LOGE("Failed to read slot %d from %s", i, path);
            goto out;
        }
        pc->slots[i].levels = 1;
        if (pc->slots[i].size == pc->base_size + pc->mipmap_size) {
            pc->slots[i].mipmap = (uint8_t*)malloc(pc->mipmap_size);
            if (!pc->slots[i].mipmap) {
                TCC_LOGE("Failed to alloc astc cache mipmap memory for slot %d in %s, OOM", i, path);
                goto out;
            }
            mgr->total_alloc += pc->mipmap_size;
            n = fread(pc->slots[i].mipmap, pc->mipmap_size, 1, fp);
            if (n < 1) {
                TCC_LOGE("Failed to read slot %d from %s", i, path);
                goto out;
            }
            pc->slots[i].levels = (1 << (get_max_level(type) + 1)) - 1;
        }
    }

    TCC_LOGD("load %d texs from %s successfully", pc->count, path);
    fclose(fp);
    return pc->count;
out:
    if (fp)
        fclose(fp);
    return -1;
}

static int astc_tcc_store_key(struct astc_tcc_mgr* mgr, uint16_t type, struct astc_tcc_cache* pc) {
    if (!mgr || !pc)
        return -1;

    FILE* fp = 0;
    char path[256];
    mode_t def_mask = umask(0);

    snprintf(path, 256, "%s/key-%.4x", mgr->cache_dir, type);
    fp = fopen(path, "wb");
    if (!fp) {
        TCC_LOGE("Failed to open %s for write", path);
        umask(def_mask);
        return -1;
    }

    size_t n;
    uint32_t slot_size = offsetof(struct astc_tcc_cache_slot, base);

    // write the head
    n = fwrite(pc, 4, 4, fp);
    for (uint16_t i = 0; i < pc->count; i++) {
        n = fwrite(&pc->slots[i], slot_size, 1, fp);
    }

    fclose(fp);
    umask(def_mask);
    return n;
}

static int astc_tcc_store_data_base(struct astc_tcc_mgr* mgr, uint16_t type, int slot, struct astc_tcc_cache* pc) {
    if (!mgr || !pc)
        return -1;

    FILE* fp = 0;
    char path[256];
    mode_t def_mask = umask(0);

    snprintf(path, 256, "%s/data-%.4x", mgr->cache_dir, type);
    fp = fopen(path, "ab+");
    if (!fp) {
        TCC_LOGE("Failed to open %s for write", path);
        umask(def_mask);
        return -1;
    }

    // write the data
    uint32_t data_size = get_base_size(type);

    fseek(fp, pc->slots[slot].offset, SEEK_SET);
    fwrite(&pc->slots[slot].key, 8, 1, fp);
    fwrite(pc->slots[slot].base, data_size, 1, fp);
    pc->total_size += (8 + data_size);

    fclose(fp);
    umask(def_mask);
    return 0;
}

static int astc_tcc_store_data_mipmap(struct astc_tcc_mgr* mgr, uint16_t type, int slot, struct astc_tcc_cache* pc) {
    if (!mgr || !pc)
        return -1;

    FILE* fp = 0;
    char path[256];
    mode_t def_mask = umask(0);

    snprintf(path, 256, "%s/data-%.4x", mgr->cache_dir, type);
    fp = fopen(path, "ab+");
    if (!fp) {
        TCC_LOGE("Failed to open %s for write", path);
        umask(def_mask);
        return -1;
    }

    // write the data
    uint32_t offset = pc->slots[slot].offset + 8 + get_base_size(type);
    uint32_t data_size = get_mipmap_size(type);

    fseek(fp, offset, SEEK_SET);
    fwrite(pc->slots[slot].mipmap, data_size, 1, fp);
    pc->total_size += data_size;

    fclose(fp);
    return 0;
}

static int astc_tcc_load_type(struct astc_tcc_mgr* mgr, uint16_t type) {
    if (!mgr || !mgr->enabled)
        return -1;

    if (mgr->type_count >= MAX_CACHE_TYPE) {
        TCC_LOGE("%s: Too many tex type (%d) to cache", __func__, mgr->type_count);
        return -1;
    }

    struct astc_tcc_cache* pc = &mgr->caches[mgr->type_count];
    if (astc_tcc_load_key(mgr, type, pc) < 0) {
        return -1;
    }
    if (astc_tcc_load_data(mgr, type, pc) < 0) {
       return -1;
    }
    mgr->type_count++;
    return pc->count;
}

static int astc_tcc_store_base(struct astc_tcc_mgr* mgr, uint16_t type, int slot) {
    if (!mgr || !mgr->enabled)
        return -1;

    struct astc_tcc_cache* pc = get_cache(mgr, type);
    if (!pc) {
        TCC_LOGD("Can't find the cache to store");
        return -1;
    }

    astc_tcc_store_data_base(mgr, type, slot, pc);
    astc_tcc_store_key(mgr, type, pc);

    return pc->count;
}

static int astc_tcc_store_mipmap(struct astc_tcc_mgr* mgr, uint16_t type, int slot) {
    if (!mgr)
        return -1;

    struct astc_tcc_cache* pc = get_cache(mgr, type);
    if (!pc) {
        TCC_LOGD("Can't find the cache to store");
        return -1;
    }

    astc_tcc_store_data_mipmap(mgr, type, slot, pc);
    astc_tcc_store_key(mgr, type, pc);

    return pc->count;
}

bool astc_tcc_can_cache(uint32_t w, uint32_t h) {
    return (w == h) &&
        (((w - 1) & w) == 0) &&
        (w  >= (1 << MIN_TEX_SIZE_POW2)) &&
        (w  <= (1 << MAX_TEX_SIZE_POW2));
}

uint8_t astc_tcc_get_max_level(uint32_t w) {
    for (uint8_t l = MIN_TEX_SIZE_POW2; l <= MAX_TEX_SIZE_POW2; l++) {
        if (w == (1 << l))
            return l;
    }
    return 0;
}

uint16_t astc_tcc_make_type(uint16_t fmt, uint8_t max_level) {
    return make_type(fmt, max_level);
}

uint64_t astc_tcc_hash(uint8_t* data, uint32_t size) {
    TCC_LOGE("%s: Calculate hash for %p size %u", __func__, data, size);
    return checksum(data, size);
}

uint8_t* astc_tcc_get(uint32_t type, uint8_t level, uint64_t key, uint32_t* stride, uint32_t* height) {
    struct astc_tcc_mgr* mgr = get_tcc_mgr();
    if (!mgr || !mgr->enabled)
        return 0;

    struct astc_tcc_cache* pc = get_cache(mgr, type);
    if (!pc) {
        TCC_LOGD("%s: No cache item found for %.4x", __func__, type);
        return 0;
    }
    int slot = get_cache_slot(pc, key);
    if (slot < 0) {
        TCC_LOGD("%s: No cache slot found for %.4x key=%" PRIx64 " ", __func__, type, key);
        return 0;
    }
    if (!valid_cache_level(pc, slot, level)) {
        TCC_LOGD("%s: No cache level found for %.4x key=%" PRIx64 " level=%d", __func__, type, key, level);
        return 0;
   }
    *stride = get_level_4x4_stride(type, level);
    *height = get_level_4x4_height(type, level);

    return (level > 0) ? pc->slots[slot].mipmap + get_level_offset(type, level) : pc->slots[slot].base;
}

int astc_tcc_put(uint16_t type, uint8_t level, uint64_t key, uint8_t* data, uint32_t stride) {
    struct astc_tcc_mgr* mgr = get_tcc_mgr();
    if (!mgr || !mgr->enabled)
        return -1;

//TCC_LOGD("%s type=%.4x level=%d key=%" PRIx64 " data=%p stride=%d",  __func__, type, level, key, data, stride);

    uint8_t max_level = get_max_level(type);
    struct astc_tcc_cache* pc = get_cache(mgr, type);
    if (!pc) {
        if (mgr->type_count >= MAX_CACHE_TYPE) {
            TCC_LOGD("Too many tex type to be cached");
            return -1;
        }
        pc = &mgr->caches[mgr->type_count++];
        TCC_LOGD("%s: New data type %.4x added", __func__, type);
        pc->type = type;
        pc->count = 0;
        pc->base_size = get_base_size(type);
        pc->mipmap_size = get_mipmap_size(type);
        pc->total_size = 0;
    }
    int slot = get_cache_slot(pc, key);
    if (slot < 0) {
        if (pc->count >= MAX_SLOTS_PER_TYPE) {
            TCC_LOGE("%s: Too many tex for %.4x to be cached", __func__, type);
            return -1;
        }
        slot = pc->count++;
        TCC_LOGD("%s: New slot %d for key %" PRIx64 " in type %.4x added", __func__, slot, key, type);
        pc->slots[slot].key = key;
        pc->slots[slot].offset = pc->total_size;
        pc->slots[slot].levels = 0;
        pc->slots[slot].mipmap = 0;
        pc->slots[slot].base = (uint8_t*)malloc(pc->base_size);
        if (!pc->slots[slot].base) {
            TCC_LOGE("%s: Failed to alloc base memory for %.4x, key=%" PRIx64 " OOM", __func__, type, key);
            return -1;
        }
        mgr->total_alloc += pc->base_size;
        pc->slots[slot].size = pc->base_size;
    }
    if (level > 0 && !pc->slots[slot].mipmap) {
        pc->slots[slot].mipmap = (uint8_t*)malloc(pc->mipmap_size);
        if (!pc->slots[slot].mipmap) {
            TCC_LOGE("%s: Failed to alloc mipmap memory for %.4x, key=%" PRIx64 " OOM", __func__, type, key);
            return -1;
        }
        mgr->total_alloc += pc->mipmap_size;
        pc->slots[slot].size += pc->mipmap_size;
    }

    uint8_t* l4x4_data = (level > 0) ? pc->slots[slot].mipmap + get_level_offset(type, level) : pc->slots[slot].base;
    uint32_t l4x4_stride = get_level_4x4_stride(type, level);
    uint32_t l4x4_height = get_level_4x4_height(type, level);

    if(l4x4_stride == stride) {
        memcpy(l4x4_data, data, stride * l4x4_height);
    } else {
        for (uint32_t i = 0; i < l4x4_height; i++) {
            memcpy(l4x4_data, data, l4x4_stride);
            l4x4_data += l4x4_stride;
            data += stride;
        }
    }
    pc->slots[slot].levels |= (1 << level);

    if (level == 0) {
        astc_tcc_store_base(mgr, type, slot);
    } else if (level == max_level) {
        astc_tcc_store_mipmap(mgr, type, slot);
    }
    return slot;
}

static DIR* open_cache_dir(struct astc_tcc_mgr* mgr) {
    if (!mgr)
        return 0;

    bool exist = true;
    mode_t def_mask = umask(0);

    snprintf(mgr->cache_dir, 256, ATSC_TCC_BASE_DIR "/%s", getprogname());
    if (access(mgr->cache_dir, F_OK | R_OK | W_OK | X_OK)) {
        exist = false;
        mkdir(mgr->cache_dir, 0777);
    }
    snprintf(mgr->cache_dir, 256, ATSC_TCC_BASE_DIR "/%s/astc_tcc", getprogname());
    if (access(mgr->cache_dir, F_OK | R_OK | W_OK | X_OK)) {
        exist = false;
        if (mkdir(mgr->cache_dir, 0777) == 0) {
            TCC_LOGD("Create astc cache dir %s", mgr->cache_dir);
        }
    }
    umask(def_mask);

    if (access(mgr->cache_dir, F_OK | R_OK | W_OK | X_OK) == 0) {
        mgr->enabled = true;
    }
    return exist ? opendir(mgr->cache_dir) : 0;
}

static int load_cache_dir(struct astc_tcc_mgr* mgr) {
    if (!mgr)
        return -1;

    int count = 0;
    DIR* dir = open_cache_dir(mgr);
    if (dir) {
        struct dirent* entry;
        struct timespec ts, te;
        int count = 0;
        uint64_t load_time;

        clock_gettime(CLOCK_MONOTONIC, &ts);

        while ((entry = readdir(dir)) != NULL) {
            uint16_t type;
            if (sscanf(entry->d_name, "key-%hx", &type) == 1 && is_valid_type(type)) {
                int n = astc_tcc_load_type(mgr, type);
                if (n > 0) {
                    count += n;
                }
            }
       }
        closedir(dir);

        clock_gettime(CLOCK_MONOTONIC, &te);
        load_time = (te.tv_sec - ts.tv_sec) * 1000000 + (te.tv_nsec - ts.tv_nsec) / 1000;
        TCC_LOGD("astc_cache load %d in %" PRId64 "us, total %d bytes memory used",
                count, load_time, mgr->total_alloc);
    }
    return count;
}

static struct astc_tcc_mgr* get_tcc_mgr() {
    static struct astc_tcc_mgr* s_mgr = NULL;
    if (!s_mgr) {
        s_mgr = (struct astc_tcc_mgr*)calloc(sizeof(struct astc_tcc_mgr), 1);
        if (!s_mgr) {
            TCC_LOGE("Failed to create astc_tcc_mgr, out of memory!");
            return NULL;
        }
        s_mgr->total_alloc = sizeof(struct astc_tcc_mgr);

        load_cache_dir(s_mgr);
    }
    return s_mgr;
}

