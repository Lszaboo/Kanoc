#pragma once

#include "cmdhelpers.hpp"
#include "vkwrappers/buffers.hpp"
#include "vkwrappers/descriptors.hpp"
#include "vkwrappers/devices.hpp"
#include "vkwrappers/pipelines.hpp"
#include "vkwrappers/shaders.hpp"
#include "vkwrappers/textures.hpp"
#include "namedresource.hpp"
#include "vkwrappers/uniforms.hpp"
#include <algorithm>
#include <cassert>
#include <glm/ext/vector_int2.hpp>
#include <iostream>
#include <memory>
#include <slang/slang.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan_core.h>

/*
 * Storage scheme for Vulkan wrappers for the Owner storage class.
 */
template<typename T>
struct ObjectStorageScheme{
    using TT = T;
    using StoreT = std::unique_ptr<T>;
    using ReturnT = T*;

    static StoreT Nullike(){
        return nullptr;
    }

    static void Pack(StoreT& target, T&& ritem) {
        target = std::make_unique<T>(std::move(ritem));
    }

    static ReturnT Return(StoreT& source){
        return source.get();
    }

    static TT Unpack(ReturnT& source){
        return *source;
    }

    static void Destroy(FullDevice& vk, TT& item){
        item.Destroy(vk);
    }
};


/*
 * Storage scheme for Vulkan handles for the Owner storage class.
 */
template<typename T>
struct HandleStorageScheme{
    using TT = T;
    using StoreT = T;
    using ReturnT = T;

    static StoreT Nullike(){
        return VK_NULL_HANDLE;
    }

    static void Pack(StoreT& target, T&& ritem) {
        target = std::move(ritem);
    }

    static ReturnT Return(StoreT& source){
        return source;
    }

    static TT Unpack(ReturnT& source){
        return source;
    }

    static void Destroy(FullDevice& vk, StoreT& source){
        DestroyVkHandle(vk, source);
    }
};

/*
 * The Owner class stores and Destroys by name Vulkan handles or wrappers.
 *
 * The template parameter (PT<T>) should work like this:
 * T ->(PT::Pack)-> StoreT ->(PT::Return)-> ReturnT ->(PT::Unpack)-> T
 *
 * Set log, name and indent to change how this class prints on destruction.
 */
template<typename PT> // PackerT
class Owner{
    protected:
        using TT = PT::TT; // Type the User puts in Add as an rval.
        using StoreT = PT::StoreT; // The Owner will store the items by this type.
        using ReturnT = PT::ReturnT; // The Owner will return from get and Add by this type.

        std::unordered_map<std::string, StoreT> storage;
        FullDevice* vk = nullptr;

        void DestroyAllWithMSG(){
            printDestroyMsg(name, indent, DestroyAll());
        }

        static void printDestroyMsg(const std::string& name, const bool& indent, const int amount){
            if (indent) {
                CMD_PRINT(CMD_TAB << CMD_BRACKETS(name, CMD_BLUE) << ": deleted " << CMD_COLORED(amount, CMD_RED) << " items!");
            }else{
                CMD_PRINT(CMD_BRACKETS(name, CMD_BLUE) << ": deleted " << CMD_COLORED(amount, CMD_RED) << " items!");
            }
        }
    public:
        bool log = true;
        std::string name = "NoName";
        bool indent = true;

        void BindDevice(FullDevice* vk){
            this->vk = vk;
        }

        FullDevice* device(){
            return vk;
        }

        Named<ReturnT> Add(const std::string& name, TT&& ritem){
            assert(name != "" && "name cannot be ""!");

            // Put Nullike value at key if possible.
            auto [keyval, inserted] = storage.try_emplace(name, PT::Nullike());
            if (!inserted) {
                // Element was already in map.
                std::cerr
                    << "Already had a value with same name in storage! Deleting item with bound device: \""
                    << name << "\"..."
                << '\n';

                // Vk Destroy the item.
                PT::Destroy(*vk, ritem);
                return Named<ReturnT>();
            }

            // since T&& has a name (ritem) it becomes lvalue, move needed.
            PT::Pack(keyval->second, std::move(ritem));

            auto nr = Named<ReturnT>();
            nr.name = name;
            nr.resource = PT::Return(keyval->second);

            return nr;
        }

        int size(){
            return storage.size();
        }

        bool Destroy(const std::string& name){
            auto keyval = storage.find(name);

            if (keyval != storage.end()) {
                // Item found for Vk Destroy.
                ReturnT rt = PT::Return(keyval->second);
                TT tt = PT::Unpack(rt);

                PT::Destroy(*vk, tt);
                storage.erase(keyval);
                return true;
            }

            return false;
        }

        int DestroyAll(){
            int amount = 0;

            for (auto& keyval : storage) {
                ReturnT rt = PT::Return(keyval.second);
                TT tt = PT::Unpack(rt);

                PT::Destroy(*vk, tt);
                amount += 1;
            }
            storage.clear();

            return amount;
        }

        ReturnT get(const std::string& name){
            auto keyval = storage.find(name);
            if (keyval != storage.end()){
                return PT::Return(keyval->second);
            }
            StoreT nl = PT::Nullike();
            return PT::Return(nl);
        }

        // Named<> compatible functions

        bool get(Named<ReturnT>& nr){
            return get(nr.name, nr.resource);
        }

        void Destroy(Named<ReturnT>& nr){
            Destroy(vk, nr.name);
            nr.name = "";
            nr.resource = PT::ReturnNullike();
        }

        ~Owner(){
            if (log) {
                DestroyAllWithMSG();
            }else{
                DestroyAll();
            }

        }
};

/*
 * Owner that stores only wrappers.
 */
template<typename T>
using WrapperOwner = Owner<ObjectStorageScheme<T>>;

/*
 * Owner that stores only handles.
 */
template<typename T>
using HandleOwner = Owner<HandleStorageScheme<T>>;

class ResourceOwners{
    public:
        std::string name = "NoName";

        WrapperOwner<ObjModel> models;
        WrapperOwner<SamplerTexture> samplertextures;
        WrapperOwner<DepthTexture> depthtextures;
        WrapperOwner<IndexTexture> idxtextures;

        HandleOwner<VkShaderModule> shaders;
        WrapperOwner<GraphicsPipeline> pipelines;

        HandleOwner<VkDescriptorSetLayout> descriptorlayouts;
        WrapperOwner<Descriptors> descriptorpools;

        WrapperOwner<ShaderDataBuffer> uniforms;

        void Init(FullDevice* vk){
            models.BindDevice(vk);
            models.name = "models";

            samplertextures.BindDevice(vk);
            samplertextures.name = "samplertextures";

            depthtextures.BindDevice(vk);
            depthtextures.name = "depthtextures";

            idxtextures.BindDevice(vk);
            idxtextures.name = "idxtextures";

            shaders.BindDevice(vk);
            shaders.name = "shaders";

            pipelines.BindDevice(vk);
            pipelines.name = "pipelines";

            descriptorlayouts.BindDevice(vk);
            descriptorlayouts.name = "descriptorlayouts";

            descriptorpools.BindDevice(vk);
            descriptorpools.name = "descriptorpools";

            uniforms.BindDevice(vk);
            uniforms.name = "uniforms";
        }

        FullDevice* device(){
            return models.device();
        }

        ~ResourceOwners(){
            CMD_PRINT(CMD_BRACKETS(name, CMD_YELLOW) << ": Destroying all Owners...");
        }
};
