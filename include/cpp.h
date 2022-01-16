/* C wrapper for certain nice C++ std functions
 * By: John Jekel
*/

#ifndef CPP_H
#define CPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */

#include <stdint.h>

/* Function/Class Declarations */

uint16_t cpp_hw_concurrency(void);

#ifdef __cplusplus
}
#endif

#endif//CPP_H
