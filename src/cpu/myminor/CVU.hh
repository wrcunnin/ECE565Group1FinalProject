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
    unsigned int inPC;
    unsigned int outVal;
    bool valuePredict;

    unsigned int mispredValue;
    unsigned int mispredPC;

    struct tableEntry
    {
        unsigned int value;
        unsigned int addr;
    }

    std::vector<tableEntry> valueTable (tableSize, 0);

    /** Table access here for given PC */
    void evaluate();

}
}
}