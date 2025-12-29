kirc
====

kirc is a tiny, dependency-light IRC client that follows the
"Keep It Simple, Stupid" (KISS) philosophy. It is intended for
users who prefer a minimal, keyboard-driven IRC experience in a
terminal environment.

Features
--------

* Small, portable C codebase with minimal runtime requirements
* Keyboard-centric editing and navigation
* Simple command and message aliases for fast interaction
* Lightweight build using a standard Makefile

Install
-------

Build and install from source (system-wide install requires
appropriate privileges):

    git clone https://github.com/mcpcpc/kirc
    cd kirc/
    make
    sudo make install

Alternatively, run the binary from the repository root after
building: `./kirc`.

Usage
-----

See `man kirc` for a more usage information.

    kirc [-s server] [-p port] [-c channels] [-r realname]
         [-u username] [-k password] [-a auth] <nickname>

License
-------

See the `LICENSE` file in the repository for details.
