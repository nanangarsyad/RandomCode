#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <random>

// run time parameters
#define GENERATE_CODE()   0
#define DO_TEST()         1
bool   c_verboseSamples = false;  // Turn on to show info for each sample
size_t c_testRepeatCount = 10000; // how many times a test is done for each timing sample
size_t c_testSamples = 50;        // how many timing samples are taken

// code generation parameters
typedef uint32_t TTestType;
#define TTestTypeName "uint32_t"
#define TTestTypePrintf "%u"
static const size_t c_generateNumValues = 5000;
static const size_t c_generateSparseNumValues = 1000;
static_assert(c_generateSparseNumValues <= c_generateNumValues, "c_generateSparseNumValues must be less than or equal to c_generateNumValues.");

#if GENERATE_CODE()

inline TTestType Random()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static const TTestType min = std::numeric_limits<TTestType>::min();
    static const TTestType max = std::numeric_limits<TTestType>::max();
    static std::uniform_int_distribution<TTestType> dist(min, max);
    return dist(mt);
}

void GenerateCode ()
{
    #define WRITECODE fprintf(file,

    FILE *file = fopen("Test.cpp", "w+t");
    if (!file)
        return;

    // generate the numbers
    std::vector<TTestType> randomValues;
    randomValues.resize(c_generateNumValues);
    for (TTestType& v : randomValues)
        v = Random();

    WRITECODE "#include \"test.h\"\n\n");

    // print out how many values there are
    WRITECODE "static const size_t c_numValues = %u;\n", c_generateNumValues);
    WRITECODE "static const size_t c_sparseNumValues = %u;\n\n", c_generateSparseNumValues);

    // make a shuffle order array
    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<size_t> shuffleOrder;
    shuffleOrder.resize(c_generateNumValues);
    for (size_t i = 0; i < c_generateNumValues; ++i)
        shuffleOrder[i] = i;
    std::shuffle(shuffleOrder.begin(), shuffleOrder.end(), g);
    WRITECODE "static const size_t c_shuffleOrder[c_numValues] = {");
    WRITECODE TTestTypePrintf, shuffleOrder[0]);
    for (size_t index = 1; index < c_generateNumValues; ++index)
        WRITECODE ","TTestTypePrintf, shuffleOrder[index]);
    WRITECODE "};\n\n");

    // make a sparse shuffle order array
    WRITECODE "static const size_t c_sparseShuffleOrder[c_sparseNumValues] = {");
    WRITECODE TTestTypePrintf, shuffleOrder[0]);
    for (size_t index = 1; index < c_generateSparseNumValues; ++index)
        WRITECODE ","TTestTypePrintf, shuffleOrder[index]);
    WRITECODE "};\n\n");

    // make a c style array
    WRITECODE "static const " TTestTypeName " carray[c_numValues] = {");
    WRITECODE TTestTypePrintf, randomValues[0]);
    for (size_t index = 1; index < c_generateNumValues; ++index)
        WRITECODE ","TTestTypePrintf,randomValues[index]);
    WRITECODE "};\n\n");

    // make a std::array style array
    WRITECODE "static const std::array<" TTestTypeName ", c_numValues> stdarray = {");
    WRITECODE TTestTypePrintf, randomValues[0]);
    for (size_t index = 1; index < c_generateNumValues; ++index)
        WRITECODE ","TTestTypePrintf, randomValues[index]);
    WRITECODE "}; \n\n");

    // make a std::vector style array
    WRITECODE "static const std::vector<" TTestTypeName "> stdvector = {");
    WRITECODE TTestTypePrintf, randomValues[0]);
    for (size_t index = 1; index < c_generateNumValues; ++index)
        WRITECODE ","TTestTypePrintf, randomValues[index]);
    WRITECODE "}; \n\n");

    // make the switch statement function
    WRITECODE "inline " TTestTypeName " LookupSwitch (size_t index) {\n");
    WRITECODE "    switch(index) {\n");
    for (size_t index = 0; index < c_generateNumValues; ++index)
        WRITECODE "        case %u: return " TTestTypePrintf ";\n", index, randomValues[index]);
    WRITECODE "    }\n    Fail(); return -1;\n}\n\n");

    // make the switch statement function with assume 0
    WRITECODE "inline " TTestTypeName " LookupSwitchAssume0 (size_t index) {\n");
    WRITECODE "    switch(index) {\n");
    for (size_t index = 0; index < c_generateNumValues; ++index)
        WRITECODE "        case %u: return " TTestTypePrintf ";\n", index, randomValues[index]);
    WRITECODE "        default: __assume(0);\n");
    WRITECODE "    }\n}\n\n");

    // make the sparse switch statement function
    std::vector<size_t> sparseRandomValues;
    for (size_t i = 0; i < c_generateNumValues; ++i)
    {
        if (std::find(&shuffleOrder[0], &shuffleOrder[c_generateSparseNumValues], i) != &shuffleOrder[c_generateSparseNumValues])
            sparseRandomValues.push_back(i);
    }
    WRITECODE "inline " TTestTypeName " SparseLookupSwitch (size_t index) {\n");
    WRITECODE "    switch(index) {\n");
    for (size_t index = 0; index < c_generateSparseNumValues; ++index)
        WRITECODE "        case %u: return " TTestTypePrintf ";\n", sparseRandomValues[index], randomValues[sparseRandomValues[index]]);
    WRITECODE "    }\n    Fail(); return -1;\n}\n");

    // make the sparse switch statement function with assume 0
    WRITECODE "inline " TTestTypeName " SparseLookupSwitchAssume0 (size_t index) {\n");
    WRITECODE "    switch(index) {\n");
    for (size_t index = 0; index < c_generateSparseNumValues; ++index)
        WRITECODE "        case %u: return " TTestTypePrintf ";\n", sparseRandomValues[index], randomValues[sparseRandomValues[index]]);
    WRITECODE "        default: __assume(0);\n");
    WRITECODE "    }\n}\n");

    // write the DoTest function
    WRITECODE "void DoTests () {\n");
    WRITECODE "    // make our dynamic memory\n");
    WRITECODE "    " TTestTypeName "* dynamicmemory = new " TTestTypeName "[c_numValues];\n");
    WRITECODE "    memcpy(dynamicmemory, &stdvector[0], sizeof(" TTestTypeName ")*c_numValues);\n");
    WRITECODE "    // Sequential Sum Tests\n");
    WRITECODE "    " TTestTypeName " sum = 0;\n");
    WRITECODE "    {\n");
    WRITECODE "        printf(\"Sequential Sum Timings : \\n\");\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::vector   \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += stdvector[i];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::array    \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += stdarray[i];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  c array       \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += carray[i];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  dynamic memory\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += dynamicmemory[i];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch        \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += LookupSwitch(i);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch Assume0\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += LookupSwitchAssume0(i);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "    }\n");
    WRITECODE "    // shuffle sum tests\n");
    WRITECODE "    {\n");
    WRITECODE "        printf(\"\\nShuffle Sum Timings : \\n\");\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::vector   \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += stdvector[c_shuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::array    \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += stdarray[c_shuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  c array       \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += carray[c_shuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  dynamic memory\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += dynamicmemory[c_shuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch        \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += LookupSwitch(c_shuffleOrder[i]);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch Assume0\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_numValues; ++i)\n");
    WRITECODE "                        sum += LookupSwitchAssume0(c_shuffleOrder[i]);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "    }\n");
    WRITECODE "    // sparse shuffle sum tests\n");
    WRITECODE "    {\n");
    WRITECODE "        printf(\"\\nSparse Shuffle Sum Timings : \\n\");\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::vector   \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += stdvector[c_sparseShuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  std::array    \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += stdarray[c_sparseShuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  c array       \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += carray[c_sparseShuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  dynamic memory\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += dynamicmemory[c_sparseShuffleOrder[i]];\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch        \");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += SparseLookupSwitch(c_sparseShuffleOrder[i]);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "        {\n");
    WRITECODE "            SBlockTimerAggregator timerAgg(\"  Switch Assume0\");\n");
    WRITECODE "            for (size_t sample = 0; sample < c_testSamples; ++sample)\n");
    WRITECODE "            {\n");
    WRITECODE "                SBlockTimer timer(timerAgg);\n");
    WRITECODE "                for (size_t times = 0; times < c_testRepeatCount; ++times)\n");
    WRITECODE "                    for (size_t i = 0; i < c_sparseNumValues; ++i)\n");
    WRITECODE "                        sum += SparseLookupSwitchAssume0(c_sparseShuffleOrder[i]);\n");
    WRITECODE "            }\n");
    WRITECODE "        }\n");
    WRITECODE "    }\n");
    WRITECODE "    printf(\"\\n\");\n");
    WRITECODE "    system(\"pause\");\n");
    WRITECODE "    printf(\"Sum = %%i\\n\", sum); // to keep sum, and all the things that calculate it from getting optimized away!\n");
    WRITECODE "}\n\n");

    // close the file
    fclose(file);

    #undef WRITECODE
}

#endif // GENERATE_CODE()

#if DO_TEST()
    extern void DoTests(); 
#endif

int main(int argc, char **argv)
{
    #if GENERATE_CODE()
        GenerateCode();
    #endif

    #if DO_TEST()
        DoTests();
    #endif

    return 0;
}

/*

TODO:
! redo timings on blog, and explanation of tests, analyzing results.
* NOTE:
 * added variance reporting
 * sparse switch statement wasn't being called
 * added __assume(0) versions of the switch statements.

BLOG: Is Code Faster than Data? switch vs array
Question: how does array access speed compare to switch statement speed?
 * x86/x64
 * sequential, random, sparse

Testing Details
 * vs 2013 community edition
 * 5000 random uint32's
 * c style array
 * std::array
 * std::vector
 * dynamic allocated memory
 * Switch statement function to look up a value by index
 * Go through each array 10000 times
 * Sequential Sum:
  * add up all the values in each array for the arrays
  * call the switch function for each index and sum it up
 * Shuffle Sum:
  * Same as sequential sum, but the indices are accessed in a shuffled order
 * Sparse Sum:
  * same as Shuffle sum, but only doing first 1000 items of the shuffle
  * Uses a speciailized switch function which has only those 1000 items in it.  Simulating unused values getting stripped out.

! link to code, in github!

Results:
* switch statement is A LOT slower than memory!
 * makes sense when you look at asm.
  1) Overhead of function call
  2) we compare the switch value against max to see if it's in range. (can avoid this with __assume(0) apparently)
  3) we then calculate our jump, possibly with an extra layer of indirection.
  4) we jump to that location and return a constant.
 * VS array access
  1) calculate memory location based on index.
  2) return value at index
 What if we inline the switch statement function
 * Don't know.. it made the compiler go into "Generating Code" for a long time, never saw it finish!

Follow up questions:
 * How does switch statement speed compare to hash tables?
 * If hash tables are faster than switch statements, the "hash assisted string to enum" code I made is probably slower than a hash table
  * The compile time assurance of no hash collisions is really nice though.
  ? can we get the benefits of both?



OLD TIMINGS WITHOUT VARIANCE, AND WRONG SWITCH FUNCTION CALLED IN SPARSE CASE:

Win32 Debug
    Sequential Sum Timings:
      c array: 108.48 ms
      std::array: 1135.56 ms
      std::vector: 2160.68 ms
      dynamic memory: 109.56 ms
      Switch: 1687.78 ms

    Shuffle Sum Timings:
      c array: 109.09 ms
      std::array: 1093.76 ms
      std::vector: 2156.12 ms
      dynamic memory: 102.07 ms
      Switch: 1749.74 ms

    Sparse Shuffle Sum Timings:
      c array: 20.01 ms
      std::array: 218.15 ms
      std::vector: 433.83 ms
      dynamic memory: 21.03 ms
      Switch: 294.71 ms

Win32 Release:
    Sequential Sum Timings:
      c array: 4.49 ms
      std::array: 3.50 ms
      std::vector: 4.50 ms
      dynamic memory: 4.50 ms
      Switch: 453.49 ms

    Shuffle Sum Timings:
      c array: 21.03 ms
      std::array: 21.52 ms
      std::vector: 22.00 ms
      dynamic memory: 22.02 ms
      Switch: 487.85 ms

    Sparse Shuffle Sum Timings:
      c array: 4.00 ms
      std::array: 4.00 ms
      std::vector: 4.00 ms
      dynamic memory: 4.50 ms
      Switch: 64.05 ms


x64 Debug:
    Sequential Sum Timings:
      c array: 110.08 ms
      std::array: 501.36 ms
      std::vector: 576.91 ms
      dynamic memory: 108.09 ms
      Switch: 1186.33 ms

    Shuffle Sum Timings:
      c array: 107.08 ms
      std::array: 491.34 ms
      std::vector: 569.40 ms
      dynamic memory: 101.57 ms
      Switch: 1264.53 ms

    Sparse Shuffle Sum Timings:
      c array: 21.52 ms
      std::array: 99.07 ms
      std::vector: 116.08 ms
      dynamic memory: 20.51 ms
      Switch: 193.64 ms

x64 Release:
    Sequential Sum Timings:
      c array: 3.99 ms
      std::array: 3.51 ms
      std::vector: 4.50 ms
      dynamic memory: 3.51 ms
      Switch: 490.96 ms

    Shuffle Sum Timings:
      c array: 23.52 ms
      std::array: 18.53 ms
      std::vector: 17.51 ms
      dynamic memory: 18.02 ms
      Switch: 524.11 ms

    Sparse Shuffle Sum Timings:
      c array: 3.00 ms
      std::array: 3.02 ms
      std::vector: 3.50 ms
      dynamic memory: 3.00 ms
      Switch: 65.56 ms

*/