#ifndef STATUS_H
#define STATUS_H

enum status {
    STATUS_OK               = 0,
    STATUS_READ_ERROR       = 1,
    STATUS_UNEXPECTED_INPUT = 2,
    STATUS_DUPLICATE_CLUE   = 3,

    STATUS_EOF,
};

#endif
