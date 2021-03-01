struct abuf {
    char * b;
    int    len;
};

struct State {
    char * prompt; /* Prompt to display. */
    char * buf;    /* Edited line buffer. */
    size_t buflen; /* Edited line buffer size. */
    size_t plen;   /* Prompt length. */
    size_t pos;    /* Current cursor position. */
    size_t oldpos; /* Previous refresh cursor position. */
    size_t len;    /* Current edited line length. */
    size_t cols;   /* Number of columns in terminal. */
};
