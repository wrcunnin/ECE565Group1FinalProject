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
    unsigned int inPC;
    unsigned int outVal;
    bool valuePredict;

    unsigned int mispredValue;
    unsigned int mispredPC;

    struct tableEntry
    {
        bool         valid;
        unsigned int tag;
        unsigned int value;
        unsigned int addr;
    }

    std::vector<tableEntry> valueTable (tableSize, 0);

    std::vector<unsigned int> predictTable (tableSize / 4, 0);

    void updateTable()

    /** Table access here for given PC */
    void evaluate();

}

}
}