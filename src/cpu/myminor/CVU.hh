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
        unsigned long vaddr;
        unsigned long paddr;
        unsigned long index;
        unsigned long data;
    };

    // std::vector<tableEntry> cvuTable (tableSize, 0);
    std::vector<tableEntry> cvuTable;

    CVU(unsigned int cvu_table_size,
        unsigned int lct_threshold,
        unsigned int lct_maxvalue);

    // virtual ~CVU();

    void storeInvalidate(unsigned long vaddr, unsigned long paddr);
    // Invalidate all matching addresses on a store

    /** Table access here for given PC */
    bool verifyEntryInCVU(unsigned long address, unsigned long index, unsigned long data, bool constant, unsigned long &paddr);

    void AddToInvalidateList(unsigned int TableIndex);
    
    void AddEntryToCVU(unsigned long data, unsigned long LVPT_Index, unsigned long vaddr, unsigned long paddr);

    void printCVUEntries();
};
}
}

#endif /* __CPU_MYMINOR_CVU_HH__ */