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

Examples
--------

Generate a BASE64 authentication token and connect
using the SASL PLAIN mechanism:

    python -c "import base64;print(base64.encodebytes(b'alice\x00alice\x00password'))"
    # output: b'YWxpY2UAYWxpY2UAcGFzc3dvcmQ=\n'
    kirc -a PLAIN:YWxpY2UAYWxpY2UAcGFzc3dvcmQ= alice

Connect with TLS/SSL support using socat:

    socat tcp-listen:6667,reuseaddr,fork,bind=127.0.0.1 ssl:irc.example.org:6697
    kirc -s 127.0.0.1 alice

Connect to an HTTP/HTTPS proxy server using
socat:

    socat tcp-listen:6667,fork,reuseaddr,bind=127.0.0.1 \
         proxy:<proxyurl>:irc.example.org:6667,proxyport=<proxyport>
    kirc -s 127.0.0.1 -p 6667 alice

License
-------

See the `LICENSE` file in the repository for details.
