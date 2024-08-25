#ifndef ASTC_CACHE_H
#define ASTC_CACHE_H

#include <inttypes.h>
#include <stdbool.h>

#ifdef ANDROID
#include <cutils/log.h>
#define TCC_LOGD(...)  ALOGD(__VA_ARGS__)
#define TCC_LOGE(...)  ALOGE(__VA_ARGS__)
#define ATSC_TCC_BASE_DIR "/data/data"
#else
#define TCC_LOGD(...) //fprintf(stdout, __VA_ARGS__);fprintf(stdout,"\n")
#define TCC_LOGE(...) fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n")
#define ATSC_TCC_BASE_DIR "/tmp"
#endif

#define MAX_CACHE_TYPE     100
#define MAX_SLOTS_PER_TYPE 1024
#define MIN_TEX_SIZE_POW2  8
#define MAX_TEX_SIZE_POW2  12

struct astc_tcc_cache_slot {
    uint64_t key;
    uint32_t offset;
    uint32_t size;
    uint32_t levels;
    uint8_t* base;
    uint8_t* mipmap;
};

struct astc_tcc_cache {
    uint16_t type;
    uint16_t count;
    uint32_t base_size;
    uint32_t mipmap_size;
    uint32_t total_size;

    struct astc_tcc_cache_slot slots[MAX_SLOTS_PER_TYPE];
};

struct astc_tcc_mgr {
    bool enabled;
    uint16_t type_count;
    char cache_dir[256];
    uint32_t total_alloc;
    struct astc_tcc_cache caches[MAX_CACHE_TYPE];
};

bool astc_tcc_can_cache(uint32_t w, uint32_t h);
uint8_t astc_tcc_get_max_level(uint32_t w);
uint16_t astc_tcc_make_type(uint16_t fmt, uint8_t max_level);
uint64_t astc_tcc_hash(uint8_t* data, uint32_t size);
uint8_t* astc_tcc_get(uint32_t type, uint8_t level, uint64_t key, uint32_t* stride, uint32_t* height);
int astc_tcc_put(uint16_t type, uint8_t level, uint64_t key, uint8_t* data, uint32_t stride);

#endif

