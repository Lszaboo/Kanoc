#pragma once
#include <string>

/*
 *  Non-owning container to a resource, that has a name in it's owner.
 */
template<typename T>
struct Named{
    std::string name = "";
    T resource;

    bool isEmpty(){
        return name == "";
    }
};
