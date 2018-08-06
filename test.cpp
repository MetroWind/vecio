#include <iostream>
#include <numeric>

#include "logging.hpp"
#include "vec-io.hpp"

int main()
{
    double Data[10] = {1, 2,3,4,5,6,7,8,9,10};
    size_t Indices[10];
    std::iota(std::begin(Indices), std::end(Indices), 8);
    
    VecIODouble TestIO;
    TestIO.Data = Data;
    TestIO.setMeta("Name", "test");
    TestIO.addDim("testdim", std::begin(Indices), std::end(Indices));

    TestIO.write(std::cout);
    return 0;
}
