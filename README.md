# ash

A sudoku solver implemented in C using simulated annealing.

## Building

```
gcc -std=c11 -O3 ash.c -o ash
```

Adjust compiler flags according to your use case.

## Protocol

Ash communicates over standard input and output using a line-oriented text
protocol. Standard error stream is not used.

### Request

Each request has the following format:

```
<report_frequency>;<puzzle>\n
```

For example:

```
100;...64..2.6......4.489.5.......18..7...5....2.8.39241....8.....3.9...7....4.2.8..1
```

#### Report Frequency

`report-frequency` is a non-negative decimal integer. It controls how
frequently the solver emits progress reports.

| Value | Behavior                        |
|-------|---------------------------------|
| 0     | Disable solver progress reports |
| 1     | Report every step               |
| 100   | Report every 100 steps          |
| N     | Report every N steps            |

#### Puzzle

`puzzle` must contain exactly 81 characters.

| Character | Meaning           |
|-----------|-------------------|
| .         | Empty cell        |
| 1-9       | Fixed sudoku clue |

No other characters are permitted.

Ash uses a block-oriented rather than the conventional row-major
Sudoku representation. For example, the puzzle

```
..6..1..59..8....4...3..61.2..3.....7....9...4....7.968.......34......5.1.....7..
```

would be serialized as

```
+-------+-------+-------+
| . . 6 | 9 . . | . . . |
| . . 1 | 8 . . | 3 . . |
| . . 5 | . . 4 | 6 1 . |
+-------+-------+-------+
| 2 . . | 7 . . | 4 . . |
| 3 . . | . . 9 | . . 7 |
| . . . | . . . | . 9 6 |
+-------+-------+-------+
| 8 . . | 4 . . | 1 . . |
| . . . | . . . | . . . |
| . . 3 | . 5 . | 7 . . |
+-------+-------+-------+
```

Requests are processed sequentially until `EOF`.

### Response Status

Each request produces exactly one response status line:

```
<status>\n
```

The following status codes are defined:

| Code | Name               | Meaning                                                             |
|------|--------------------|---------------------------------------------------------------------|
| 0    | `OK`               | The request was successfully parsed and accepted by the solver      |
| 1    | `READ_ERROR`       | An internal error occurred while reading the request                |
| 2    | `UNEXPECTED_INPUT` | The request is malformed                                            |
| 3    | `DUPLICATE_CLUE`   | The puzzle contains duplicate clues that violate Sudoku constraints |

### Solver Reports

After a request has been successfully parsed and accepted by the solver
(`OK`; see above), ash emits progress reports at the user-defined frequency,
using the format defined below:

```
<step> <temperature> <duplicates> <candidate solution>\n
```

Reports are emitted until the puzzle is solved.

#### Step

A non-negative integer representing the number of solver iterations performed.

#### Temperature

Current annealing temperature.

#### Duplicates

Current number of duplicate pairs in the candidate solution violating Sudoku
constraints.

#### Candidate Solution

Current candidate solution. 

Uses the same block-oriented representation as puzzles.

## Determinism

Ash uses randomized optimization algorithm. Therefore, users should not
expect two independent executions of the same request to produce identical
solutions or intermediate reports.

## Testing

```
ASH=<path-to-ash> python test.py
```
