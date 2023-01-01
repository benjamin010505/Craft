#include <stdlib.h>
#include <string.h>
#include "map.h"

// 一种hash算法
int hash_int(int key) {
    key = ~key + (key << 15);
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057;
    key = key ^ (key >> 16);
    return key;
}

int hash(int x, int y, int z) {
    x = hash_int(x);
    y = hash_int(y);
    z = hash_int(z);
    return x ^ y ^ z;
}

/**
 * 创建一个Map,这个Map可以看成是HashMap.它的hash算法是根据dx,dy,dz来计算。
 * 这里可以认为HashMap的key为:dx+dy+dz ,value为 dz
 * @param map 返回给调用方
 * @param dx 可以看作key的一部分
 * @param dy 可以看作key的一部分
 * @param dz 可以看作key的一部分
 * @param mask 根据计算出的hash值，使用mask进行&运算，使最终计算出的index不超出mask.决定了HashMap桶数
 */
void map_alloc(Map *map, int dx, int dy, int dz, int mask) {
    map->dx = dx;
    map->dy = dy;
    map->dz = dz;
    map->mask = mask;
    map->size = 0;
    // 分配内存, HashMap中的项是 MapEntry
    // MapEntry用来存储每个数据
    map->data = (MapEntry *)calloc(map->mask + 1, sizeof(MapEntry));
}

/**
 * 释放内存
 * @param map
 */
void map_free(Map *map) {
    // 只有map->data 是堆内存。
    free(map->data);
}

/**
 * 将map src的数据复制到 dst.
 * @param dst
 * @param src
 */
void map_copy(Map *dst, Map *src) {
    dst->dx = src->dx;
    dst->dy = src->dy;
    dst->dz = src->dz;
    dst->mask = src->mask;
    dst->size = src->size;
    dst->data = (MapEntry *)calloc(dst->mask + 1, sizeof(MapEntry));
    // 内存拷贝
    memcpy(dst->data, src->data, (dst->mask + 1) * sizeof(MapEntry));
}

/**
 * 向map中存数据.
 * @param map 调用方持有的map
 * @param x
 * @param y
 * @param z
 * @param w
 * @return
 */
int map_set(Map *map, int x, int y, int z, int w) {
    // 根据x,y,z 进行hash,计算出桶位（index)
    unsigned int index = hash(x, y, z) & map->mask;
    x -= map->dx;
    y -= map->dy;
    z -= map->dz;
    MapEntry *entry = map->data + index;
    int overwrite = 0;
    while (!EMPTY_ENTRY(entry)) { // 出现了Hash碰撞
        if (entry->e.x == x && entry->e.y == y && entry->e.z == z) {
            // 如果x,y,z 都相等说明hashMap的 key相同，这种情况应该覆盖
            overwrite = 1;
            break;
        }
        // 如果碰撞了，放到下一个位置，如果下一个位置仍然冲突，会继续while循环，寻找下一个
        index = (index + 1) & map->mask;
        entry = map->data + index;
    }
    if (overwrite) {
        // 因为key相同，所以要覆盖
        if (entry->e.w != w) {
            entry->e.w = w;
            return 1;
        }
    }
    else if (w) {
        // 未发生hash碰撞,正常put值
        entry->e.x = x;
        entry->e.y = y;
        entry->e.z = z;
        entry->e.w = w;
        map->size++;
        if (map->size * 2 > map->mask) {
            // 如果hashMap中元素超过了mask的 1/2 ，要进行扩容；
            map_grow(map);
        }
        return 1;
    }
    return 0;
}

/**
 * 从Map中获取值
 *
 * x,y,z 可以认为是HashMap的key
 * @param map
 * @param x
 * @param y
 * @param z
 * @return
 */
int map_get(Map *map, int x, int y, int z) {
    unsigned int index = hash(x, y, z) & map->mask;
    x -= map->dx;
    y -= map->dy;
    z -= map->dz;
    if (x < 0 || x > 255) return 0;
    if (y < 0 || y > 255) return 0;
    if (z < 0 || z > 255) return 0;
    MapEntry *entry = map->data + index;
    while (!EMPTY_ENTRY(entry)) {
        if (entry->e.x == x && entry->e.y == y && entry->e.z == z) {
            // 根据key匹配出value
            return entry->e.w;
        }
        index = (index + 1) & map->mask;
        entry = map->data + index;
    }
    return 0;
}

/**
 * map 扩容
 * @param map
 */
void map_grow(Map *map) {
    Map new_map;
    new_map.dx = map->dx;
    new_map.dy = map->dy;
    new_map.dz = map->dz;
    new_map.mask = (map->mask << 1) | 1; // map的大小扩了1倍
    new_map.size = 0;
    // 分配内存
    new_map.data = (MapEntry *)calloc(new_map.mask + 1, sizeof(MapEntry));
    MAP_FOR_EACH(map, ex, ey, ez, ew) { // 循环遍历将老map的值拿出来重新塞到新的map中
        map_set(&new_map, ex, ey, ez, ew);
    } END_MAP_FOR_EACH;
    free(map->data);
    map->mask = new_map.mask;
    map->size = new_map.size;
    map->data = new_map.data;
}
