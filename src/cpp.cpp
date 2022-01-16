/* C wrapper for certain nice C++ std functions
 * By: John Jekel
*/

/* Includes */

#include "cpp.h"

#include <cstdint>
#include <thread>

/* Function Implementations */

extern "C" uint16_t cpp_hw_concurrency(void)
{
    return std::thread::hardware_concurrency();
}
