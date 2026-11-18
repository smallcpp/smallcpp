#ifndef SMALLCPP_SCPP_STACKMAP_HPP
#define SMALLCPP_SCPP_STACKMAP_HPP

#include <string>

/* Join .llvm_stackmaps in a linked executable. Returns 0 when the
 * file is unchanged or the headers were joined. */
int merge_stackmaps(const std::string &path);

#endif
