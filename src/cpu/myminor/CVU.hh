#ifndef __CPU_MYMINOR_CVU_HH__
#define __CPU_MYMINOR_CVU_HH__

#include <vector>

#include "arch/generic/mmu.hh"
#include "base/named.hh"
#include "cpu/base.hh"
#include "cpu/myminor/buffers.hh"
#include "cpu/myminor/cpu.hh"
#include "cpu/myminor/pipe_data.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

class CVU : public Named
{

protected:
    unsigned int tableSize;
    unsigned int threshold;
    unsigned int maxValue;

    struct tableEntry
    {
        bool         valid;
        unsigned int addr;
        unsigned int index;
        unsigned int data;
    }

    std::vector<tableEntry> cvuTable (tableSize, 0);

    void storeInvalidate(unsigned int address);
    // Invalidate all matching addresses on a store

    /** Table access here for given PC */
    void verifyEntryInCVU(unsigned int address, unsigned int index);

}
}
}

#endif /* __CPU_MYMINOR_CVU_HH__ */