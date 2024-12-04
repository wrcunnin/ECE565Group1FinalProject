#ifndef __CPU_MYMINOR_LVPT_HH__
#define __CPU_MYMINOR_LVPT_HH__

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

/** Execute stage.  Everything apart from fetching and decoding instructions.
 *  The LSQ lives here too. */
class LVPT : public Named
{

protected:
    unsigned int tableSize;
    unsigned int threshold;
    unsigned int maxValue;

    struct tableEntry
    {
        bool         valid;
        unsigned int tag;
        unsigned int addr;
        unsigned int value;
    };

    struct lvptData
    {
        bool constant;
        bool predict;
        unsigned int value;
        unsigned int pc;
        unsigned int index;
    };

    std::vector<tableEntry> valueTable (tableSize, 0);

    std::vector<unsigned int> predictTable (tableSize / 4, 0);

    void updateTable(unsigned int data, unsigned int addr, unsigned int pc, bool mispredict)

    /** Table access here for given PC */
    lvptData read(unsigned int pc);

}

}
}

#endif /* __CPU_MYMINOR_LVPT_HH__ */