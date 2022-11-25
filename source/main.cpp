//Copyright © 2022 Charles Kerr. All rights reserved.

#include <iostream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstdlib>

#include "argument.hpp"
#include "multi.hpp"
#include "strutil.hpp"
#include "uop.hpp"

using namespace std::string_literals;

int main(int argc, const char * argv[]) {
    
    auto housepath = std::filesystem::path() ;
    auto uophouse = std::filesystem::path() ;
    auto replace = false ;
    
    auto return_code = EXIT_SUCCESS ;
    try{
        auto arg = argument_t(argc, argv);
        for (const auto &[key,value]:arg.flags){
            if (key=="override"){
                replace = true ;
            }
            else if (key == "no-override"){
                replace = false;
            }
            else if (key=="housing"){
                housepath = std::filesystem::path(value);
            }
            else if (key=="uophouse"){
                uophouse = std::filesystem::path(value);
            }
        }
        if (arg.paths.size() ==3){
            // Lets first determine if the first file exists, and is a uop
            auto firstinput = std::ifstream(arg.paths[0].string(),std::ios::binary);
            if (!firstinput.is_open()){
                throw std::runtime_error(strutil::format("Unable to open input file: %s",arg.paths[0].string().c_str()));
            }
            auto isuop = validUOP(firstinput);
            firstinput.close();
            auto srcmulti= multistorage_t() ;
            if (isuop){
                srcmulti = multistorage_t(arg.paths[0]);
            }
            else {
                srcmulti = multistorage_t(arg.paths[1],arg.paths[0]);
            }
            // Ok, we have the data!
            // Check if we shouldn't override existing files
            if (!replace){
                if (std::filesystem::exists(arg.paths[2])){
                    throw std::runtime_error(strutil::format("Files exists, with no --override present: %s",arg.paths[2].string().c_str()));
                }
                if (isuop){
                    if (std::filesystem::exists(arg.paths[1])){
                        throw std::runtime_error(strutil::format("Files exists, with no --override present: %s",arg.paths[1].string().c_str()));
                    }
                    if (!housepath.empty()){
                        if (std::filesystem::exists(housepath)){
                             throw std::runtime_error(strutil::format("Files exists, with no --override present: %s",housepath.string().c_str()));
                        }
                    }
                }
            }
            // Now, save the data!
            if (isuop) {
                // Should we save the housing data?
                if (!housepath.empty()){
                    auto output = std::ofstream(housepath.string(),std::ios::binary) ;
                    if (!output.is_open()){
                        throw std::runtime_error(strutil::format("Unable to create: %s",housepath.string().c_str()));
                    }
                    auto data = srcmulti.housing() ;
                    output.write(reinterpret_cast<char*>(data.data()),data.size());
                    output.close();
                }
                std::cout <<"Generating: "<<arg.paths[1].string()<<" , "<<arg.paths[2].string()<<std::endl;
                srcmulti.save(arg.paths[2],arg.paths[1]);
            }
            else {
                // We need to get the housing.bin data!
                auto housedata = std::vector<std::uint8_t>() ;
                if (!housepath.empty()){
                    auto input = std::ifstream(housepath.string(),std::ios::binary);
                    if (!input.is_open()){
                        throw std::runtime_error(strutil::format("Unable to open: ", housepath.string().c_str()));
                    }
                    input.seekg(0,std::ios::end) ;
                    auto size = input.tellg();
                    input.seekg(0,std::ios::beg);
                    housedata =std::vector<std::uint8_t>(size,0);
                    input.read(reinterpret_cast<char*>(housedata.data()),housedata.size());
                }
                else if (!uophouse.empty()){
                    auto temp = multistorage_t(uophouse) ;
                    housedata = temp.housing();
                }
                if (housedata.empty()){
                    throw std::runtime_error("Housing.bin data is empty. Did you include --housing or --uophouse flags?");
                }
                std::cout <<"Generating: "<<arg.paths[2].string()<<std::endl;
                srcmulti.save(arg.paths[2],std::filesystem::path(),housedata);
            }
        }
        else {
            std::cout <<"Usage: (Note, filepaths are just examples)" <<std::endl;
            std::cout <<"\tGeneral format is: convertmulti filepath filepath filepath\n\n";
            std::cout <<"Where:\n";
            
            std::cout <<"\tTo convert from MultiCollection.uop to multi.idx/mul:\n";
            std::cout <<"\t\tconvertmulti MultiCollection.uop multi.idx multi.mul\n\n";
            
            
            std::cout <<"\tTo convert from multi.idx/mul to MultiCollection.uop\n";
            std::cout <<"\t\tconvertmulti multi.idx multi.mul MultiCollection.uop\n\n";
            
            std::cout <<"Flags:\n\n";
            
            std::cout <<"\t--housing=filepath\n";
            std::cout<<"\t\tSpecifies the housing.bin file path.  If converting from idx/mul to uop\n";
            std::cout <<"\t\ta housing.bin file must be specified (or use the --uophouse flag)\n";
            std::cout <<"\t\tIf converting from uop to idx/mul, this flag specifes the filepath to store\n";
            std::cout <<"\t\tthe housing.bin file to, if present.\n\n";
            
            std::cout <<"\t--uophouse=existinguop_path\n";
            std::cout <<"\t\tOnly used on converting from idx/mul to multicollection.uop. If one doesn't\n";
            std::cout <<"\t\thave an existing houseing.bin, this allows one to specify an existing MultiCollection.uop\n";
            std::cout <<"\t\tto retrieve one from.\n\n";
            
            std::cout <<"\t--override\n";
            std::cout <<"\t\tIf present, this will allow one to overwrite the specified destination files if they exist\n";
            std::cout <<"\t\tIt is a saftey precaution to prevent accidental overwrites\n\n";
            
            std::cout <<"\tThe flags can appear anywhere on the argument line\n";
            std::cout <<std::endl;
            return_code = EXIT_FAILURE;
        }
    }
    catch (const std::exception &e){
        std::cerr <<e.what()<<std::endl;
        return_code = EXIT_FAILURE;
    }
    return return_code;
}
