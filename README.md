# Advent of Code

First you will need to generate the meta program to generate the day.

Below is the instructions on how to build and run days:

```bash
# From root of project folder
./bin/build.sh day <day number>         # Build puzzle day number
./bin/build.sh day <day number> -run    # Build and run puzzle day number
./bin/build.sh meta                     # Build the meta program
./bin/build.sh clean                    # Clean build outputs
```

### Folder Structure

```
bin/            Build script
build/          Build output
inputs/         Advent of code input files for puzzles
src/            Code
├── base        Core library (Like my standard lib)
├── meta        The meta program
├── os          Library for os specific things
└── puzzles     Where the puzzles main files are
thirdparty/     External libs (Just xxhash crypto stuff is hard)
```
