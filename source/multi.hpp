//Copyright © 2022 Charles Kerr. All rights reserved.

#ifndef multi_hpp
#define multi_hpp

#include <cstdint>
#include <string>
#include <vector>
#include <ostream>
#include <map>
#include <utility>
#include <fstream>
#include <filesystem>

#include "uop.hpp"
//=================================================================================
//  multi_component_t ;
//=================================================================================
//=================================================================================
struct multi_component_t {
    static constexpr auto mul_record_size = 16 ;
    std::uint16_t tileid ;
    std::int16_t offsetx ;
    std::int16_t offsety ;
    std::int16_t offsetz ;
    std::uint64_t flag ;
    std::vector<std::uint32_t> cliloc;
    multi_component_t():tileid(0xFFFF),offsetx(0),offsety(0),offsetz(0),flag(0){}
    multi_component_t(const std::string &entry) ;
    auto operator<(const multi_component_t &value) const ->bool ;
    auto description() const ->std::string ;
    auto loaduop(const std::uint8_t *data) ->int ;
    auto loadmul(const std::uint8_t *data) ->int ;
    auto data(bool isuop = true) const ->std::vector<std::uint8_t> ;
};
//=================================================================================
//=================================================================================
//  multi_t ;
//=================================================================================
struct multi_t {
    std::vector<multi_component_t> data ;
    multi_t() = default ;
    multi_t(const std::vector<std::uint8_t> &bytes, bool isuop) ;
    multi_t(std::vector<std::string> text) ;
    multi_t(const std::filesystem::path &csvfile);
    auto size() const ->std::int32_t ;
    auto empty() const ->bool { return data.empty();}
    auto operator()(std::int32_t index) const -> const multi_component_t& ;
    auto operator()(std::int32_t index) ->multi_component_t& ;
    auto record(bool isuop) const ->std::vector<std::uint8_t> ;
    auto description(std::ostream &output) const ->void ;
};
//=================================================================================
//  multistorage_t ;
//=================================================================================
class multistorage_t {
private:
    table_entry housing_location ;
    std::map<std::uint32_t,table_entry> entry_location ;
    
    mutable std::ifstream datafile ;
    std::filesystem::path indexfile ;
    bool isuop ;
    
    
    auto retrieve_uopaccess(std::ifstream &uopfile) ->void ;
    auto retrieve_idxaccess(std::ifstream  &idxfile) ->void ;
    static auto gatherTextMulti(const std::filesystem::path &path)  -> std::map<std::uint32_t,std::filesystem::path> ;

public:
    static auto saveUOP(const std::filesystem::path &csvdirectory ,const std::filesystem::path &uopfile, const std::filesystem::path &housingpath=std::filesystem::path("housing.bin"))->void ;
    static auto saveMUL(const std::filesystem::path &csvdirectory, const std::filesystem::path &mulfile, const std::filesystem::path &indexfile)->void ;
    multistorage_t(const std::filesystem::path &datafile, const std::filesystem::path &indexfile=std::filesystem::path()) ;
    multistorage_t()=default ;
    auto uop() const ->bool {return isuop;}
    auto maxid() const ->std::uint32_t ;
    auto saveHousing(const std::filesystem::path &filepath) const ->void ;
    auto housing() const ->std::vector<std::uint8_t> ;
    auto save(const std::filesystem::path &datapath,const std::filesystem::path &idxpath=std::filesystem::path(),const std::vector<std::uint8_t> &housingdata = std::vector<std::uint8_t>()) ->void ;
    auto operator[](std::uint32_t index) const -> multi_t ;
};



#endif /* multi_hpp */
