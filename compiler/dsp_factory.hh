/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2016 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#ifndef __dsp_factory_base__
#define __dsp_factory_base__

#include <string>
#include <vector>
#include <ostream>
#include <string.h>
#include <assert.h>

#include "faust/gui/meta.h"

#define LVVM_BACKEND_NAME       "Faust LLVM backend"
#define COMPILATION_OPTIONS_KEY "compilation_options"
#define COMPILATION_OPTIONS     "declare compilation_options    "

#define FAUSTVERSION "2.0.a56"

class dsp_factory;
class dsp;

/*
 In order to better separate compilation and execution for dynamic backends (LLVM, interpreter, asm.js, WebAssembly).
 A dsp_factory_base* object will either be generated by the compiler from a dsp,
 or by reading an already compiled dsp (in LLVM IR or interpreter bytecode).
 */

class dsp_factory_base {
    
    public:
    
        virtual ~dsp_factory_base()
        {}
        
        virtual std::string getName() = 0;
        virtual void setName(const std::string& name) = 0;
        
        virtual std::string getSHAKey() = 0;
        virtual void setSHAKey(const std::string& sha_key) = 0;
        
        virtual std::string getDSPCode() = 0;
        virtual void setDSPCode(const std::string& code) = 0;
        
        virtual dsp* createDSPInstance(dsp_factory* factory) = 0;
        
        virtual void metadata(Meta* meta) = 0;
    
        virtual void write(std::ostream* out, bool binary = false, bool small = false) = 0;
    
        virtual void writeAux(std::ostream* out, bool binary = false, bool small = false) {}    // Helper functions
    
        virtual std::vector<std::string> getDSPFactoryLibraryList() = 0;
    
        // Sub-classes will typically implement this method to create a factory from a stream
        static dsp_factory_base* read(std::istream* in) { return nullptr; }
  
};

class dsp_factory_imp : public dsp_factory_base {
    
    protected:
    
        std::string fName;
        std::string fSHAKey;
        std::string fExpandedDSP;
        std::vector<std::string> fPathnameList;
    
    public:
    
        dsp_factory_imp(const std::string& name,
                        const std::string& sha_key,
                        const std::string& dsp,
                        const std::vector<std::string>& pathname_list)
            :fName(name), fSHAKey(sha_key), fExpandedDSP(dsp), fPathnameList(pathname_list)
        {}
    
        dsp_factory_imp(const std::string& name,
                        const std::string& sha_key,
                        const std::string& dsp)
            :fName(name), fSHAKey(sha_key), fExpandedDSP(dsp)
        {}
        
        virtual ~dsp_factory_imp()
        {}
    
        std::string getName()
        {
            struct MyMeta : public Meta
            {
                std::string name;
                virtual void declare(const char* key, const char* value)
                {
                    if (strcmp(key, "name") == 0) name = value;
                }
            };
            
            MyMeta meta_data;
            metadata(&meta_data);
            return (meta_data.name != "") ? meta_data.name : fName;
        }

        void setName(const std::string& name) { fName = name; }
        
        std::string getSHAKey() { return fSHAKey; }
        void setSHAKey(const std::string& sha_key) { fSHAKey = sha_key; }
        
        std::string getDSPCode() { return fExpandedDSP; }
        void setDSPCode(const std::string& code) { fExpandedDSP = code; }
        
        virtual dsp* createDSPInstance(dsp_factory* factory) { return nullptr; }
        
        virtual void metadata(Meta* meta) { assert(false); }
    
        virtual void write(std::ostream* out, bool binary = false, bool small = false) {}
    
        virtual std::vector<std::string> getDSPFactoryLibraryList() { return fPathnameList; }
 
};

class text_dsp_factory_aux : public dsp_factory_imp {
    
    protected:
    
        std::string fCode;
        std::string fHelper;
    
    public:
    
        text_dsp_factory_aux(const std::string& name,
                             const std::string& sha_key,
                             const std::string& dsp,
                             const std::vector<std::string>& pathname_list,
                             const std::string& code,
                             const std::string& helper)
            :dsp_factory_imp(name, sha_key, dsp, pathname_list), fCode(code), fHelper(helper)
        {}
        
        virtual void write(std::ostream* out, bool binary = false, bool small = false)
        {
            *out << fCode;
        }
    
        virtual void writeAux(std::ostream* out, bool binary = false, bool small = false)
        {
            *out << fHelper;
        }
};

dsp_factory_base* compile_faust_factory(int argc, const char* argv[], const char* name, const char* input, std::string& error_msg, bool generate);

std::string expand_dsp(int argc, const char* argv[], const char* name, const char* input, std::string& sha_key, std::string& error_msg);

#endif