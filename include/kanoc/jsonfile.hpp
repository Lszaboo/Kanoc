#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JSONFile{
    public:
        json j;
        bool existed = false;
        std::string name = "";

        void Load(const std::string& path);
        void Save(const std::string& path);
};
