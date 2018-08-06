#include <cstring>

#include "logging.hpp"
#include "vec-io.hpp"

static const char MagicActual[] = {'V', 'E', 'C', 'I', 'O', '\0', '\0', '\0'};
const char* VecIOBin::Magic = MagicActual;

void DimSpecBin :: write(std::ostream& file) const
{
    log("Entered DimSpecBin::write().");
    log("Writing dim name: ", Name, "...");
    file.write(Name, DIM_NAME_MAX_LENGTH);
    log("Writing index count...");
    logIoVar("Index count", IdxCount);

    Endian.writeLittle(IdxCount, file);

    if(Idx == nullptr)
    {
        throw std::logic_error("Undefined indices");
        return;
    }

    for(uint64_t i = 0; i < IdxCount; ++i)
    {
        Endian.writeLittle(Idx[i], file);
    }
    log("Returning from DimSpecBin::write()..");
}

void VecIOBin :: write(std::ostream& file) const
{
    log("Entered VecIOBin::write().");
    file.write(reinterpret_cast<const char*>(Magic), sizeof(Magic));

    log("Writing sizes...");
    // Write sizes
    logIoVar("Header size", HeaderSize);
    Endian.writeLittle(HeaderSize, file);
    logIoVar("Meta size", MetaSize);
    Endian.writeLittle(MetaSize, file);
    logIoVar("Spec size", SpecSize);
    Endian.writeLittle(SpecSize, file);
    logIoVar("Data size", DataSize);
    Endian.writeLittle(DataSize, file);
    logIoVar("Number size", NumberSize);
    Endian.writeLittle(NumberSize, file);

    log("Writing checksums...");
    // Write checksums
    Endian.writeLittle(CheckSumMeta, file);
    Endian.writeLittle(CheckSumData, file);
    Endian.writeLittle(CheckSum, file);

    log("Writing metadata...");
    // Write metadata
    file.write(reinterpret_cast<const char*>(&IsMetaKeyValue), sizeof(uint8_t));
    file.write(reinterpret_cast<char*>(Metadata), MetaSize - sizeof(uint8_t));

    log("Writing dimensions...");
    // Write dimension specs
    for(uint64_t i = 0; i < DimCount; ++i)
    {
        Dims[i].write(file);
    }

    log("Returning from VecIOBin::write()..");
}
