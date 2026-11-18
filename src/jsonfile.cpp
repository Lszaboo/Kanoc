#include "kanoc/jsonfile.hpp"

#include <fstream>
#include <ostream>
#include <sstream>

#include "kanoc/cmd_helpers.hpp"

void JSONFile::Load(const std::string& path){
    CMD_PRINT(CMD_BRACKETS(name, CMD_BOLDGREEN) << ": loading \"" << path << "\"...");
    existed = false;
    std::ifstream fi(path);

    if (!fi.is_open()) {
        // File doesn't exist, start with empty json.
        j = {};
    }else{
        std::stringstream sstream;
        sstream << fi.rdbuf();
        j = json::parse(sstream.str());
        existed = true;
    }

    fi.close();
}

void JSONFile::Save(const std::string& path){
    CMD_PRINT(CMD_BRACKETS(name, CMD_BOLDGREEN) << ": saving \"" << path << "\"...");
    std::ofstream f(path);

    std::string jsonStr = j.dump(4) + "\n";
    f << jsonStr;

    f.close();
}
