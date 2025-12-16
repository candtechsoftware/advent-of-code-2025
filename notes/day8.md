# Day 8 Results

## Part 1

First we need all possible connections. With N points, every pair (i, j) where i < j is a potential edge,
Generate all N*(N-1)/2 edges between points,
find the k=1000 smallest distances,
a union those pairs together. (Makes sense to use a union find algo here.) Pretty much exactly what the problem is asking.


```c
for (u64 i = 0; i < count; i++) {
    for (u64 j = i + 1; j < count; j++) {
        s64 dx = points[i].x - points[j].x;
        s64 dy = points[i].y - points[j].y;
        s64 dz = points[i].z - points[j].z;
        edges[idx].a = i;
        edges[idx].b = j;
        edges[idx].dist_sq = dx*dx + dy*dy + dz*dz; // @Note this should be optimized. 
        idx++;
    }
}
```
then just find k min in just a O(n^2) loop pretty simple kinda ugly though?
A easy opimiztion is to just sort them with the qsort func (libc has one). 

### Making is super fast using SPMD (Single Process multiple data or lanes)
Also instead of qsort, build a radix sort which does really good in parrellel, and qsort not so much 
quicksort has the pivot choice, you can't partition in parallel easily because
threads would fight over where elements go. (@Note(Alex) Thread contention, I want to unpack this more and see if there is a better sorting algo or pivot strategt for this) 
radix sort, sorts by looking at one "digit" (8 bits) at a time. 8 passes for 64-bit keys.
so for each pass:
- See how many elements have each byte value (0-255)? Each lane counts its range. No conflicts.
- Lane 0 computes where each bucket starts. Serial but smalle (256 entries)
- Go "wide" with each lane knows its offset within each bucket.

#### SPMD Approach 
[SPMD resource](https://www.geeksforgeeks.org/computer-organization-architecture/single-program-multiple-data-spmd-model/) 
[pdf lecture](https://web.stanford.edu/class/cs315b/lectures/lecture02.pdf)
[Nvidia Paper](https://www.cs.utexas.edu/~skeckler/pubs/MICRO_2014_Predication.pdf) 

Traditional thread pool approach would have been spawning multiple threads and giving it a function pointer to 
run and a thread would just do some type of work stealing, I am sure this could be super optimized? Maybe if we just launched all jobs at ones but that is already 
kinda SPMD but with more contention as the lanes sync and barriers help to clear this up as all threads are running the same function with shared and subset data. 
It's very simlar to the SIMD (single instruction multiple data but at a higher level)
more like fragment shader which is the same program ran for every pixel but has uniform data that
is pre-orginized before execution. Each core knows its `lane_idx()` and how many total `lane_count()`.

Serial version:
```c
for (i = 0; i < point_count; i++) {
    for (j = i+1; j < point_count; j++) {
        edges[idx++] = make_edge(i, j);
    }
}
```

Lane version - each lane handles a range of `i` values:
```c
Rng1U64 range = lane_range(point_count); // Grabs this lanes work. 
for (i = range.min; i < range.max; i++) {
    for (j = i+1; j < point_count; j++) {
        u64 idx = ins_atomic_u64_inc_eval(edge_counter) - 1;  // grab next slot atomically Not sure we need to do this but I need to play around with this more I think we can get away with removing this but for now I am keeping it cause it works?  @Note(Alex) research this? 
        edges[idx] = make_edge(i, j);
    }
}
lane_sync();
```

In the single threaded uf_find does path compression, 
but If multiple lanes call this simultaneously,
they race to write `parent[i]`. so I had to ensure the uf_find func didn't compress the path so we 
do a read only pass.
```c
u32 uf_find_read_only(u32 *parent, u32 i) {
    while (parent[i] != i) i = parent[i];  // read only
    return i;
}
```

