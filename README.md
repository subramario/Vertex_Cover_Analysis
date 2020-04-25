# Vertex_Cover_Analysis
Analyzed algorithmic efficiency of three approaches to solving the minimum vertex cover for undirected graphs of various sizes.

## Example

**Input:**
```
V 10
E {<1,2>,<7,1>,<3,6>,<4,6>,<0,7>,<2,8>,<0,9>,<5,4>,<8,4>,<2,4>,<3,4>,<0,6>,<4,1>,<9,1>,<6,5>}
```
**Output:**
```
Vertex Cover for CNF-SAT-VC: 2,4,6,7,9
Vertex Cover for APPROX-VC-1: 0,1,2,4,6
Vertex Cover for APPROX-VC-2: 0,1,2,3,4,5,6,7

CNF-SAT-VC Runtime: 46646.3 microseconds
APPROX-VC-1 Runtime: 139.341 microseconds
APPROX-VC-2 Runtime: 112.531 microseconds

Approximation ratio for APPROX-VC-1:  1.00
Approximation ratio for APPROX-VC-2:  1.60
```
