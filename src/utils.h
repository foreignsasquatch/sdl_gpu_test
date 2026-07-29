#include "SDL3/SDL.h"
#include "SDL3/SDL_stdinc.h"

#pragma once
#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#define HANDMADE_MATH_NO_SIMD
#define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS
#define HMM_SINF SDL_sinf
#define HMM_COSF SDL_cosf
#define HMM_TANF SDL_tanf
#define HMM_ACOSF SDL_acosf
#define HMM_SQRTF SDL_sqrtf

#include "HandmadeMath.h"

typedef struct Vector2 {
    float x, y;
} Vector2;

typedef struct Vector3 {
    float x, y, z;
} Vector3;

typedef struct Vector4 {
    float x, y, z, w;
} Vector4;

typedef struct Matrix4x4 {
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;
} Matrix4x4;

typedef struct Array {
    void* array;
    size_t elementSize;
    size_t size;
    size_t used;
} Array;

typedef struct ObjFaceIndex {
    Uint16 pos;
    Uint16 uv;
} ObjFaceIndex;

typedef struct ObjData {
    Array positions;
    Array uvs;
    Array faces;
} ObjData;

void initArray(Array* a, size_t elemSize, size_t initSize);
void pushArray(Array* a, void* element);
void freeArray(Array* a);

void loadObj(ObjData* obj, const char* filename);
void unloadObj(ObjData* obj);

#endif
