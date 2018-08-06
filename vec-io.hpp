#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <algorithm>

#include "logging.hpp"

struct Endianness
{
    static const size_t SWAP_BUFFER_SIZE = 1024;
    enum ValueType {BIG, LITTLE};
    ValueType Value;

    Endianness()
    {
        // The compiler may decide to calculate this at compile time,
        // and just refer to the result as a constant at run time. But
        // this may be fine.
        uint16_t num = 1;
        if(*(uint8_t *)&num == 1)
        {
            Value = LITTLE;
        }
        else
        {
            Value = BIG;
        }
    }

    ~Endianness() {}

    template<class T>
    static T swap(const T& x)
    {
        T y = x;
        uint8_t* Bytes = reinterpret_cast<uint8_t*>(&y);
        for(size_t i = 0; i < sizeof(T)/2; ++i)
        {
            std::swap(Bytes[sizeof(T) - 1 - i], Bytes[i]);
        }
        return y;
    }

    template<class T>
    T swapToLittle(const T& x) const
    {
        if(Value == LITTLE)
        {
            return x;
        }
        else
        {
            return swap(x);
        }
    }

    template<class T>
    void writeLittle(const T& x, std::ostream& file) const
    {
        T Value = swapToLittle(x);
        file.write(reinterpret_cast<char*>(&Value), sizeof(T));
    }

    void writeLittle(const char* pos, size_t length, std::ostream& file) const
    // Treat the `length`-byte data at `pos` as a whole, and write it
    // to file in little endian. `Length` cannot exceed SWAP_BUFFER_SIZE.
    {
        if(Value == LITTLE)
        {
            file.write(pos, length);
        }
        else
        {
            char buffer[SWAP_BUFFER_SIZE];
            std::reverse_copy(pos, pos + length, buffer);
            file.write(buffer, length);
        }
    }
};

const Endianness Endian;

struct DimSpecBin
{
    static const size_t DIM_NAME_MAX_LENGTH = 128;
    char Name[DIM_NAME_MAX_LENGTH];
    uint64_t IdxCount;
    uint64_t* Idx = nullptr;

    size_t sizeBinary() const
    {
        return sizeof(uint8_t) * DIM_NAME_MAX_LENGTH + sizeof(uint64_t)
            + IdxCount * sizeof(uint64_t);
    }

    void write(std::ostream& file) const;
};

struct VecIOBin
{
    static const char* Magic;
    uint64_t HeaderSize;
    uint64_t MetaSize;            // Including `IsMetaKeyValue'.
    uint64_t SpecSize;            // Including `DimCount'.
    uint64_t DataSize;
    uint64_t NumberSize;        // Size of a single number in data. Either 32 or 64.

    uint32_t CheckSumMeta = 0;
    uint32_t CheckSumData = 0;
    uint32_t CheckSum = 0;

    // Metadata
    uint8_t IsMetaKeyValue;        // 1 means key-value, 0 means not key-value.
    uint8_t* Metadata = nullptr;

    // Spec
    uint64_t DimCount;
    DimSpecBin* Dims = nullptr;

    void write(std::ostream& file) const;
};

struct DimSpec
{
    std::string Name;
    std::vector<uint64_t> Idx;

    size_t size() const
    {
        return Idx.size();
    }
};

// enum class VecNumType { FLOAT, DOUBLE };

template<class NumType>
class VecIO
{
private:
    std::map<std::string, std::string> Metadata;
    std::vector<DimSpec> Dims;
    std::map<std::string, DimSpec*> DimMap;

public:
    NumType* Data = nullptr;

    VecIO() {};
    ~VecIO() {};

    void setMeta(const std::string& key, const std::string& value);
    std::string getMeta(const std::string& key) const;

    template<class IterType> inline
    void addDim(const std::string& dim_name, IterType idx_begin, IterType idx_end);

    void write(std::ostream& output);
};

typedef VecIO<double> VecIODouble;

template<class NumType>
template<class IterType> inline
void VecIO<NumType> :: addDim(const std::string& dim_name,
                              IterType idx_begin, IterType idx_end)
{
    if(dim_name.size() >= DimSpecBin::DIM_NAME_MAX_LENGTH)
    {
        throw std::length_error(std::string("Name of dimension too long: ")
                                + dim_name);
    }

    DimSpec NewDim;
    NewDim.Name = dim_name;

    for(auto Iter = idx_begin; Iter != idx_end; ++Iter)
    {
        NewDim.Idx.push_back(*Iter);
    }

    Dims.push_back(NewDim);
    DimMap[dim_name] = &(*(std::end(Dims)));
}

template<class NumType>
void VecIO<NumType> :: setMeta(const std::string& key, const std::string& value)
{
    Metadata[key] = value;
}

template<class NumType>
std::string VecIO<NumType> :: getMeta(const std::string& key) const
{
    return Metadata[key];
}

template<class NumType>
void VecIO<NumType> :: write(std::ostream& output)
{
    VecIOBin Bin;

    // Gather metadata.
    Bin.IsMetaKeyValue = 1;
    std::vector<uint8_t> BufMeta;

    // Insert the key-value pairs into the buffer as key\0value\0...
    Bin.MetaSize = 0;
    for(const auto& pair: Metadata)
    {
        // Char is guaranteed to have size of 1. Uint8_t is guaranteed
        // to be the same size as char.
        const char* key = pair.first.c_str();
        BufMeta.insert(std::end(BufMeta), key, key + pair.first.size() + 1);
        const char* val = pair.second.c_str();
        BufMeta.insert(std::end(BufMeta), val, val + pair.second.size() + 1);

        Bin.MetaSize += (pair.first.size() + pair.second.size() + 2) * sizeof(char);
    }
    Bin.Metadata = BufMeta.data();
    Bin.MetaSize += sizeof(uint8_t); // Size of IsMetaKeyValue.

    // Gather dimension specs.
    Bin.SpecSize = 0;

    std::vector<DimSpecBin> DimsBin(Dims.size());
    uint64_t DataSize = 1;      // Number of numbers in data.
    for(size_t i = 0; i < Dims.size(); ++i)
    {
        DimSpecBin& DimBin = DimsBin[i];
        std::strcpy(DimBin.Name, Dims[i].Name.c_str());
        DimBin.IdxCount = Dims[i].size();
        DimBin.Idx = Dims[i].Idx.data();

        Bin.SpecSize += DimBin.sizeBinary();
        DataSize *= DimBin.IdxCount;
    }
    Bin.SpecSize += sizeof(uint64_t); // SpecSize
    Bin.DimCount = Dims.size();
    Bin.Dims = DimsBin.data();
    Bin.NumberSize = sizeof(NumType);

    Bin.DataSize = DataSize * Bin.NumberSize;

    Bin.HeaderSize = sizeof(Bin.Magic)
        + 5 * sizeof(uint64_t)  // The sizes
        + 3 * sizeof(uint32_t)  // Checksums
        + Bin.MetaSize + Bin.SpecSize;

    // Start writing
    Bin.write(output);

    // Write data
    log("Writing data...");
    if(Endian.Value == Endianness::LITTLE)
    {
        output.write(reinterpret_cast<char*>(Data), Bin.DataSize);
    }
    else
    {
        char* Pos = reinterpret_cast<char*>(Data);
        for(uint64_t i = 0; i < DataSize; ++i)
        {
            Endian.writeLittle(Pos, Bin.NumberSize, output);
            Pos += Bin.NumberSize;
        }
    }
}
