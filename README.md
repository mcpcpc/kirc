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
At minimum, supply a nickname to connect. For full options,
see the manpage.

    kirc [-s server] [-p port] [-c channels] [-r realname]
         [-u username] [-k password] [-a auth] <nickname>

Examples
--------

Connect to the default host with nickname `alice`:

    kirc alice

Connect to a specific server and join channels:

    kirc -s irc.example.org -p 6667 \
        -c "#foo,#bar" alice

Connect and create a log file:

    kirc -s irc.example.org -p 6667 mynick \
        | tee -a kirc-$(date +%Y%m%d).log

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

Commands & Aliases
------------------

    @<dest> <message>   send `PRIVMSG` to a specified user or channel
    /<command>          send a raw command to the IRC server
    /#<channel>         set a new default message channel
    /me <action>        send a CTCP ACTION to target
    /ctcp <nick> <cmd>  send a raw CTCP command (e.g. “CLIENTINFO”,
                        “DCC”, “PING”, “TIME“, and “VERSION”)

Key Bindings
------------

Control keys are mapped to familiar text-editing bindings:

    CTRL+B             move cursor left
    CTRL+F             move cursor right
    CTRL+P             navigate command history (previous)
    CTRL+N             navigate command history (next)
    CTRL+E             move cursor to end of line
    CTRL+A             move cursor to start of line
    CTRL+U             delete entire line
    CTRL+C             force quit kirc

Configuration
-------------

kirc is intentionally minimal and uses command-line options for
configuration. If you need persistent settings, wrap your
invocation in a shell script or alias.

For best results and rendering, use a terminal emulator with 256
color support, such as `xterm-256color`.


License
-------

See the `LICENSE` file in the repository for details.
