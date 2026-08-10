import unittest


class Status:
    OK               = 0
    READ_ERROR       = 1
    UNEXPECTED_INPUT = 2
    DUPLICATE_CLUE   = 3
    EOF              = 4


class Grid:
    ORDER1 = 3
    ORDER2 = 9
    ORDER4 = 81

    def __init__(self):
        self.digits = [0] * 81

    def from_string(string):
        if len(string) != 81:
            raise ValueError("a sudoku grid must contain exactly 81 characters")

        g = Grid()

        for i, x in enumerate(string):
            if x == ".":
                g.digits[i] = 0
            elif ord("0") < ord(x) <= ord("9"):
                g.digits[i] = ord(x) - ord("0")
            else:
                raise ValueError("invalid string format")

        return g

    def index(self, block_id, offset):
        index = block_id * order2 + offset

        return self.digits[index]

    def at(self, x, y):
        order1   = self.ORDER1
        block_id = (y // order1) * order1 + (x // order1)
        offset   = (y  % order1) * order1 + (x  % order1)

        return self.index(block_id, offset)

    def block(self, block_id):
        order2 = self.ORDER2
        digits = []

        for offset in range(order2):
            digits.append(self.index(block_id, offset))

        return digits

    def row(self, y):
        order2 = self.ORDER2
        digits = []

        for x in range(order2):
            digits.append(self.at(x, y))

        return digits

    def column(self, x):
        order2 = self.ORDER2
        digits = []

        for y in range(order2):
            digits.append(self.at(x, y))

        return digits

    def duplicates(self):
        from collections import Counter

        order2 = self.ORDER2
        count  = 0

        for i in range(order2):
            for digits in [self.block(i), self.row(i), self.column(i)]:
                counts = Counter(d for d in digits if d != 0)
                count += sum(n * (n - 1) // 2 for n in counts.values())

        return count


def read_status(stream):
    line = stream.readline().decode()
    raw  = line.strip().rstrip("\r\n")

    try:
        status = int(raw)
    except ValueError as exc:
        raise ValueError(f"invalid status: {raw!r}") from exc

    return status


class Report:
    def __init__(self, step, temperature, duplicates, grid):
        self.step        = step
        self.temperature = temperature
        self.duplicates  = duplicates
        self.grid        = grid


def read_report(stream):
    line   = stream.readline().decode()
    raw    = line.strip().rstrip("\r\n")
    fields = line.split(" ")

    if len(fields) != 4:
        raise ValueError(f"malformed report: {raw!r}") from exc

    try:
        step        = int(fields[0])
        temperature = float(fields[1])
        duplicates  = int(fields[2])
        grid        = Grid.from_string(fields[3])
    except ValueError as exc:
        raise ValueError(f"malformed report: {raw!r}") from exc

    return Report(s)


def is_ready(stream):
    import select

    return bool(select.select([stream], [], [], 0)[0])


class Test(unittest.TestCase):
    def test_malformed_requests(self):
        def test_case(request):
            self.process.stdin.write(request.encode());
            self.process.stdin.flush();

            status = read_status(self.process.stdout)

            self.assertEqual(Status.UNEXPECTED_INPUT, status)
            self.assertFalse(is_ready(self.process.stdout))

        requests = [
            ( 0, ""),
            ( 1, " "),
            ( 2, "  "),
            ( 3, "   "),
            ( 4, "a"),
            ( 5, " b"),
            ( 6, " c "),
            ( 7, "0"),
            ( 8, "1"),
            ( 9, "0;"),
            (10, "1;"),
            (11, "-;"),
            (12, "a;"),
            (13, "1;a................................................................................"),
            (14, "1;.......................b........................................................."),
            (15, "3;....................a..b........................................................."),
            (16, "2;................................................................................c"),
            (17, "5;..............................................................................."),
            (18, "10;.............................................................................."),
            (19, "32;.."),
            (20, "12;."),
        ]

        seen = set()

        for i, request in requests:
            assert i not in seen
            seen.add(i)

            with self.subTest(i=i):
                test_case(request + "\n")

    @classmethod
    def setUpClass(cls):
        import os
        import subprocess
        from   pathlib import Path

        exe = os.environ.get("ASH")

        if not exe:
            raise RuntimeError("ASH not set")

        cls.process = subprocess.Popen(
            [Path.cwd() / exe],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
        )

    @classmethod
    def tearDownClass(cls):
        cls.process.terminate()
        cls.process.wait()


if __name__ == "__main__":
    unittest.main()
