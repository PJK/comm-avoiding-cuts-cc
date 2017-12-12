/** Galois Conflict flags -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2012, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @author Donald Nguyen <ddn@cs.utexas.edu>
 */

#ifndef GALOIS_RUNTIME_METHODFLAGS_H
#define GALOIS_RUNTIME_METHODFLAGS_H

#include "Galois/config.h"
#include "Galois/MethodFlags.h"

namespace Galois {
namespace Runtime {

void doCheckWrite();

inline bool isWriteMethod(Galois::MethodFlag m, bool write) {
  return write || (m & MethodFlag::WRITE) != Galois::MethodFlag::NONE;
}

inline void checkWrite(Galois::MethodFlag m, bool write) {
#ifndef GALOIS_USE_HTM
  if (isWriteMethod(m, write))
    doCheckWrite();
#endif
}

}
} // end namespace Galois

#endif //GALOIS_RUNTIME_METHODFLAGS_H
