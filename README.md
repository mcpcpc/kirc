<h3 align="center"><img src="https://raw.githubusercontent.com/mcpcpc/kirc/master/.github/kirc.png" alt="logo" height="170px"></h3>
<p align="center">KISS for IRC, a tiny IRC client written in POSIX C99.</p>
<p align="center">
<a href="./LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg"></a>
<a href="https://github.com/mcpcpc/kirc/releases"><img src="https://img.shields.io/github/v/release/mcpcpc/kirc.svg"></a>
<a href="https://repology.org/metapackage/kirc"><img src="https://repology.org/badge/tiny-repos/kirc.svg" alt="Packaging status"></a>
<a href="https://www.codacy.com/manual/mcpcpc/kirc/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=mcpcpc/kirc&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/5616c0ed4b2f4209826038dbc270dbf5" alt="Codacy status"></a>
</p>

## Features

*   Excellent cross-platform compatibility (due to [POSIX](https://opensource.com/article/19/7/what-posix-richard-stallman-explains) standard compliance).
*   No dependencies other than a [C99 compiler](https://en.wikipedia.org/wiki/C99).
*   Native [PLAIN SASL](https://tools.ietf.org/html/rfc4422) authentication support.
*   [TLS/SSL](https://en.m.wikipedia.org/wiki/Transport_Layer_Security) protocol capable (via external TLS utilities).
*   Chat history logging.
*   Multi-channel joining at server connection.
*   Simple shortcut commands and full support for all IRC commands in the [RFC 2812](https://tools.ietf.org/html/rfc2812) standard:

```shell
<message>                 Send a PRIVMSG to the current channel.
@<channel|nick> <message> Send a message to a specified channel or nick 
/<command>                Send command to IRC server (see RFC 2812 for full list).
/#<channel>               Assign new default message channel.
/?                        Print current message channel.
```

*   Color scheme definition via [ANSI 8-bit colors](https://en.wikipedia.org/wiki/ANSI_escape_code). Therefore, one could theoretically achieve uniform color definition across all shell applications and tools.

## Screenshots

![Screenshot 1](/.github/example.png)

![Screenshot 1](/.github/example2.png)

## Usage

```shell
usage: kirc [-s hostname] [-p port] [-c channel] [-n nick] [-r real name] [-u username] [-k password] [-x init command] [-w columns] [-W columns] [-o path] [-h|v|V]
-s     server address (default: 'irc.freenode.org')
-p     server port (default: '6667')
-c     channel name(s), delimited by a "," or "|" character (default: 'kirc')
-n     nickname (required)
-u     server username (optional)
-k     server password (optional)
-a     PLAIN SASL authentication token (optional)
-r     real name (optional)
-v     version information
-V     verbose output (e.g. raw stream)
-o     output path to log irc stream
-x     send command to irc server after inital connection
-w     maximum width of the printed left column (default: '20')
-W     maximum width of the entire printed stream (default '80')
-h     basic usage information
```

## Installation

Building and installing on **KISS Linux** using the Community repository:

```shell
kiss b kirc
kiss i kirc
```

Building and installing on **Arch** and **Arch-based** distros using the AUR:

```shell
git clone https://aur.archlinux.org/kirc-git.git
cd kirc
makepkg -si
```

Building and installing from source (works on **Raspbian**, **Debian**, **Ubuntu** and many other Unix distributions):

```shell
git clone https://github.com/mcpcpc/kirc.git
cd kirc
make
make install
```

## Transport Layer Security (TLS) Support

There is no native TLS/SSL support. Instead, users can achieve this functionality by using third-party tools (e.g. stunnel, socat, ghosttunnel).

*   _socat_ example:

```shell
socat tcp-listen:6667,reuseaddr,fork,bind=127.0.0.1 ssl:<irc-server>:6697
kirc -s 127.0.0.1 -c 'channel' -n 'name' -r 'realname'
```

## PLAIN SASL Authentication

In order to connect using PLAIN SASL authentication, the user must provide the required token during the initial connection. If the authentication token is base64 encoded and, therefore, can be generated a number of ways. For example, using Python, one could use the following:

```shell
python -c 'import base64; print(base64.encodebytes(b"nick\x00nick\x00password"))'
```

For example, lets assume an authentication identity of `jilles` and password `sesame`:

```shell
$ python -c 'import base64; print(base64.encodebytes(b"jilles\x00jilles\x00sesame"))'
b 'amlsbGVzAGppbGxlcwBzZXNhbWU=\n'
$ kirc -n jilles -a amlsbGVzAGppbGxlcwBzZXNhbWU=
```

# EXTERNAL SASL Authentication (for Tor)

In order to connect using EXTERNAL SASL, you will first need to generate a client certificate.  We can do so using `openssl`:

```shell
openssl req -x509 -sha256 -new -newkey rsa:4096 -days 1000 -nodes -out client.pem -keyout client.pem
```



## Contact

For any further questions or concerns, feel free to reach out to me on `#kirc`
or `#kisslinux` channels of the _irc.freenode.org_ server.
