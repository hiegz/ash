#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define order1 (3)
#define order2 (order1 * order1)
#define order4 (order2 * order2)
#define order8 (order4 * order4)

enum status {
    STATUS_OK               = 0,
    STATUS_READ_ERROR       = 1,
    STATUS_UNEXPECTED_INPUT = 2,
    STATUS_DUPLICATE_CLUE   = 3,

    STATUS_EOF,
};

int get_x(int block_id, int offset) {
    return ((block_id % order1) * order1) + (offset % order1);
}

int get_y(int block_id, int offset) {
    return ((block_id / order1) * order1) + (offset / order1);
}

static int _last_read;

// ...
int read_stdin(int *character) {
    _last_read = getc(stdin);

    switch (_last_read) {
    case EOF:
        if (feof(stdin)) {
            return STATUS_EOF;
        }

        if (ferror(stdin)) {
            return STATUS_READ_ERROR;
        }

        assert(0 && "unreachable");

    default:
        *character = _last_read;

        return STATUS_OK;
    }

    assert(0 && "unreachable");
}

// ...
int unread_stdin() {
    int ret = ungetc(_last_read, stdin);

    if (_last_read != ret)
        return STATUS_READ_ERROR;

    return STATUS_OK;
}

struct swap {
    int block_id;

    int offset0;
    int offset1;
};

struct sparse_set2 {
    int count;

    // Maps items to their index in `dense`.
    int sparse[order2];

    // Stores items in a 'dense' sequence.
    int dense[order2];
};

void sparse_set2_init(struct sparse_set2 *set_ptr) {
    set_ptr->count = 0;
}

int sparse_set2_find(struct sparse_set2 *set_ptr, int item) {
    int count = set_ptr->count;
    int index = set_ptr->sparse[item];

    if (index >= 0 && index < count) {
        int discovered_item = set_ptr->dense[index];

        if (item == discovered_item) {
            // square already open, nothing to be done.
            return index;
        }
    }

    return count;
}

int sparse_set2_insert(struct sparse_set2 *set_ptr, int item) {
    int count = set_ptr->count;
    int index = sparse_set2_find(set_ptr, item);

    if (index != count) {
        return 0;
    }

    set_ptr->sparse[item]  = index;
    set_ptr->dense [index] = item;
    set_ptr->count         = count + 1;

    return 1;
}

int sparse_set2_erase(struct sparse_set2 *set_ptr, int item) {
    int count = set_ptr->count;
    int index = sparse_set2_find(set_ptr, item);

    if (index == count) {
        return 0;
    }

    int last_item = set_ptr->dense[count - 1];

    set_ptr->sparse[last_item] = index;
    set_ptr->dense [index]     = last_item;
    set_ptr->count             = count - 1;

    return 1;
}

struct grid {
    // Number of duplicate pairs within each row and column of a sudoku grid.
    int duplicates;

    // Counts digits within individual rows and columns. Helps incrementally
    // adjust energy. Indexed by x/y coords and digit.
    int counters[2][order2][order2];

    // Array of sets of squares open to modification. Helps efficiently find
    // swap targets. indexed by block id.
    struct sparse_set2 open_square_set_arr[order2];

    // Each block should only contain unique digits. Indexed by block id and
    // offset.
    int digits[order2][order2];
};

void grid_init(struct grid *grid_ptr) {
    grid_ptr->duplicates = 0;

    memset(grid_ptr->counters, 0, sizeof(grid_ptr->counters));

    for (int block_id = 0; block_id < order2; ++block_id) {
        sparse_set2_init(&grid_ptr->open_square_set_arr[block_id]);

        for (int offset = 0; offset < order2; ++offset) {
            sparse_set2_insert(&grid_ptr->open_square_set_arr[block_id], offset);
        }
    }

    for (int block_id = 0; block_id < order2; ++block_id) {
        for (int offset = 0; offset < order2; ++offset) {
            int digit = offset;
            int x     = get_x(block_id, offset);
            int y     = get_y(block_id, offset);

            grid_ptr->digits[block_id][offset] = digit;

            grid_ptr->duplicates += grid_ptr->counters[0][x][digit]++;
            grid_ptr->duplicates += grid_ptr->counters[1][y][digit]++;
        }
    }
}

int grid_close_square(struct grid *grid_ptr, int block_id, int block_offset) {
    return sparse_set2_erase(&grid_ptr->open_square_set_arr[block_id], block_offset);
}

int grid_square_is_closed(struct grid *grid_ptr, int block_id, int block_offset) {
    int count = grid_ptr->open_square_set_arr[block_id].count;
    int index = sparse_set2_find(&grid_ptr->open_square_set_arr[block_id], block_offset);

    return count == index;
}

void grid_swap(struct grid *grid_ptr, struct swap swap) {
    int   digit0 = grid_ptr->digits[swap.block_id][swap.offset0];
    int   digit1 = grid_ptr->digits[swap.block_id][swap.offset1];

    // insert digit1 at offset0

    int x0 = get_x(swap.block_id, swap.offset0);
    int y0 = get_y(swap.block_id, swap.offset0);

    grid_ptr->duplicates -= --grid_ptr->counters[0][x0][digit0];
    grid_ptr->duplicates -= --grid_ptr->counters[1][y0][digit0];
    grid_ptr->duplicates +=   grid_ptr->counters[0][x0][digit1]++;
    grid_ptr->duplicates +=   grid_ptr->counters[1][y0][digit1]++;

    grid_ptr->digits[swap.block_id][swap.offset0] = digit1;

    // insert digit0 at offset1

    int x1 = get_x(swap.block_id, swap.offset1);
    int y1 = get_y(swap.block_id, swap.offset1);

    grid_ptr->duplicates -= --grid_ptr->counters[0][x1][digit1];
    grid_ptr->duplicates -= --grid_ptr->counters[1][y1][digit1];
    grid_ptr->duplicates +=   grid_ptr->counters[0][x1][digit0]++;
    grid_ptr->duplicates +=   grid_ptr->counters[1][y1][digit0]++;

    grid_ptr->digits[swap.block_id][swap.offset1] = digit0;
}

void grid_suggest_swap(struct grid const *grid_cptr, struct swap *swap_ptr) {
    swap_ptr->block_id = rand() % order2;
    swap_ptr->offset0  = rand() % (grid_cptr->open_square_set_arr[swap_ptr->block_id].count - 0);
    swap_ptr->offset1  = rand() % (grid_cptr->open_square_set_arr[swap_ptr->block_id].count - 1);

    if (swap_ptr->offset1 >= swap_ptr->offset0) {
        swap_ptr->offset1++;
    }

    swap_ptr->offset0 = grid_cptr->open_square_set_arr[swap_ptr->block_id].dense[swap_ptr->offset0];
    swap_ptr->offset1 = grid_cptr->open_square_set_arr[swap_ptr->block_id].dense[swap_ptr->offset1];
}

void status(int status) {
    fprintf(stdout, "%d\n", status);
    fflush (stdout);
}

void report(int step, double temperature, struct grid const *grid_cptr) {
    fprintf(stdout, "%d %f %d", step, temperature, grid_cptr->duplicates);
    fprintf(stdout, " ");

    for (int block_id = 0; block_id < order2; ++block_id) {
        for (int block_offset = 0; block_offset < order2; ++block_offset) {
            fprintf(stdout, "%c", grid_cptr->digits[block_id][block_offset] + '1');
        }
    }

    fprintf(stdout, "\n");
    fflush (stdout);
}

int read_non_negative_int(int *result) {
    int c;
    int ret;

    *result = 0;

    while (1) {
        ret = read_stdin(&c);

        if (STATUS_OK != ret) {
            return ret;
        }

        switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            *result *= 10;
            *result += c - '0';

            break;

        default:
            return unread_stdin();
        }
    }
}

int read_puzzle(struct grid *grid_ptr) {
    struct swap swap;

    int i;
    int c;
    int ret;
    int digit;
    int block_id;
    int closing_offset;
    int digit_offset;

    grid_init(grid_ptr);

    for (int i = 0; i < order4; ++i) {
        ret = read_stdin(&c);

        if (STATUS_OK != ret) {
            return ret;
        }

        switch (c) {
        case '.':
            continue;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            digit          = c - '1';
            block_id       = i / order2;
            closing_offset = i % order2;
            digit_offset   = digit;

            while (digit != grid_ptr->digits[block_id][digit_offset]) {
                digit_offset = grid_ptr->digits[block_id][digit_offset];
            }

            if (grid_square_is_closed(grid_ptr, block_id, digit_offset))
            {
                // square we just found is closed. this is only possible when user
                // provides duplicate clues.
                return STATUS_DUPLICATE_CLUE;
            }

            swap.block_id = block_id;
            swap.offset0  = closing_offset;
            swap.offset1  = digit_offset;

            grid_swap(grid_ptr, swap);
            grid_close_square(grid_ptr, block_id, closing_offset);

            continue;

        default:
            ret = unread_stdin();

            if (STATUS_OK != ret) {
                return ret;
            }

            return STATUS_UNEXPECTED_INPUT;
        }
    }

    return STATUS_OK;
}

int read_request(struct grid *grid_ptr, int *report_frequency_ptr) {
    int ret;
    int c;

    // read frequency

    ret = read_non_negative_int(report_frequency_ptr);

    if (STATUS_OK != ret) {
        return ret;
    }

    // read separator (;)

    ret = read_stdin(&c);

    if (STATUS_OK != ret) {
        return ret;
    }

    switch (c) {
    case ';':
        break;

    default:
        ret = unread_stdin();

        if (STATUS_OK != ret)
            return ret;

        // fprintf(stdout, "here %c\n", c);

        return STATUS_UNEXPECTED_INPUT;
    }

    // read puzzle

    ret = read_puzzle(grid_ptr);

    if (STATUS_OK != ret) {
        return ret;
    }

    return STATUS_OK;
}

int read_to_next_request() {
    int ret;
    int c;

    while (1) {
        ret = read_stdin(&c);

        if (STATUS_OK != ret) {
            return ret;
        }

        if ('\n' == c) {
            break;
        }
    }

    return STATUS_OK;
}

double compute_energy_delta_deviation(struct grid *grid_ptr) {
    struct swap swap;

    int    before;
    int    after;
    int    delta;
    int    sample_count = order8;
    int    samples[sample_count];
    int    sum_of_samples;
    int    sample_mean;
    double error;
    double sum_of_errors;
    double sample_std;

    sum_of_samples = 0;

    for (int j = 0; j < sample_count; ++j) {
        before = grid_ptr->duplicates;

        grid_suggest_swap(grid_ptr, &swap);
        grid_swap(grid_ptr, swap);

        after = grid_ptr->duplicates;
        delta = after - before;

        // undo
        grid_swap(grid_ptr, swap);

        //

        samples[j]      = delta;
        sum_of_samples += samples[j];
    }

    sample_mean   = sum_of_samples / (double)(sample_count);
    sum_of_errors = 0;

    for (int j = 0; j < sample_count; ++j) {
        error          = (double)samples[j] - sample_mean;
        sum_of_errors += (error * error);
    }

    return sqrt(sum_of_errors / (double)(sample_count));
}

#define relaxation_time (order8)
#define patience        (order8)

int run_once() {
    int ret;
    int ret2;

    // sudoku grid
    struct grid grid;

    // how often does the user want to receive solver state?
    int report_frequency;

    // read user input
    //
    // read next request + newline

    ret  = read_request(&grid, &report_frequency);
    ret2 = read_to_next_request();

    if (STATUS_OK != ret) {
        status(ret);
        return ret;
    }

    if (STATUS_OK != ret2) {
        status(ret2);
        return ret2;
    }

    status(STATUS_OK);

    //
    //
    //

    int reports_enabled = report_frequency != 0;

    struct swap swap;

    int    step = -1;
    int    steps_since_cooling;
    int    steps_since_improvement;
    int    steps_since_report;

    double temperature;

reheat:
    temperature             = compute_energy_delta_deviation(&grid);
    steps_since_cooling     = 0;
    steps_since_improvement = 0;
    steps_since_report      = report_frequency; // report before first step

    while (1) {
        step++;

        if (grid.duplicates == 0 || (reports_enabled && steps_since_report == report_frequency)) {
            report(step, temperature, &grid);

            steps_since_report = 0;
        }

        if (grid.duplicates == 0) {
            break;
        }

        // ^^^
        // before step

        int before = grid.duplicates;

        grid_suggest_swap(&grid, &swap);
        grid_swap(&grid, swap);

        int after = grid.duplicates;
        int diff  = after - before;

        if (diff > 0 && ((double)rand() / RAND_MAX) >= exp(-diff / temperature)) {
            // reject swap
            grid_swap(&grid, swap);

            assert(grid.duplicates == before);
        }

        if (diff >= 0) {
            ++steps_since_improvement;
        }

        if (diff < 0) {
            steps_since_improvement = 0;
        }

        ++steps_since_cooling;
        ++steps_since_report;

        // after step
        // vvv

        if (steps_since_cooling == relaxation_time) {
            steps_since_cooling = 0;
            temperature         = 0.995 * temperature;
        }

        if (steps_since_improvement == relaxation_time) {
            goto reheat;
        }
    }

    return STATUS_OK;
}

int run() {
    while (1) {
        switch (run_once()) {
        case STATUS_EOF:
            return 0;

        case STATUS_READ_ERROR:
            return 1;
        }
    }
}

int main() {
    return run();
}
