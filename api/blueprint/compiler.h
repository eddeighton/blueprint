
#ifndef COMPILER_26_NOV_2020
#define COMPILER_26_NOV_2020

#include "blueprint/basicarea.h"

#include <memory>
#include <vector>

namespace Blueprint
{
    
class Compiler
{
public:
    Compiler( const Site::PtrVector& sites );

private:
    class CompilerImpl;
    std::shared_ptr< CompilerImpl > m_pPimpl;
    
};

}

#endif //COMPILER_26_NOV_2020