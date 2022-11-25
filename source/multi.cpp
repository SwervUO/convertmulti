//Copyright © 2022 Charles Kerr. All rights reserved.

#include "multi.hpp"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <zlib.h>
#include <sstream>

#include "strutil.hpp"
#include "hash.hpp"


using namespace std::string_literals;
constexpr auto housinghash = 0x126D1E99DDEDEE0ALL ;
constexpr auto idxmax = 8480 ;
const std::string hashformat = "build/multicollection/%.6u.bin"s;
//=================================================================================
auto multi_component_t::operator<(const multi_component_t &value) const ->bool {
    auto rvalue = true ;
    if (offsetx > value.offsetx){
        rvalue = false ;
    }
    else if (value.offsetx == offsetx){
        if (offsety > value.offsety){
            rvalue = false ;
        }
        else if (offsety == value.offsety){
            if (offsetz > value.offsetz){
                rvalue = false ;
            }
        }
    }
    return rvalue ;
}
//=================================================================================
auto multi_component_t::description() const ->std::string {
    std::stringstream output ;
    output << strutil::ntos<std::uint16_t>(tileid,strutil::radix_t::hex,true,4)<<",";
    output<<strutil::ntos<std::int16_t>(offsetx)<<",";
    output<<strutil::ntos<std::int16_t>(offsety)<<",";
    output<<strutil::ntos<std::int16_t>(offsetz)<<",";
    output<<strutil::ntos<std::uint64_t>(flag,strutil::radix_t::hex,true) <<",";
    for (const auto &value:cliloc) {
        output<<strutil::ntos<std::uint32_t>(value)<<":";
    }
    return output.str() ;
}
//===========================================================================
multi_component_t::multi_component_t(const std::string &entry) :multi_component_t() {
    auto comp = strutil::parse(entry,",") ;
    switch (comp.size()){
        default:
        case 6: {
            auto values = strutil::parse(comp.at(5),":") ;
            for (const auto &value:values){
                if (!value.empty()) {
                    cliloc.push_back(strutil::ston<std::uint32_t>(value));
                }
            }
            [[fallthrough]];
        }
        case 5: {
            flag = strutil::ston<std::uint64_t>(comp.at(4));
            [[fallthrough]];
        }
        case 4: {
            offsetz = strutil::ston<std::int16_t>(comp.at(3));
            [[fallthrough]];
        }

        case 3: {
            offsety = strutil::ston<std::int16_t>(comp.at(2));
            [[fallthrough]];
        }
        case 2: {
            offsetx = strutil::ston<std::int16_t>(comp.at(1));
            [[fallthrough]];
        }
        case 1: {
            tileid = strutil::ston<std::uint16_t>(comp.at(0));
            [[fallthrough]];
        }
        case 0:{
            break;
        }
    }
}

//=================================================================================
// Format:
//      std::uint16_t tileid
//      std::int16_t offsetx
//      std::int16_t offsety
//      std::int16_t offsetz
//      std::uint16_t flag      << different from mul
//                              (value of 0, set bit 1 in old flag)
//                              (value of 1, no bits set)
//                              (value of 257, set flag = 0x0000000100000000)(alphablend)
//      std::uint32_t num_cliloc
//      std::uint32_t cliloc[num_cliloc]
//
auto multi_component_t::loaduop(const std::uint8_t *data) ->int {
    auto offset = 0 ;
    std::copy(data,data+2,reinterpret_cast<std::uint8_t*>(&tileid));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsetx));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsety));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsetz));
    auto lflag = std::uint16_t(0) ;
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&lflag));
    switch (lflag) {
        default:
        case 0:
            flag =1 ;
            break;
        case 256:
            flag =0x0000000100000001;
            break;
        case 257:
            flag = 0x0000000100000000;
            break;
        case 1:
            flag = 0 ;
    }
    auto count = std::uint32_t(0);
    offset+=2 ;
    std::copy(data+offset,data+offset+4,reinterpret_cast<std::uint8_t*>(&count));
    cliloc.clear() ;
    auto cli = std::uint32_t(0) ;
    for (std::uint32_t j=0 ; j< count;j++){
        offset+=4 ;
        std::copy(data+offset,data+offset+4,reinterpret_cast<std::uint8_t*>(&cli));
        cliloc.push_back(cli) ;
    }
    return offset+4 ;
}
//=================================================================================
//  Format:
//      std::uint16_t tileid
//      std::uint16_t offsetx
//      std::uint16_t offsety
//      std::uint16_t offsetz
//      std::uint64_t flag
//
auto multi_component_t::loadmul(const std::uint8_t *data) ->int {
    cliloc.clear() ;
    auto offset = 0 ;
    std::copy(data,data+2,reinterpret_cast<std::uint8_t*>(&tileid));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsetx));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsety));
    offset+=2 ;
    std::copy(data+offset,data+offset+2,reinterpret_cast<std::uint8_t*>(&offsetz));
    offset+=2 ;
    std::copy(data+offset,data+offset+8,reinterpret_cast<std::uint8_t*>(&flag));
    offset += 8 ;
    return offset ;
}
//=================================================================================
auto multi_component_t::data(bool isuop ) const ->std::vector<std::uint8_t> {
    auto size = multi_component_t::mul_record_size ;
    if (isuop){
        size = 14 + static_cast<int>(4*cliloc.size()) ;
    }
    auto rvalue = std::vector<std::uint8_t>(size,0) ;
    std::copy(reinterpret_cast<const std::uint8_t*>(&tileid),reinterpret_cast<const std::uint8_t*>(&tileid)+2,rvalue.begin()) ;
    std::copy(reinterpret_cast<const std::uint8_t*>(&offsetx),reinterpret_cast<const std::uint8_t*>(&offsetx)+2,rvalue.begin()+2) ;
    std::copy(reinterpret_cast<const std::uint8_t*>(&offsety),reinterpret_cast<const std::uint8_t*>(&offsety)+2,rvalue.begin()+4) ;
    std::copy(reinterpret_cast<const std::uint8_t*>(&offsetz),reinterpret_cast<const std::uint8_t*>(&offsetz)+2,rvalue.begin()+6) ;
    if (!isuop){
        std::copy(reinterpret_cast<const std::uint8_t*>(&flag),reinterpret_cast<const std::uint8_t*>(&flag)+8,rvalue.begin()+8) ;
    }
    else {
        auto uflag = std::uint16_t(0) ;
        switch (flag) {
            case 1:
                uflag =0 ;
                break;
            case 0x0000000100000001:
                uflag = 256;
                break;
            case 0x0000000100000000:
                uflag = 257;
                break;
            default:
            case 0:
                uflag = 1 ;
        }
        std::copy(reinterpret_cast<const std::uint8_t*>(&uflag),reinterpret_cast<const std::uint8_t*>(&uflag)+2,rvalue.begin()+8) ;
        
        auto count = std::uint32_t(cliloc.size()) ;
        auto offset =  10;
        std::copy(reinterpret_cast<const std::uint8_t*>(&count),reinterpret_cast<const std::uint8_t*>(&count)+4,rvalue.begin()+offset) ;
        offset += 4;
        for (const auto &value:cliloc){
            std::copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+4,rvalue.begin()+offset) ;
            offset+=4 ;
        }
    }
    return rvalue ;
}

//===========================================================================
// multi_t
//===========================================================================
//===========================================================================
multi_t::multi_t(const std::vector<std::uint8_t> &bytes, bool isuop) :multi_t() {
    auto numentries = bytes.size() / multi_component_t::mul_record_size ; // Lets just assume mul for a minute
    auto dataoffset = 0 ;
    if (isuop){
        dataoffset = 4 ;
        std::copy(bytes.data()+dataoffset,bytes.data()+dataoffset+4,reinterpret_cast<std::uint8_t*>(&numentries));
        dataoffset += 4 ;
    }
    for (auto j=0 ; j < numentries;j++){
        
        auto component = multi_component_t() ;
        if(isuop){
            dataoffset += component.loaduop(bytes.data()+dataoffset);
        }
        else {
            dataoffset += component.loadmul(bytes.data()+dataoffset);
        }
        data.push_back(component);
    }
}
//===========================================================================
multi_t::multi_t(std::vector<std::string> text) :multi_t() {
    for (auto const &line : text){
        if (!line.empty()){
            auto [first,rest] = strutil::split(line,",");
            
            if ((strutil::lower(first) != "tileid") && (!rest.empty())) {
                // We are going to assume this is  a valid entry
                data.push_back(multi_component_t(line));
            }
        }
    }
}
//===========================================================================
multi_t::multi_t(const std::filesystem::path &csvfile):multi_t(){
    auto input = std::ifstream(csvfile.string());
    if (!input.is_open()){
        throw std::runtime_error("Unable to open: "s+csvfile.string());
    }
    auto data = std::vector<char>(2049,0) ;
    while(input.good() && !input.eof()){
        input.getline(data.data(), 2048) ;
        std::string text = data.data() ;
        if (!text.empty()){
            if (text[text.size()-1] == '\n') {
                text.resize(text.size()-1, 0);
            }
            if (!text.empty()){
                auto [first,rest] = strutil::split(text,",");
                
                if ((strutil::lower(first) != "tileid") && (!rest.empty())) {
                    // We are going to assume this is  a valid entry
                    this->data.push_back(multi_component_t(text));
                }

            }
        }
    }
}

//===========================================================================
auto multi_t::size() const ->std::int32_t {
    return static_cast<std::int32_t>(data.size());
}
//===========================================================================
auto multi_t::operator()(std::int32_t index) const -> const multi_component_t& {
    return data.at(index) ;
}
//===========================================================================
auto multi_t::operator()(std::int32_t index) ->multi_component_t& {
    return data.at(index) ;

}
//===========================================================================
auto multi_t::description(std::ostream &output) const ->void {
    output <<"TileID,OffsetX,OffsetY,OffsetZ,Flag,Cliloc\n";
    for (const auto &rec:data){
        output << rec.description()<<"\n";
    }
}
//===========================================================================
auto multi_t::record(bool isuop) const ->std::vector<std::uint8_t> {
    auto rvalue = std::vector<std::uint8_t>() ;
    if (isuop) {
        // we need a header, and number of entries
        auto header = std::uint32_t(0) ;
        rvalue.resize(8,0) ;
        std::copy(reinterpret_cast<std::uint8_t*>(&header),reinterpret_cast<std::uint8_t*>(&header)+4,rvalue.begin());
        header = static_cast<std::uint32_t>(data.size()) ;
        std::copy(reinterpret_cast<std::uint8_t*>(&header),reinterpret_cast<std::uint8_t*>(&header)+4,rvalue.begin()+4);
    }
    for (const auto &entry:data){
        auto temp = entry.data(isuop);
        rvalue.insert(rvalue.end(),temp.begin(),temp.end()) ;
    }
    return rvalue ;
}


//===========================================================================
// multistorage_t
//===========================================================================

//===========================================================================
auto multistorage_t::retrieve_idxaccess(std::ifstream &idxfile) ->void {
    entry_location.clear() ;
    housing_location = table_entry() ;
    auto id = 0 ;
    while (idxfile.good() && !idxfile.eof()){
        auto entry = table_entry() ;
        idxfile.read(reinterpret_cast<char*>(&entry.offset),4);
        idxfile.read(reinterpret_cast<char*>(&entry.compressed_length),4);
        idxfile.read(reinterpret_cast<char*>(&entry.decompressed_length),4);
        entry.decompressed_length = entry.compressed_length ;
        if (idxfile.gcount() ==4){
            if ((entry.offset < 0xFFFFFFFE)  && (entry.compressed_length>0) ){
                // This is a valid entry ;
                entry_location.insert_or_assign(id,entry) ;
            }
        }
        id++ ;
    }
}
//===========================================================================
auto multistorage_t::retrieve_uopaccess(std::ifstream &uopfile) ->void {
    entry_location.clear() ;
    housing_location = table_entry() ;
    // Gather all the hashes
    auto offsets = gatherEntryOffsets(uopfile) ;
    // Create our hashset
    auto hashstring = "build/multicollection/%.6u.bin"s ;
    auto hashes = hashset_t(hashstring,0,0x10000) ; ;
    // Now add the hashstring for housing.bin
    hashes.insert(housinghash,0xFFFFFFFE) ;
    entry_location = createIDTableMapping(uopfile,hashes,offsets) ;
    // Now, the only issue, if this "should" enclude the housing.bin, so lets get that
    auto iter = entry_location.find(0xFFFFFFFE) ;
    if (iter == entry_location.end()){
        // No housing bin located
        throw std::runtime_error("housing.bin hash not found");
    }
    housing_location = iter->second ;
    entry_location.erase(iter) ;
}

//==========================================================================
auto multistorage_t::gatherTextMulti(const std::filesystem::path &path)  -> std::map<std::uint32_t,std::filesystem::path> {
    auto rvalue = std::map<std::uint32_t,std::filesystem::path>() ;
    for (auto const &dir_entry : std::filesystem::recursive_directory_iterator(path)){
        if (strutil::lower(dir_entry.path().extension().string())==".csv"){
            // This could be one
            auto name =dir_entry.path().stem().string() ;
            auto loc = name.find_first_not_of("0") ;
            if (loc != std::string::npos){
                name = name.substr(loc) ;
            }
            else {
                name = "0";
            }
            try {
                auto id = static_cast<std::uint32_t>(std::stoul(name,nullptr,0)) ;
                rvalue.insert_or_assign(id, dir_entry.path()) ;
            }
            catch(...) {
                std::cerr <<"Skipping non-id csv file: "<<dir_entry.path().string()<<std::endl;
            }
         }
    }
    return rvalue ;
}


//====================================================================================
multistorage_t::multistorage_t(const std::filesystem::path &datafile, const std::filesystem::path &indexfile){
    this->datafile = std::ifstream(datafile.string(),std::ios::binary) ;
    if (!this->datafile.is_open()){
        throw std::runtime_error("Failed to open: "s + datafile.string());
    }
    
    if (indexfile.empty()){
        // we think this is a uop, lets check
        if (validUOP(this->datafile)) {
            isuop = true ;
            retrieve_uopaccess(this->datafile);
        }
        else {
            throw std::runtime_error("Invalid uop: "s + datafile.string());
        }
    }
    else {
        // Ok, so we are thinking idx/mul
        this->indexfile = indexfile ;
        auto input = std::ifstream(indexfile.string(),std::ios::binary) ;
        if (!input.is_open()){
            throw std::runtime_error("Failed to open: "s +  indexfile.string());
        }
        isuop = false ;
        retrieve_idxaccess(input);
    }
}

//====================================================================================
auto multistorage_t::maxid() const ->std::uint32_t {
    auto iter = entry_location.rbegin() ;
    return iter->first ;

}
//====================================================================================
auto multistorage_t::saveHousing(const std::filesystem::path &filepath) const ->void {
    if (!isuop){
        throw std::runtime_error("Error, housing requested from non-uop data");
    }
    auto output = std::ofstream(filepath.string(),std::ios::binary);
    if (!output.is_open()){
        throw std::runtime_error("Unable to create: "s + filepath.string());
    }
    auto offset = housing_location.offset + housing_location.header_length ;
    auto data = std::vector<std::uint8_t>(housing_location.compressed_length,0) ;
    datafile.seekg(offset,std::ios::beg);
    datafile.read(reinterpret_cast<char*>(data.data()),data.size()) ;
    if (housing_location.compression){
        // uncompress the data!
        auto temp = std::vector<std::uint8_t>(housing_location.decompressed_length,0) ;
        
        auto destsize = static_cast<uLong>(temp.size())  ;
        auto srcsize = static_cast<uLong>(data.size()) ;
        auto status = uncompress2(temp.data(), &destsize, data.data(), &srcsize);
        if (status != Z_OK){
            throw std::runtime_error("Decompression error");
        }
        std::swap(data,temp) ;
    }
    output.write(reinterpret_cast<char*>(data.data()), data.size());
}

//====================================================================================
auto multistorage_t::operator[](std::uint32_t index) const -> multi_t {
    auto rvalue = multi_t() ;
    auto iter = entry_location.find(index) ;
    if (iter != entry_location.end()){
        
        constexpr auto componentmulsize = 16 ;
        if (iter->second.decompressed_length >= componentmulsize) {
            auto offset = iter->second.offset + iter->second.header_length ;
            auto data = std::vector<std::uint8_t>(iter->second.compressed_length,0) ;
            datafile.seekg(offset,std::ios::beg);
            
            datafile.read(reinterpret_cast<char*>(data.data()),data.size()) ;
            if (iter->second.compression){
                // uncompress the data!
                auto temp = std::vector<std::uint8_t>(iter->second.decompressed_length,0) ;
                
                auto destsize = static_cast<uLong>(temp.size())  ;
                auto srcsize = static_cast<uLong>(data.size()) ;
                auto status = uncompress2(temp.data(), &destsize, data.data(), &srcsize);
                if (status != Z_OK){
                    throw std::runtime_error("Decompression error");
                }
                std::swap(data,temp) ;
            }
            rvalue = multi_t(data,isuop) ;
        }
    }
    return rvalue;
}

//====================================================================================
auto multistorage_t::saveUOP(const std::filesystem::path &csvdirectory ,const std::filesystem::path &uopfile, const std::filesystem::path &housingpath)->void {
    auto entries = gatherTextMulti(csvdirectory) ;
    
    if (entries.empty()){
        throw std::runtime_error("No valid csv entries found at: "s + csvdirectory.string());
    }
    auto housing = std::ifstream((csvdirectory / housingpath).string(),std::ios::binary) ;
    if (!housing.is_open()){
        throw std::runtime_error("Unable to open: "s + (csvdirectory / housingpath).string());
    }
    auto output = std::ofstream(uopfile.string(),std::ios::binary) ;
    if (!output.is_open()){
        throw std::runtime_error("Unable to create: "s + uopfile.string()) ;
    }
    auto offsets = createUOP(output, static_cast<std::uint32_t>(entries.size())+1) ;
    auto offset = output.tellp() ;
    auto count = 0 ;
    for (auto const &[id,path]:entries){
        auto collection = multi_t(path) ;
        auto data = collection.record(true) ;
        auto table = table_entry() ;
        table.decompressed_length = static_cast<std::uint32_t>(data.size()) ;
        table.compression = 1 ;
        auto destsize = static_cast<uLong>(compressBound(static_cast<uLong>(data.size())));
        auto dest = std::vector<std::uint8_t>(destsize,0) ;
        auto srcsize = uLongf(data.size());
        
        auto status = compress(dest.data(), &destsize, data.data(), srcsize) ;
        if (status != Z_OK) {
            throw std::runtime_error("Error compressing data for entry: "s + std::to_string(id));
        }
        dest.resize(destsize);
        std::swap(dest,data) ;
        table.compressed_length = static_cast<std::uint32_t>(data.size());
        table.identifier = hashLittle2(strutil::format(hashformat,id));
        table.data_block_hash = hashAdler32(data);
        table.offset = output.tellp() ;
        output.seekp(offset,std::ios::beg);
        output.write(reinterpret_cast<char*>(data.data()),data.size());
        offset = output.tellp();
        output.seekp(offsets[count],std::ios::beg);
        table.save(output);
        output.seekp(offset,std::ios::beg) ;
        count++ ;
    }
    // Now we need to housing.bin
    housing.seekg(0,std::ios::end) ;
    auto size = housing.tellg() ;
    housing.seekg(0,std::ios::beg) ;
    auto house = std::vector<std::uint8_t>(size,0) ;
    housing.read(reinterpret_cast<char*>(house.data()),house.size());
    auto table = table_entry() ;
    auto destsize = static_cast<uLong>(compressBound(static_cast<uLong>(house.size())));
    auto dest = std::vector<std::uint8_t>(destsize,0) ;
    auto srcsize = uLongf(house.size());
    
    auto status = compress(dest.data(), &destsize, house.data(), srcsize) ;
    if (status != Z_OK) {
        throw std::runtime_error("Error compressing data for housing entry"s );
    }
    dest.resize(destsize);
    std::swap(dest,house) ;
    table.decompressed_length = static_cast<std::uint32_t>(dest.size());
    table.compressed_length = static_cast<std::uint32_t>(house.size());
    table.identifier = housinghash ;
    table.compression =1 ;
    table.data_block_hash = hashAdler32(house);
    table.offset = output.tellp() ;
   
    output.seekp(offset,std::ios::beg);
    output.write(reinterpret_cast<char*>(house.data()),house.size());
    offset = output.tellp();
    output.seekp(offsets[count],std::ios::beg);
    table.save(output);
    output.seekp(offset,std::ios::beg) ;

}
//====================================================================================
auto multistorage_t::saveMUL(const std::filesystem::path &csvdirectory, const std::filesystem::path &mulfile, const std::filesystem::path &indexfile)->void {
    auto entries = gatherTextMulti(csvdirectory) ;
    if (entries.empty()){
        throw std::runtime_error("No valid csv entries found at: "s + csvdirectory.string());
    }
    auto maxid = (entries.rbegin()->first) + 1 ;
    if (maxid < idxmax) {
        maxid = idxmax ;
    }
    auto idx = std::ofstream(indexfile.string(),std::ios::binary);
    if(!idx.is_open()){
        throw std::runtime_error("Unable to create: "s+indexfile.string());
    }
    auto mul = std::ofstream(mulfile.string(),std::ios::binary);
    if(!mul.is_open()){
        throw std::runtime_error("Unable to create: "s+mulfile.string());
    }
    auto length = std::uint32_t(0) ;
    auto extra = std::uint32_t(0) ;
    auto offset = std::uint32_t(0) ;
    for (std::uint32_t id = 0 ; id < maxid;id++){
        auto iter = entries.find(id) ;
        if (iter != entries.end()){
            auto multi = multi_t(iter->second) ;
            auto muldata = multi.record(false) ;
            offset = static_cast<std::uint32_t>(mul.tellp()) ;
            mul.write(reinterpret_cast<char*>(muldata.data()),muldata.size());
            auto length = static_cast<std::uint32_t>(muldata.size());
            idx.write(reinterpret_cast<char*>(&offset),4);
            idx.write(reinterpret_cast<char*>(&length), 4);
            idx.write(reinterpret_cast<char*>(&extra),4);
        }
        else {
            length = 0 ;
            offset = 0xFFFFFFFE;
            idx.write(reinterpret_cast<char*>(&offset),4);
            idx.write(reinterpret_cast<char*>(&length), 4);
            idx.write(reinterpret_cast<char*>(&extra),4);
        }
    }
}
//====================================================================================
auto multistorage_t::housing() const ->std::vector<std::uint8_t> {
    auto offset = housing_location.offset + housing_location.header_length ;
    auto data = std::vector<std::uint8_t>(housing_location.compressed_length,0) ;
    datafile.seekg(offset,std::ios::beg);
    datafile.read(reinterpret_cast<char*>(data.data()),data.size()) ;
    if (housing_location.compression){
        // uncompress the data!
        auto temp = std::vector<std::uint8_t>(housing_location.decompressed_length,0) ;
        
        auto destsize = static_cast<uLong>(temp.size())  ;
        auto srcsize = static_cast<uLong>(data.size()) ;
        auto status = uncompress2(temp.data(), &destsize, data.data(), &srcsize);
        if (status != Z_OK){
            throw std::runtime_error("Decompression error");
        }
        std::swap(data,temp) ;
    }
    return data ;
}
//====================================================================================
auto multistorage_t::save(const std::filesystem::path &datapath,const std::filesystem::path &idxpath,const std::vector<std::uint8_t> &housingdata ) ->void {
    if (!idxpath.empty()){
        // We are saving to a mul/idx
        auto idx = std::ofstream(idxpath.string(),std::ios::binary) ;
        if (!idx.is_open()){
            throw std::runtime_error(strutil::format("Unable to create: %s",idxpath.string().c_str()));
        }
        auto mul = std::ofstream(datapath.string(),std::ios::binary);
        if (!mul.is_open()){
            throw std::runtime_error(strutil::format("Unable to create: %s",datapath.string().c_str()));
        }
        if (entry_location.empty()){
            throw std::runtime_error("There are no multi entries to save"s);
        }
        auto maxid = std::max(idxmax,static_cast<int>(entry_location.rbegin()->first)) ;
        
        auto length = std::uint32_t(0) ;
        auto extra = std::uint32_t(0) ;
        auto offset = std::uint32_t(0) ;
        for (std::uint32_t id = 0 ; id < static_cast<std::uint32_t>(maxid);id++){
            auto iter = entry_location.find(id) ;
            if (iter != entry_location.end()){
                auto multi = (*this)[id] ;
                auto muldata = multi.record(false) ;
                offset = static_cast<std::uint32_t>(mul.tellp()) ;
                mul.write(reinterpret_cast<char*>(muldata.data()),muldata.size());
                auto length = static_cast<std::uint32_t>(muldata.size());
                idx.write(reinterpret_cast<char*>(&offset),4);
                idx.write(reinterpret_cast<char*>(&length), 4);
                idx.write(reinterpret_cast<char*>(&extra),4);
            }
            else {
                length = 0 ;
                offset = 0xFFFFFFFE;
                idx.write(reinterpret_cast<char*>(&offset),4);
                idx.write(reinterpret_cast<char*>(&length), 4);
                idx.write(reinterpret_cast<char*>(&extra),4);
            }
        }
     }
    else {
        // We are saving to a uop!
        if (housingdata.empty()){
            throw std::runtime_error(strutil::format("No housing.bin data provided, can not create: %s",datapath.string().c_str()));
        }
        auto uop = std::ofstream(datapath.string(),std::ios::binary);
        if (!uop.is_open()){
            throw std::runtime_error(strutil::format("Unable to create: %s",datapath.string().c_str()));
        }
        auto offsets = createUOP(uop, static_cast<std::uint32_t>(entry_location.size()) + 1) ;

        auto offset = uop.tellp() ;
        auto count = 0 ;
        for (auto const &[id,entry_offset]:entry_location){
            auto collection = (*this)[id] ;
            auto data = collection.record(true) ;
            auto table = table_entry() ;
            table.decompressed_length = static_cast<std::uint32_t>(data.size()) ;
            table.compression = 1 ;
            auto destsize = static_cast<uLong>(compressBound(static_cast<uLong>(data.size())));
            auto dest = std::vector<std::uint8_t>(destsize,0) ;
            auto srcsize = uLongf(data.size());
            
            auto status = compress(dest.data(), &destsize, data.data(), srcsize) ;
            if (status != Z_OK) {
                throw std::runtime_error("Error compressing data for entry: "s + std::to_string(id));
            }
            dest.resize(destsize);
            std::swap(dest,data) ;
            table.compressed_length = static_cast<std::uint32_t>(data.size());
            table.identifier = hashLittle2(strutil::format(hashformat,id));
            table.data_block_hash = hashAdler32(data);
            table.offset = uop.tellp() ;
            uop.seekp(offset,std::ios::beg);
            uop.write(reinterpret_cast<char*>(data.data()),data.size());
            offset = uop.tellp();
            uop.seekp(offsets[count],std::ios::beg);
            table.save(uop);
            uop.seekp(offset,std::ios::beg);
            count++ ;
        }
        auto housedata = housingdata;
        // Now we need to housing.bin
        auto table = table_entry() ;
        auto destsize = static_cast<uLong>(compressBound(static_cast<uLong>(housedata.size())));
        auto dest = std::vector<std::uint8_t>(destsize,0) ;
        auto srcsize = uLongf(housedata.size());
        
        auto status = compress(dest.data(), &destsize, housedata.data(), srcsize) ;
        if (status != Z_OK) {
            throw std::runtime_error("Error compressing data for housing entry"s );
        }
        dest.resize(destsize);
        std::swap(dest,housedata) ;
        table.decompressed_length = static_cast<std::uint32_t>(dest.size());
        table.compressed_length = static_cast<std::uint32_t>(housedata.size());
        table.identifier = housinghash ;
        table.compression =1 ;
        table.data_block_hash = hashAdler32(housedata);
        table.offset = uop.tellp() ;
        
        uop.seekp(offset,std::ios::beg);
        uop.write(reinterpret_cast<char*>(housedata.data()),housedata.size());
        offset = uop.tellp();
        uop.seekp(offsets[count],std::ios::beg);
        table.save(uop);
        uop.seekp(offset,std::ios::beg) ;
   }
}
