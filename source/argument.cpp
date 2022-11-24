//Copyright © 2022 Charles Kerr. All rights reserved.

#include "argument.hpp"

#include <iostream>
#include <vector>

#include "strutil.hpp"
using namespace std::string_literals;

//=================================================================================
argument_t::argument_t(int argc,const char * argv[]) {
    
    for (auto i = 1 ; i<argc;i++){
        auto value = std::string(argv[i]);
        if (value.find("--")== 0){
            if (value.size()>2){
                value = value.substr(2) ;
                auto [key,keyvalue] = strutil::split(value,"=");
                flags.push_back(std::make_pair(strutil::lower(key), keyvalue)) ;
           }
        }
        else {
            paths.push_back(std::filesystem::path(value));
        }
     }
}

