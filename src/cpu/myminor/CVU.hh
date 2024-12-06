#ifndef __CPU_MYMINOR_CVU_HH__
#define __CPU_MYMINOR_CVU_HH__

#include <vector>
#include <list> 

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

class CVU
{
public:
    unsigned int tableSize;
    unsigned int threshold;
    unsigned int maxValue;
    std::list<unsigned int> InvalidEntries;


    struct tableEntry
    {
        bool         valid;
        unsigned int addr;
        unsigned int index;
        unsigned int data;
    };

    // std::vector<tableEntry> cvuTable (tableSize, 0);
    std::vector<tableEntry> cvuTable;

    CVU(unsigned int cvu_table_size,
        unsigned int lct_threshold,
        unsigned int lct_maxvalue);

    // virtual ~CVU();

    void storeInvalidate(unsigned int address);
    // Invalidate all matching addresses on a store

    /** Table access here for given PC */
    bool verifyEntryInCVU(unsigned int address, unsigned int index, bool constant);

    void AddToInvalidateList(unsigned int TableIndex);
    
    void AddEntryToCVU(unsigned int data, unsigned int LVPT_Index, unsigned int Translated_Data_Address);
};
}
}

#endif /* __CPU_MYMINOR_CVU_HH__ */