import unittest


def read_env_ash():
    import os
    from   pathlib import Path

    ash = os.environ.get("ASH")

    if not ash:
        raise RuntimeError("ASH not set")

    return Path.cwd() / ash

ash = read_env_ash()

def spawn_ash():
    import subprocess

    return subprocess.Popen(
        [ash],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE
    )

def kill_ash(process):
    process.terminate()
    process.wait()


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
        order2 = self.ORDER2
        index  = block_id * order2 + offset

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
    fields = raw.split(" ")

    if len(fields) != 4:
        raise ValueError(f"malformed report: {raw!r}") from exc

    try:
        step        = int(fields[0])
        temperature = float(fields[1])
        duplicates  = int(fields[2])
        grid        = Grid.from_string(fields[3])
    except ValueError as exc:
        raise ValueError(f"malformed report: {raw!r}") from exc

    return Report(step, temperature, duplicates, grid)


def is_ready(stream):
    import select

    return bool(select.select([stream], [], [], 0)[0])


class RequestResponseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.process = spawn_ash()

    @classmethod
    def tearDownClass(cls):
        kill_ash(cls.process)

    # 
    #
    # 

    def run_solver_test(self, puzzle):
        self.process.stdin.write(("0;" + puzzle + "\n").encode())
        self.process.stdin.flush()

        status = read_status(self.process.stdout)

        self.assertEqual(Status.OK, status)

        report = read_report(self.process.stdout)

        self.assertEqual(0, report.duplicates)
        self.assertEqual(0, report.grid.duplicates())

    def test_solver_1(self):
        self.run_solver_test(".................................................................................")

    def test_solver_2(self):
        self.run_solver_test("8.1..925...3..71..9.685.47.5..76..32.6183....7.4.......2...5....19...4525..3.2197")

    def test_solver_3(self):
        self.run_solver_test("2.38.9..64..16..3...57.4.197.2..8..1..325.6.7.6......2..793.6....572..9.926...47.")

    def test_solver_4(self):
        self.run_solver_test("34..7125..5..29........598.76.5...3.9..4.7..6..8..2.4.....1........43.......6.524")

    def test_solver_5(self):
        self.run_solver_test(".6....7..2...9.86.319.8.4.....4.7...67..583.2..5......62..5....7.1...5.....62.19.")

    def test_solver_6(self):
        self.run_solver_test("...64..2.6......4.489.5.......18..7...5....2.8.39241....8.....3.9...7....4.2.8..1")

    def test_solver_7(self):
        self.run_solver_test("6......1.9...78..57.....3......8..4.3..7..........365.1.93.4......1.6.....78.....")

    #
    #
    #

    def run_report_frequency_test(self, frequency):
        self.process.stdin.write((str(frequency) + ";.................................................................................\n").encode())
        self.process.stdin.flush()

        status = read_status(self.process.stdout)

        self.assertEqual(Status.OK, status)

        while True:
            report = read_report(self.process.stdout)
            step   = report.step

            if (report.duplicates == 0):
                break

            self.assertEqual(0, step % frequency)

    def test_report_frequency_1(self):
        self.run_report_frequency_test(0)

    def test_report_frequency_2(self):
        self.run_report_frequency_test(256)

    def test_report_frequency_3(self):
        self.run_report_frequency_test(1024)

    def test_report_frequency_4(self):
        self.run_report_frequency_test(8192)

    #
    #
    #

    def run_duplicate_detection_test(self, request):
        self.process.stdin.write((request + "\n").encode())
        self.process.stdin.flush()

        status = read_status(self.process.stdout)

        self.assertEqual(Status.DUPLICATE_CLUE, status)
        self.assertFalse(is_ready(self.process.stdout))

    def test_duplicate_detection_1(self):
        self.run_duplicate_detection_test("0;11...............................................................................")

    def test_duplicate_detection_2(self):
        self.run_duplicate_detection_test("0;...............................................................................33")

    def test_duplicate_detection_3(self):
        self.run_duplicate_detection_test("0;...................22............................................................")

    def test_duplicate_detection_4(self):
        self.run_duplicate_detection_test("0;..................1234556789.....................................................")

    def test_duplicate_detection_5(self):
        self.run_duplicate_detection_test("0;..................123456789...........................123345678..................")

    #
    #
    #

    def run_malformed_request_test(self, request):
        self.process.stdin.write((request + "\n").encode());
        self.process.stdin.flush();

        status = read_status(self.process.stdout)

        self.assertEqual(Status.UNEXPECTED_INPUT, status)
        self.assertFalse(is_ready(self.process.stdout))

    def test_malformed_request_1(self):
        self.run_malformed_request_test("")

    def test_malformed_request_2(self):
        self.run_malformed_request_test(" ")

    def test_malformed_request_3(self):
        self.run_malformed_request_test("  ")
        
    def test_malformed_request_4(self):
        self.run_malformed_request_test("   ")

    def test_malformed_request_5(self):
        self.run_malformed_request_test("a")

    def test_malformed_request_6(self):
        self.run_malformed_request_test(" b")

    def test_malformed_request_7(self):
        self.run_malformed_request_test(" c ")

    def test_malformed_request_8(self):
        self.run_malformed_request_test("0")

    def test_malformed_request_9(self):
        self.run_malformed_request_test("1")

    def test_malformed_request_10(self):
        self.run_malformed_request_test("0;")

    def test_malformed_request_11(self):
        self.run_malformed_request_test("-;")

    def test_malformed_request_12(self):
        self.run_malformed_request_test("1;a................................................................................")

    def test_malformed_request_13(self):
        self.run_malformed_request_test("1;.......................b.........................................................")

    def test_malformed_request_14(self):
        self.run_malformed_request_test("3;....................a..b.........................................................")

    def test_malformed_request_15(self):
        self.run_malformed_request_test("2;................................................................................c")

    def test_malformed_request_16(self):
        self.run_malformed_request_test("5;...............................................................................")

    def test_malformed_request_17(self):
        self.run_malformed_request_test("32;..")

    def test_malformed_request_18(self):
        self.run_malformed_request_test("12;.")


class ReturnValueTest(unittest.TestCase):
    def test_eof(self):
        with spawn_ash() as process:
            process.stdin.close()
            returnValue = process.wait()

            self.assertEqual(0, returnValue)


if __name__ == "__main__":
    unittest.main()
