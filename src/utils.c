#include "utils.h"
#include "SDL3/SDL_stdinc.h"

void initArray(Array* a, size_t elemSize, size_t initSize) {
    a->array = SDL_malloc(initSize * elemSize);
    if(a->array == NULL) {
        SDL_Log("Failed to allocate array");
    }
    a->used = 0;
    a->size = initSize;
    a->elementSize = elemSize;
}

void pushVec2ToArray(Array* a, HMM_Vec2 element) {
    if(a->used == a->size) {
        a->size *= 2;
        a->array = SDL_realloc(a->array, a->size * a->elementSize);
        if(a->array == NULL) {
            SDL_Log("Failed to reallocate array");
        }
    }
    SDL_memcpy(&((HMM_Vec2*)a->array)[a->used], &element, a->elementSize);    
    a->used++;
}

void pushVec3ToArray(Array* a, HMM_Vec3 element) {
    if(a->used == a->size) {
        a->size *= 2;
        a->array = SDL_realloc(a->array, a->size * a->elementSize);
        if(a->array == NULL) {
            SDL_Log("Failed to reallocate array");
        }
    }
    SDL_memcpy(&((HMM_Vec3*)a->array)[a->used], &element, a->elementSize);    
    a->used++;
}

void pushFaceIndexToArray(Array* a, ObjFaceIndex element) {
    if(a->used == a->size) {
        a->size *= 2;
        a->array = SDL_realloc(a->array, a->size * a->elementSize);
        if(a->array == NULL) {
            SDL_Log("Failed to reallocate array");
        }
    }
    SDL_memcpy(&((ObjFaceIndex*)a->array)[a->used], &element, a->elementSize);    
    a->used++;
}


void freeArray(Array* a) {
    SDL_free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

void parseObjPositions(Array *a, char* str) {
    char* temp;
    char* token = SDL_strtok_r(str, " ", &temp);
    int count = 0;
    float x, y, z;
    while(token != NULL) {
        if(count == 1) SDL_sscanf(token, "%f", &x); 
        if(count == 2) SDL_sscanf(token, "%f", &y); 
        if(count == 3) SDL_sscanf(token, "%f", &z); 
        token = SDL_strtok_r(NULL, " ", &temp);
        count++;
    }
    pushVec3ToArray(a, (HMM_Vec3){x, y, z});
    //HMM_Vec3 v = ((HMM_Vec3*)a->array)[a->used-1];
    //SDL_Log("%f %f %f", v.X, v.Y, v.Z);
}

void parseObjUvs(Array *a, char* str) {
    char* temp;
    char* token = SDL_strtok_r(str, " ", &temp);
    int count = 0;
    float x, y;
    while(token != NULL) {
        if(count == 1) SDL_sscanf(token, "%f", &x); 
        if(count == 2) SDL_sscanf(token, "%f", &y); 
        token = SDL_strtok_r(NULL, " ", &temp);
        count++;
    }
    pushVec2ToArray(a, (HMM_Vec2){x, y});
}

ObjFaceIndex parseObjFaceIndex(char* str) {
    char* temp;
    char* token = SDL_strtok_r(str, "/", &temp);
    int count = 0;
    unsigned int v, u;
    while(token != NULL) {
        if(count == 0) SDL_sscanf(token, "%u", &v);
        if(count == 1) SDL_sscanf(token, "%u", &u);
        token = SDL_strtok_r(NULL, "/", &temp);
        count++;
    }
    return (ObjFaceIndex){.pos = v-1, .uv = u-1};
}

void parseObjFaces(Array *a, char* str) {
    char* temp;
    char* token = SDL_strtok_r(str, " ", &temp);
    int count = 0;
    ObjFaceIndex x, y, z;
    while(token != NULL) {
        if(count == 1) x = parseObjFaceIndex(token);
        if(count == 2) y = parseObjFaceIndex(token);
        if(count == 3) z = parseObjFaceIndex(token);
        token = SDL_strtok_r(NULL, " ", &temp);
        count++; 
    }
    //SDL_Log("%i %i", x.pos, x.uv);
    pushFaceIndexToArray(a, x);
    pushFaceIndexToArray(a, y);
    pushFaceIndexToArray(a, z);
}

// NOTE: Does not exit when file is not found (bad idea)
void loadObj(ObjData* obj, const char* filename) {
    size_t fileSize;
    char* fileContent = SDL_LoadFile(filename, &fileSize);
    if(fileContent == NULL) {
        SDL_Log("Failed to load OBJ file from: %s", filename);
    }

    initArray(&(obj->positions), sizeof(HMM_Vec3), 2);
    initArray(&(obj->uvs), sizeof(HMM_Vec2), 2);
    initArray(&(obj->faces), sizeof(ObjFaceIndex), 2);

    char* smth;
    char* token = SDL_strtok_r(fileContent, "\n", &smth);
    while(token != NULL) {
        if(SDL_strstr(token, "v ") != NULL) {
            parseObjPositions(&(obj->positions), token);
        } else if(SDL_strstr(token, "vt") != NULL) {
            parseObjUvs(&(obj->uvs), token);
        } else if(SDL_strstr(token, "f") != NULL) {
            parseObjFaces(&(obj->faces), token);
        }

        token = SDL_strtok_r(NULL, "\n", &smth);
    }

    SDL_free(fileContent);
}

void unloadObj(ObjData* obj) {
    freeArray(&obj->positions);
    freeArray(&obj->uvs);
    freeArray(&obj->faces);
}
