# Kanoc
A lightweight wrapper for lighting up Vulkan.

## Create build folder.
```bash
cmake -B build
```

## Create excecutable inside build folder.
```bash
cmake --build build
```

## Compile and Run.
```bash
cmake --build build; ./build/VulkanApp
```

## Linux Validation Layer enable:
```bash
export VK_LOADER_LAYERS_ENABLE=*validation
```

## Linux Validation Layer disable:
```bash
unset VK_LOADER_LAYERS_ENABLE
```
