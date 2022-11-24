//Copyright © 2022 Charles Kerr. All rights reserved.

#ifndef argument_hpp
#define argument_hpp

#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#include <utility>
//=================================================================================
// 
struct argument_t {
    std::vector<std::pair<std::string,std::string>> flags ;
    std::vector<std::filesystem::path> paths ;
    argument_t(int argc, const char * argv[] );
};

#endif /* argument_hpp */
