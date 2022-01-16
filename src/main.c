/* main() and other glue code
 * By: John Jekel
*/

/* Includes */

#include "cmake_config.h"
#include "interactive.h"
#include "cmdline.h"
#include "cpp.h"

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

/* Function Implementations */

int main(int argc, const char* const* argv)
{
    fprintf(stderr, "mandelbrot_bmp_generator version %u.%u\n", MBBMP_VERSION_MAJOR, MBBMP_VERSION_MINOR);
    fputs("By: John Jekel (JZJ)\n", stderr);
    fprintf(stderr, "Compiled on %s at %s\n", __DATE__, __TIME__);
    fprintf(stderr, "System has %hu hardware threads avaliable\n\n", cpp_hw_concurrency());

#ifndef NDEBUG
    //fprintf(stderr, "Debug build: Git info: Commit %s, branch %s\n", MBBMP_GIT_HASH, MBBMP_GIT_BRANCH);
#endif

    if (argc < 2)//Interactive session
        return interactive();
    else
        return cmdline(argc, argv);

    return 0;
}
