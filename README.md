# Conway's Game of Life in C

I was after a tui implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) to keep me occupied while I should be working and decided I'd make one for the fun of it.

![game-of-life](./rsc/golc.gif)

## Installation

You'll need [git](https://git-scm.com/), [CMake](https://cmake.org/), [make](https://www.gnu.org/software/make/), and the [NCursesW](https://en.wikipedia.org/wiki/Ncurses) library.

```bash
git clone https://github.com/BodneyC/game-of-life
cd game-of-life
cmake . && make
./bin/golc
```

## Usage

Usage is based on key presses, mappings are as follows:

| Key | Short for | Desc.                                          |
| :-- | :--       | :--                                            |
| `r` | Run       | Run the automaton                              |
| `b` | Backup    | Save the state of the program (active cells)   |
| `R` | Restore   | Restore the screen to the state saved with `b` |
| `i` | Iterate   | Perform a single, manual iteration             |
| `s` | Speed     | Reduce the interval rate, speed up             |
| `S` | Slow      | Increase the interval rate, slow down          |

### Practical Usage

1. Click around on the screen, highlight some cells
2. Hit `r` and see it move about a bit
