// TODO: Implement better logging for the whole thing
#include "SDL3/SDL.h"
#include "utils.h"
#include "HandmadeMath.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_GPUShader* loadShader(SDL_GPUDevice* device, const char* filename, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount) {
    SDL_GPUShaderStage stage;
    if(SDL_strstr(filename, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if(SDL_strstr(filename, "frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid Shader Stage");
        return NULL;
    }

    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if(backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if(backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if(backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } else {
        SDL_Log("Unrecognized backend shader format!");
        return NULL;
    }

    size_t codeSize;
    void* code = SDL_LoadFile(filename, &codeSize);
    if(code == NULL) {
        SDL_Log("Failed to load shader from disk: %s", filename);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderCreateInfo = {
        .code = code,
        .code_size = codeSize,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_uniform_buffers = uniformBufferCount,
        .num_storage_buffers = storageBufferCount,
        .num_storage_textures = storageTextureCount
    };
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderCreateInfo);
    if(shader == NULL) {
        SDL_Log("Failed  to create shader: %s", SDL_GetError());
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return shader;
}

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to load SDL: %s", SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("renderer", 1280, 720, 0);
    if(window == NULL) {
        SDL_Log("Failed to create SDL Window: %s", SDL_GetError());
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if(device == NULL) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
    }
    
    if(!SDL_ClaimWindowForGPUDevice(device, window)) {
       SDL_Log("Failed to claim window for GPU device: %s", SDL_GetError()); 
    }

    SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, presentMode);

    SDL_GPUShader* vertexShader = loadShader(device, "content/shader.vert.spv", 0, 1, 0, 0);
    SDL_GPUShader* fragmentShader = loadShader(device, "content/shader.frag.spv", 1, 0, 0, 0);

    int imageWidth, imageHeight;
    void* imageData = stbi_load("content/image.png", &imageWidth, &imageHeight, NULL, 4);
    size_t imageDataByteSize = imageWidth * imageHeight * 4;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo) {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = imageWidth,
        .height = imageHeight,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = 0,
    });

    typedef struct VertexData {
        HMM_Vec3 position;
        HMM_Vec4 color;
        HMM_Vec2 uv;
    } VertexData;

    HMM_Vec4 white = {1, 1, 1, 1};

    VertexData vertices[] = {
        {{-0.5,  0.5, 0}, white, {0, 0}},
        {{ 0.5,  0.5, 0}, white, {1, 0}},
        {{-0.5, -0.5, 0}, white, {0, 1}},
        {{ 0.5, -0.5, 0}, white, {1, 1}},
    };

    Uint16 indices[] = {
        0, 1, 2,
        2, 1, 3
    };

    size_t verticesByteSize = sizeof(vertices);
    size_t indicesByteSize = sizeof(indices);

    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = verticesByteSize,
    });

    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = indicesByteSize,
    });

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = verticesByteSize + indicesByteSize
        }
    );

    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(transferData, vertices, verticesByteSize);
    SDL_memcpy(transferData+verticesByteSize, indices, indicesByteSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = imageDataByteSize
        }
    );

    void* textureTransferData = SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false);
    SDL_memcpy(textureTransferData, imageData, imageDataByteSize);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    SDL_GPUCommandBuffer* copyCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(copyCommandBuffer);

    SDL_UploadToGPUBuffer(copyPass, 
    &(SDL_GPUTransferBufferLocation){
        .transfer_buffer = transferBuffer,
        .offset = 0,
    }, 
    &(SDL_GPUBufferRegion){
        .buffer = vertexBuffer,
        .offset = 0,
        .size = verticesByteSize
    }, false);

    SDL_UploadToGPUBuffer(copyPass, 
    &(SDL_GPUTransferBufferLocation){
        .transfer_buffer = transferBuffer,
        .offset = verticesByteSize,
    }, 
    &(SDL_GPUBufferRegion){
        .buffer = indexBuffer,
        .offset = 0,
        .size = indicesByteSize
    }, false);

    SDL_UploadToGPUTexture(copyPass,
    &(SDL_GPUTextureTransferInfo){
        .transfer_buffer = textureTransferBuffer
    },
    &(SDL_GPUTextureRegion){
        .texture = texture,
        .w = imageWidth,
        .h = imageHeight,
        .d = 1
    }, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyCommandBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &(SDL_GPUSamplerCreateInfo){});

    const SDL_GPUVertexAttribute vertexAttribs[] = {
        {
            .buffer_slot = 0,
            .location = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0
        },
        {
            .buffer_slot = 0,
            .location = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = sizeof(HMM_Vec3)
        },
        {
            .buffer_slot = 0,
            .location = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = sizeof(HMM_Vec3) + sizeof(HMM_Vec4)
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {
                {
                    .slot = 0,
                    .pitch = sizeof(VertexData),
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
                }
            },
            .num_vertex_attributes = sizeof(vertexAttribs) / sizeof(vertexAttribs[0]),
            .vertex_attributes = vertexAttribs
        },
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]){
                {
                    .format = SDL_GetGPUSwapchainTextureFormat(device,window)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    };
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);
    float aspect = (float)width/(float)height;
    
    float rotationSpeed = 90 * HMM_DegToRad;
    float rotation = 0;
    HMM_Mat4 projectionMatrix = HMM_Perspective_RH_NO(70 * HMM_DegToRad, aspect, 0.0001, 1000);

    SDL_Event event;
    bool isRunning = true;

    Uint64 lastTicks = SDL_GetTicks();

    while(isRunning) {
        Uint64 newTicks = SDL_GetTicks();
        float deltaTime = ((float)(newTicks - lastTicks)) / 1000;
        lastTicks = newTicks;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    isRunning = false;
                default:
                    break;
            }
        }

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTexture* swapchainTexture;
    
        if(!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, NULL, NULL)) {
            SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        }

        rotation += rotationSpeed * deltaTime;
        HMM_Mat4 modelMatrix = HMM_MulM4(HMM_Translate((HMM_Vec3){0, 0, -2}), HMM_Rotate_RH(rotation, (HMM_Vec3){0, 1, 0}));
        HMM_Mat4 mvp = HMM_MulM4(projectionMatrix, modelMatrix);

        if(swapchainTexture != NULL) {
            SDL_GPUColorTargetInfo colorTargetInfo = {0};
            colorTargetInfo.clear_color = (SDL_FColor){0, 0, 0, 1};
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
           
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
            
            SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
            SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = vertexBuffer, .offset = 0}, 1);
            SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding){.buffer = indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp.Elements, sizeof(mvp.Elements));
            SDL_BindGPUFragmentSamplers(renderPass, 0, &(SDL_GPUTextureSamplerBinding){.texture = texture, .sampler = sampler}, 1);
            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUBuffer(device, indexBuffer);
    SDL_ReleaseGPUTexture(device, texture);
    SDL_ReleaseGPUSampler(device, sampler);

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
