#include "marchingCubes_cpu.h"

#include "vtkDataArray.h"
#include "vtkFloatArray.h"

#include <vector>
#include <cmath>
#include <map>
#include <cstdlib>
#include <helper_math.h>
#include <iostream>

using namespace std;

static unsigned edgeTable[256] = {
  0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
  0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
  0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
  0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
  0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
  0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
  0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
  0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
  0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
  0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
  0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
  0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
  0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
  0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
  0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
  0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
  0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
  0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
  0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
  0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
  0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
  0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
  0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
  0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
  0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
  0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
  0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
  0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
  0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
  0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
  0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
  0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

#define X 255
static unsigned triTable[256][16] = {
  {X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {1, 8, 3, 9, 8, 1, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 10, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 1, 2, 10, X, X, X, X, X, X, X, X, X, X},
  {9, 2, 10, 0, 2, 9, X, X, X, X, X, X, X, X, X, X},
  {2, 8, 3, 2, 10, 8, 10, 9, 8, X, X, X, X, X, X, X},
  {3, 11, 2, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 11, 2, 8, 11, 0, X, X, X, X, X, X, X, X, X, X},
  {1, 9, 0, 2, 3, 11, X, X, X, X, X, X, X, X, X, X},
  {1, 11, 2, 1, 9, 11, 9, 8, 11, X, X, X, X, X, X, X},
  {3, 10, 1, 11, 10, 3, X, X, X, X, X, X, X, X, X, X},
  {0, 10, 1, 0, 8, 10, 8, 11, 10, X, X, X, X, X, X, X},
  {3, 9, 0, 3, 11, 9, 11, 10, 9, X, X, X, X, X, X, X},
  {9, 8, 10, 10, 8, 11, X, X, X, X, X, X, X, X, X, X},
  {4, 7, 8, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {4, 3, 0, 7, 3, 4, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, 8, 4, 7, X, X, X, X, X, X, X, X, X, X},
  {4, 1, 9, 4, 7, 1, 7, 3, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 8, 4, 7, X, X, X, X, X, X, X, X, X, X},
  {3, 4, 7, 3, 0, 4, 1, 2, 10, X, X, X, X, X, X, X},
  {9, 2, 10, 9, 0, 2, 8, 4, 7, X, X, X, X, X, X, X},
  {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, X, X, X, X},
  {8, 4, 7, 3, 11, 2, X, X, X, X, X, X, X, X, X, X},
  {11, 4, 7, 11, 2, 4, 2, 0, 4, X, X, X, X, X, X, X},
  {9, 0, 1, 8, 4, 7, 2, 3, 11, X, X, X, X, X, X, X},
  {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, X, X, X, X},
  {3, 10, 1, 3, 11, 10, 7, 8, 4, X, X, X, X, X, X, X},
  {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, X, X, X, X},
  {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, X, X, X, X},
  {4, 7, 11, 4, 11, 9, 9, 11, 10, X, X, X, X, X, X, X},
  {9, 5, 4, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {9, 5, 4, 0, 8, 3, X, X, X, X, X, X, X, X, X, X},
  {0, 5, 4, 1, 5, 0, X, X, X, X, X, X, X, X, X, X},
  {8, 5, 4, 8, 3, 5, 3, 1, 5, X, X, X, X, X, X, X},
  {1, 2, 10, 9, 5, 4, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 8, 1, 2, 10, 4, 9, 5, X, X, X, X, X, X, X},
  {5, 2, 10, 5, 4, 2, 4, 0, 2, X, X, X, X, X, X, X},
  {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, X, X, X, X},
  {9, 5, 4, 2, 3, 11, X, X, X, X, X, X, X, X, X, X},
  {0, 11, 2, 0, 8, 11, 4, 9, 5, X, X, X, X, X, X, X},
  {0, 5, 4, 0, 1, 5, 2, 3, 11, X, X, X, X, X, X, X},
  {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, X, X, X, X},
  {10, 3, 11, 10, 1, 3, 9, 5, 4, X, X, X, X, X, X, X},
  {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, X, X, X, X},
  {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, X, X, X, X},
  {5, 4, 8, 5, 8, 10, 10, 8, 11, X, X, X, X, X, X, X},
  {9, 7, 8, 5, 7, 9, X, X, X, X, X, X, X, X, X, X},
  {9, 3, 0, 9, 5, 3, 5, 7, 3, X, X, X, X, X, X, X},
  {0, 7, 8, 0, 1, 7, 1, 5, 7, X, X, X, X, X, X, X},
  {1, 5, 3, 3, 5, 7, X, X, X, X, X, X, X, X, X, X},
  {9, 7, 8, 9, 5, 7, 10, 1, 2, X, X, X, X, X, X, X},
  {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, X, X, X, X},
  {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, X, X, X, X},
  {2, 10, 5, 2, 5, 3, 3, 5, 7, X, X, X, X, X, X, X},
  {7, 9, 5, 7, 8, 9, 3, 11, 2, X, X, X, X, X, X, X},
  {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, X, X, X, X},
  {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, X, X, X, X},
  {11, 2, 1, 11, 1, 7, 7, 1, 5, X, X, X, X, X, X, X},
  {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, X, X, X, X},
  {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, X},
  {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, X},
  {11, 10, 5, 7, 11, 5, X, X, X, X, X, X, X, X, X, X},
  {10, 6, 5, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 5, 10, 6, X, X, X, X, X, X, X, X, X, X},
  {9, 0, 1, 5, 10, 6, X, X, X, X, X, X, X, X, X, X},
  {1, 8, 3, 1, 9, 8, 5, 10, 6, X, X, X, X, X, X, X},
  {1, 6, 5, 2, 6, 1, X, X, X, X, X, X, X, X, X, X},
  {1, 6, 5, 1, 2, 6, 3, 0, 8, X, X, X, X, X, X, X},
  {9, 6, 5, 9, 0, 6, 0, 2, 6, X, X, X, X, X, X, X},
  {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, X, X, X, X},
  {2, 3, 11, 10, 6, 5, X, X, X, X, X, X, X, X, X, X},
  {11, 0, 8, 11, 2, 0, 10, 6, 5, X, X, X, X, X, X, X},
  {0, 1, 9, 2, 3, 11, 5, 10, 6, X, X, X, X, X, X, X},
  {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, X, X, X, X},
  {6, 3, 11, 6, 5, 3, 5, 1, 3, X, X, X, X, X, X, X},
  {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, X, X, X, X},
  {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, X, X, X, X},
  {6, 5, 9, 6, 9, 11, 11, 9, 8, X, X, X, X, X, X, X},
  {5, 10, 6, 4, 7, 8, X, X, X, X, X, X, X, X, X, X},
  {4, 3, 0, 4, 7, 3, 6, 5, 10, X, X, X, X, X, X, X},
  {1, 9, 0, 5, 10, 6, 8, 4, 7, X, X, X, X, X, X, X},
  {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, X, X, X, X},
  {6, 1, 2, 6, 5, 1, 4, 7, 8, X, X, X, X, X, X, X},
  {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, X, X, X, X},
  {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, X, X, X, X},
  {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, X},
  {3, 11, 2, 7, 8, 4, 10, 6, 5, X, X, X, X, X, X, X},
  {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, X, X, X, X},
  {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, X, X, X, X},
  {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, X},
  {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, X, X, X, X},
  {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, X},
  {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, X},
  {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, X, X, X, X},
  {10, 4, 9, 6, 4, 10, X, X, X, X, X, X, X, X, X, X},
  {4, 10, 6, 4, 9, 10, 0, 8, 3, X, X, X, X, X, X, X},
  {10, 0, 1, 10, 6, 0, 6, 4, 0, X, X, X, X, X, X, X},
  {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, X, X, X, X},
  {1, 4, 9, 1, 2, 4, 2, 6, 4, X, X, X, X, X, X, X},
  {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, X, X, X, X},
  {0, 2, 4, 4, 2, 6, X, X, X, X, X, X, X, X, X, X},
  {8, 3, 2, 8, 2, 4, 4, 2, 6, X, X, X, X, X, X, X},
  {10, 4, 9, 10, 6, 4, 11, 2, 3, X, X, X, X, X, X, X},
  {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, X, X, X, X},
  {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, X, X, X, X},
  {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, X},
  {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, X, X, X, X},
  {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, X},
  {3, 11, 6, 3, 6, 0, 0, 6, 4, X, X, X, X, X, X, X},
  {6, 4, 8, 11, 6, 8, X, X, X, X, X, X, X, X, X, X},
  {7, 10, 6, 7, 8, 10, 8, 9, 10, X, X, X, X, X, X, X},
  {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, X, X, X, X},
  {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, X, X, X, X},
  {10, 6, 7, 10, 7, 1, 1, 7, 3, X, X, X, X, X, X, X},
  {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, X, X, X, X},
  {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, X},
  {7, 8, 0, 7, 0, 6, 6, 0, 2, X, X, X, X, X, X, X},
  {7, 3, 2, 6, 7, 2, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, X, X, X, X},
  {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, X},
  {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, X},
  {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, X, X, X, X},
  {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, X},
  {0, 9, 1, 11, 6, 7, X, X, X, X, X, X, X, X, X, X},
  {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, X, X, X, X},
  {7, 11, 6, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {7, 6, 11, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 8, 11, 7, 6, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, 11, 7, 6, X, X, X, X, X, X, X, X, X, X},
  {8, 1, 9, 8, 3, 1, 11, 7, 6, X, X, X, X, X, X, X},
  {10, 1, 2, 6, 11, 7, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 10, 3, 0, 8, 6, 11, 7, X, X, X, X, X, X, X},
  {2, 9, 0, 2, 10, 9, 6, 11, 7, X, X, X, X, X, X, X},
  {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, X, X, X, X},
  {7, 2, 3, 6, 2, 7, X, X, X, X, X, X, X, X, X, X},
  {7, 0, 8, 7, 6, 0, 6, 2, 0, X, X, X, X, X, X, X},
  {2, 7, 6, 2, 3, 7, 0, 1, 9, X, X, X, X, X, X, X},
  {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, X, X, X, X},
  {10, 7, 6, 10, 1, 7, 1, 3, 7, X, X, X, X, X, X, X},
  {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, X, X, X, X},
  {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, X, X, X, X},
  {7, 6, 10, 7, 10, 8, 8, 10, 9, X, X, X, X, X, X, X},
  {6, 8, 4, 11, 8, 6, X, X, X, X, X, X, X, X, X, X},
  {3, 6, 11, 3, 0, 6, 0, 4, 6, X, X, X, X, X, X, X},
  {8, 6, 11, 8, 4, 6, 9, 0, 1, X, X, X, X, X, X, X},
  {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, X, X, X, X},
  {6, 8, 4, 6, 11, 8, 2, 10, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, X, X, X, X},
  {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, X, X, X, X},
  {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, X},
  {8, 2, 3, 8, 4, 2, 4, 6, 2, X, X, X, X, X, X, X},
  {0, 4, 2, 4, 6, 2, X, X, X, X, X, X, X, X, X, X},
  {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, X, X, X, X},
  {1, 9, 4, 1, 4, 2, 2, 4, 6, X, X, X, X, X, X, X},
  {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, X, X, X, X},
  {10, 1, 0, 10, 0, 6, 6, 0, 4, X, X, X, X, X, X, X},
  {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, X},
  {10, 9, 4, 6, 10, 4, X, X, X, X, X, X, X, X, X, X},
  {4, 9, 5, 7, 6, 11, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 4, 9, 5, 11, 7, 6, X, X, X, X, X, X, X},
  {5, 0, 1, 5, 4, 0, 7, 6, 11, X, X, X, X, X, X, X},
  {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, X, X, X, X},
  {9, 5, 4, 10, 1, 2, 7, 6, 11, X, X, X, X, X, X, X},
  {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, X, X, X, X},
  {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, X, X, X, X},
  {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, X},
  {7, 2, 3, 7, 6, 2, 5, 4, 9, X, X, X, X, X, X, X},
  {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, X, X, X, X},
  {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, X, X, X, X},
  {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, X},
  {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, X, X, X, X},
  {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, X},
  {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, X},
  {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, X, X, X, X},
  {6, 9, 5, 6, 11, 9, 11, 8, 9, X, X, X, X, X, X, X},
  {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, X, X, X, X},
  {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, X, X, X, X},
  {6, 11, 3, 6, 3, 5, 5, 3, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, X, X, X, X},
  {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, X},
  {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, X},
  {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, X, X, X, X},
  {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, X, X, X, X},
  {9, 5, 6, 9, 6, 0, 0, 6, 2, X, X, X, X, X, X, X},
  {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, X},
  {1, 5, 6, 2, 1, 6, X, X, X, X, X, X, X, X, X, X},
  {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, X},
  {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, X, X, X, X},
  {0, 3, 8, 5, 6, 10, X, X, X, X, X, X, X, X, X, X},
  {10, 5, 6, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {11, 5, 10, 7, 5, 11, X, X, X, X, X, X, X, X, X, X},
  {11, 5, 10, 11, 7, 5, 8, 3, 0, X, X, X, X, X, X, X},
  {5, 11, 7, 5, 10, 11, 1, 9, 0, X, X, X, X, X, X, X},
  {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, X, X, X, X},
  {11, 1, 2, 11, 7, 1, 7, 5, 1, X, X, X, X, X, X, X},
  {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, X, X, X, X},
  {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, X, X, X, X},
  {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, X},
  {2, 5, 10, 2, 3, 5, 3, 7, 5, X, X, X, X, X, X, X},
  {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, X, X, X, X},
  {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, X, X, X, X},
  {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, X},
  {1, 3, 5, 3, 7, 5, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 7, 0, 7, 1, 1, 7, 5, X, X, X, X, X, X, X},
  {9, 0, 3, 9, 3, 5, 5, 3, 7, X, X, X, X, X, X, X},
  {9, 8, 7, 5, 9, 7, X, X, X, X, X, X, X, X, X, X},
  {5, 8, 4, 5, 10, 8, 10, 11, 8, X, X, X, X, X, X, X},
  {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, X, X, X, X},
  {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, X, X, X, X},
  {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, X},
  {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, X, X, X, X},
  {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, X},
  {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, X},
  {9, 4, 5, 2, 11, 3, X, X, X, X, X, X, X, X, X, X},
  {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, X, X, X, X},
  {5, 10, 2, 5, 2, 4, 4, 2, 0, X, X, X, X, X, X, X},
  {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, X},
  {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, X, X, X, X},
  {8, 4, 5, 8, 5, 3, 3, 5, 1, X, X, X, X, X, X, X},
  {0, 4, 5, 1, 0, 5, X, X, X, X, X, X, X, X, X, X},
  {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, X, X, X, X},
  {9, 4, 5, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {4, 11, 7, 4, 9, 11, 9, 10, 11, X, X, X, X, X, X, X},
  {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, X, X, X, X},
  {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, X, X, X, X},
  {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, X},
  {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, X, X, X, X},
  {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, X},
  {11, 7, 4, 11, 4, 2, 2, 4, 0, X, X, X, X, X, X, X},
  {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, X, X, X, X},
  {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, X, X, X, X},
  {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, X},
  {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, X},
  {1, 10, 2, 8, 7, 4, X, X, X, X, X, X, X, X, X, X},
  {4, 9, 1, 4, 1, 7, 7, 1, 3, X, X, X, X, X, X, X},
  {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, X, X, X, X},
  {4, 0, 3, 7, 4, 3, X, X, X, X, X, X, X, X, X, X},
  {4, 8, 7, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {9, 10, 8, 10, 11, 8, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 9, 3, 9, 11, 11, 9, 10, X, X, X, X, X, X, X},
  {0, 1, 10, 0, 10, 8, 8, 10, 11, X, X, X, X, X, X, X},
  {3, 1, 10, 11, 3, 10, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 11, 1, 11, 9, 9, 11, 8, X, X, X, X, X, X, X},
  {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, X, X, X, X},
  {0, 2, 11, 8, 0, 11, X, X, X, X, X, X, X, X, X, X},
  {3, 2, 11, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 8, 2, 8, 10, 10, 8, 9, X, X, X, X, X, X, X},
  {9, 10, 2, 0, 9, 2, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, X, X, X, X},
  {1, 10, 2, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {1, 3, 8, 9, 1, 8, X, X, X, X, X, X, X, X, X, X},
  {0, 9, 1, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 3, 8, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X}
};
#undef X

// number of vertices for each case above
static unsigned numVertsTable[256] = {
  0,
  3,
  3,
  6,
  3,
  6,
  6,
  9,
  3,
  6,
  6,
  9,
  6,
  9,
  9,
  6,
  3,
  6,
  6,
  9,
  6,
  9,
  9,
  12,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  9,
  3,
  6,
  6,
  9,
  6,
  9,
  9,
  12,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  9,
  6,
  9,
  9,
  6,
  9,
  12,
  12,
  9,
  9,
  12,
  12,
  9,
  12,
  15,
  15,
  6,
  3,
  6,
  6,
  9,
  6,
  9,
  9,
  12,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  9,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  15,
  9,
  12,
  12,
  15,
  12,
  15,
  15,
  12,
  6,
  9,
  9,
  12,
  9,
  12,
  6,
  9,
  9,
  12,
  12,
  15,
  12,
  15,
  9,
  6,
  9,
  12,
  12,
  9,
  12,
  15,
  9,
  6,
  12,
  15,
  15,
  12,
  15,
  6,
  12,
  3,
  3,
  6,
  6,
  9,
  6,
  9,
  9,
  12,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  9,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  15,
  9,
  6,
  12,
  9,
  12,
  9,
  15,
  6,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  15,
  9,
  12,
  12,
  15,
  12,
  15,
  15,
  12,
  9,
  12,
  12,
  9,
  12,
  15,
  15,
  12,
  12,
  9,
  15,
  6,
  15,
  12,
  6,
  3,
  6,
  9,
  9,
  12,
  9,
  12,
  12,
  15,
  9,
  12,
  12,
  15,
  6,
  9,
  9,
  6,
  9,
  12,
  12,
  15,
  12,
  15,
  15,
  6,
  12,
  9,
  15,
  12,
  9,
  6,
  12,
  3,
  9,
  12,
  12,
  15,
  12,
  15,
  9,
  12,
  12,
  15,
  15,
  6,
  9,
  12,
  6,
  3,
  6,
  9,
  9,
  6,
  9,
  12,
  6,
  3,
  9,
  6,
  12,
  3,
  6,
  3,
  3,
  0,
};

static float3 vertexInterp(float isolevel, float3 p0, float3 p1, float f0, float f1)
{
  float t;

  if (f1 != f0) {
    if (f1 > f0) {
      t = (isolevel - f0) / (f1 - f0);
    }
    else {
      t = (isolevel - f1) / (f0 - f1);
      t = 1.0f-t;
    }
    // t = (isolevel - f0) / (f1 - f0);
  }
  else {
    t = 0.5f;
  }

  if (t < 0.001f) t = 0.001f;
  if (t > 0.999f) t = 0.999f;

  // TEST 
  float3 l = lerp(p0, p1, t);//0.5f); // ,t 
  return l;
}

class compare_float3 {
public:
  bool operator()(const float3 a, const float3 b) const {
    return (a.x < b.x || (a.x == b.x && (a.y < b.y || (a.y == b.y && (a.z < b.z)))));
  }
};

static void mergeTriangles(float4* pos, int totalVerts,
			   std::vector<unsigned int>& indices,
			   std::vector<float4>& vertices,
			   int &vertexID)
{
  std::map<float3, unsigned int, compare_float3> vertexInfo;

  for (int t = 0; t < totalVerts/3; t++) {

    float3 vrt[3] = {make_float3(pos[t*3+0]),
    		     make_float3(pos[t*3+1]),
    		     make_float3(pos[t*3+2])};

    for (int v = 0; v < 3; v++) {

      float3 &key = vrt[v];

      if (vertexInfo.find(key) == vertexInfo.end()) {

    	vertexInfo[key] = vertexID;

    	vertices.push_back(pos[t*3+v]);
    	indices.push_back(vertexID);

    	vertexID++;
      }
      else {

      	indices.push_back(vertexInfo[key]);
      }
    }
  }
}

static float interpolateScalar(const float* field,
			       const unsigned* res,
			       const float3 &pos)
{
  int lx = std::floor(pos.x);
  int ux = lx+1;
  int ly = std::floor(pos.y);
  int uy = ly+1;
  int lz = std::floor(pos.z);
  int uz = lz+1;

  float x = pos.x - lx;
  float y = pos.y - ly;
  float z = pos.z - lz;

  int ires[3] = {static_cast<int>(res[0])-1, 
		 static_cast<int>(res[1])-1, 
		 static_cast<int>(res[2])-1};

  lx = std::max(0,std::min(ires[0], lx));
  ux = std::max(0,std::min(ires[0], ux));
  ly = std::max(0,std::min(ires[1], ly));
  uy = std::max(0,std::min(ires[1], uy));
  lz = std::max(0,std::min(ires[2], lz));
  uz = std::max(0,std::min(ires[2], uz));

  unsigned lzslab = lz*res[0]*res[1];
  unsigned uzslab = uz*res[0]*res[1];
  int lyr = ly*res[0];
  int uyr = uy*res[0];

  unsigned id[8] = {lx + lyr + lzslab,
		    ux + lyr + lzslab,
		    lx + uyr + lzslab,
		    ux + uyr + lzslab,
		    lx + lyr + uzslab,
		    ux + lyr + uzslab,
		    lx + uyr + uzslab,
		    ux + uyr + uzslab};

  float voxelNodes[8];
  for (int i = 0; i < 8; i++) {
    voxelNodes[i] = field[id[i]];
  }

  float a = (1.0f-x)*voxelNodes[0] + x*voxelNodes[1];
  float b = (1.0f-x)*voxelNodes[2] + x*voxelNodes[3];
  float c = (1.0f-y)*a + y*b;
  a = (1.0f-x)*voxelNodes[4] + x*voxelNodes[5];
  b = (1.0f-x)*voxelNodes[6] + x*voxelNodes[7];
  float d = (1.0f-y)*a + y*b;
  float e = (1.0f-z)*c + z*d;

  return e;
}

float4 computeNormal(const float* volume, 
		     const unsigned* resolution, 
		     const float3 &vert)
{
  float left   = interpolateScalar(volume, resolution, vert + make_float3(-1.0f, 0.0f, 0.0f));
  float right  = interpolateScalar(volume, resolution, vert + make_float3( 1.0f, 0.0f, 0.0f));
  float bottom = interpolateScalar(volume, resolution, vert + make_float3( 0.0f,-1.0f, 0.0f));
  float top    = interpolateScalar(volume, resolution, vert + make_float3( 0.0f, 1.0f, 0.0f));
  float back   = interpolateScalar(volume, resolution, vert + make_float3( 0.0f, 0.0f,-1.0f));
  float front  = interpolateScalar(volume, resolution, vert + make_float3( 0.0f, 0.0f, 1.0f));

  return make_float4(normalize(make_float3(right-left,top-bottom,front-back)), 0.0f);
}

void extractSurface(const float* volume, 
		    const int*res,
		    vtkFloatArray *coords[3],
		    const float isoValue,		    	    
		    std::vector<unsigned int>& indices,
		    std::vector<float4>& vertices,
		    int &vertexID)
{
  std::vector<float4> verticesTmp;
  verticesTmp.resize(0);

  const float borderVal = -4096.0f;
  float field[8];
  float3 v[8];
  
  for (int k = 1; k < res[2]; k++) {
    int km = k-1;

    for (int j = 1; j < res[1]; j++) {
      int jm = j-1;
      
      for (int i = 1; i < res[0]; i++) {
	int im = i-1;
	
	int ids[8] = {im + jm*res[0] + km*res[0]*res[1],
		      i  + jm*res[0] + km*res[0]*res[1],
		      i  + j*res[0]  + km*res[0]*res[1],
		      im + j*res[0]  + km*res[0]*res[1],
		      im + jm*res[0] + k*res[0]*res[1],
		      i  + jm*res[0] + k*res[0]*res[1],
		      i  + j*res[0]  + k*res[0]*res[1],
		      im + j*res[0]  + k*res[0]*res[1]};

	float vs[6] = {coords[0]->GetComponent(im,0),
		       coords[0]->GetComponent(i ,0),
		       coords[1]->GetComponent(jm,0),
		       coords[1]->GetComponent(j ,0),
		       coords[2]->GetComponent(km,0),
		       coords[2]->GetComponent(k ,0)};
	
	v[0] = make_float3(vs[0], vs[2], vs[4]);	
	v[1] = make_float3(vs[1], vs[2], vs[4]);	
	v[2] = make_float3(vs[1], vs[3], vs[4]);
	v[3] = make_float3(vs[0], vs[3], vs[4]);
	v[4] = make_float3(vs[0], vs[2], vs[5]);
	v[5] = make_float3(vs[1], vs[2], vs[5]);
	v[6] = make_float3(vs[1], vs[3], vs[5]);
	v[7] = make_float3(vs[0], vs[3], vs[5]);

	field[0] = 0.0f;
	field[1] = 0.0f;
	field[2] = 0.0f;
	field[3] = 0.0f;
	field[4] = 0.0f;
	field[5] = 0.0f;
	field[6] = 0.0f;
	field[7] = 0.0f;

	for (int node = 0; node < 8; ++node) {	  
	  field[node] = volume[ids[node]];
	}

	// calculate flag indicating if each vertex is inside or outside isosurface
	unsigned int cubeIndex =  uint(field[0] < isoValue);
	cubeIndex += uint(field[1] < isoValue)*2;
	cubeIndex += uint(field[2] < isoValue)*4;
	cubeIndex += uint(field[3] < isoValue)*8;
	cubeIndex += uint(field[4] < isoValue)*16;
	cubeIndex += uint(field[5] < isoValue)*32;
	cubeIndex += uint(field[6] < isoValue)*64;
	cubeIndex += uint(field[7] < isoValue)*128;

	int numVerts = numVertsTable[cubeIndex];

	if (numVerts > 0) {

	  int2 edgeNodes[12] = {make_int2(0, 1),make_int2(1, 2),
	  			make_int2(2, 3),make_int2(3, 0),
	  			make_int2(4, 5),make_int2(5, 6),
	  			make_int2(6, 7),make_int2(7, 4),
	  			make_int2(0, 4),make_int2(1, 5),
	  			make_int2(2, 6),make_int2(3, 7)};

	  float3 vertlist[12];

	  vertlist[ 0] = vertexInterp(isoValue, v[0], v[1], field[0], field[1]);
	  vertlist[ 1] = vertexInterp(isoValue, v[1], v[2], field[1], field[2]);
	  vertlist[ 2] = vertexInterp(isoValue, v[2], v[3], field[2], field[3]);
	  vertlist[ 3] = vertexInterp(isoValue, v[3], v[0], field[3], field[0]);
	  vertlist[ 4] = vertexInterp(isoValue, v[4], v[5], field[4], field[5]);
	  vertlist[ 5] = vertexInterp(isoValue, v[5], v[6], field[5], field[6]);
	  vertlist[ 6] = vertexInterp(isoValue, v[6], v[7], field[6], field[7]);
	  vertlist[ 7] = vertexInterp(isoValue, v[7], v[4], field[7], field[4]);
	  vertlist[ 8] = vertexInterp(isoValue, v[0], v[4], field[0], field[4]);
	  vertlist[ 9] = vertexInterp(isoValue, v[1], v[5], field[1], field[5]);
	  vertlist[10] = vertexInterp(isoValue, v[2], v[6], field[2], field[6]);
	  vertlist[11] = vertexInterp(isoValue, v[3], v[7], field[3], field[7]);

	  for(int iv = 0; iv < numVerts; iv += 3) {

	    for (int jv = 0; jv < 3; jv++) {

	      uint edge = triTable[cubeIndex][iv+jv];
	      float3 vert = vertlist[edge];

	      int idx = field[edgeNodes[edge].x] > field[edgeNodes[edge].y] ? 
						   ids[edgeNodes[edge].x] : ids[edgeNodes[edge].y];
	      verticesTmp.push_back(make_float4(vert, idx));
	    }
	  }
	}
      }
    }
  }
  mergeTriangles(verticesTmp.data(), verticesTmp.size(), indices, vertices, vertexID);
}
