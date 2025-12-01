# Advent of Code 

First you will need to generate the meta program to generate the day 

below is the instructions on how to build and run days 

```bash 
## From root of project folder
./bin/build.sh day <day number>         - Build puzzle day number
./bin/build.sh day <day number> -run    - Build and run puzzle day number
./bin/build.sh meta                     - Build the meta program
./bin/build.sh clean                    - clean build outputs
```

Folder structure 
```bash 
bin/            Build script
build/          Build output
inputs/         Advent of code input files for puzzles
src/            code
   src/base     core library (Like my standard lib) 
   src/meta     the meta program 
   src/os       library for os specific things 
   src/puzzles  where the puzzles main files are
thirdparty      external libs (Just xxhash crypto stuff is hard)
```

### Results
This year I am trying to learn and understand simd (I will also try some multi-threading when I see it could be utilized) 
wanted to compare solutions and understand the actual difference and if `-release` mode would actually do some of the optimizations already?

> [!IMPORTANT]
The timings is kinda ghetto and don't really take this as an scientific fact just a someone trying to learn/share. Things could definetly be wrong!

#### Day 1 Results
```bash
Part 1 (scalar): (time: 27 us)
Part 1 (SIMD):   (time: 27 us)
```
So at first glance I kinda knew this would happen. The dial position has a serial dependency: `pos[i+1] = f(pos[i], delta[i+1])`. 
Each dial position depends on the previous one so the compiler had to wait for the previous calculation.
Apparently this is called a loop carried dependency and it kills SIMD.
[Intel docs](https://www.intel.com/content/www/us/en/docs/programmable/683259/19-1/minimize-loop-carried-dependencies.html)
[Compiler explorer link](https://godbolt.org/z/4aexPWeon) 

```bash
Part 2 (scalar slow): (time: 2928 us)
Part 2 (scalar fast): (time: 33 us)
Part 2 (SIMD):        (time: 33 us)
```
So naive solution for scalar version was to simulate every click. Pretty obvious solution but wanted to get on to the SIMD version, so didn't put much thought into it. 
When thinking of the SIMD version you want to process multiple clicks in one shot so you think of it in a more flat way.
Instead of iterating I was thinking how I could load 4 items positions and distances at at time, then calculate when if does a "click",
I can just calculate how many times you cross zero with math. (This should have definitely came to me with out his??) 
If youre at position 50 and move 160 clicks right you do (50+160)/100 which is 2 crossings. @NOTE(Alex): duhh
One division instead of 160 iterations. Though this still has the loop carried dependency issue SIMD thinking led me to the math optimization even though SIMD itself didnt help.
[Compiler explorer link](https://godbolt.org/z/ax1eMx3fT) 
