---
layout: default
---

## About

**kirc** is a tiny open-source Internet Relay Chat (IRC) client designed with usability and cross-platform compatibility in mind.

## Features

*   No dependencies other than a C99 compiler.
*   Native SASL PLAIN and EXTERNAL authentication support.
*   TLS/SSL protocol capable (via external TLS utilities).
*   Full chat history logging.
*   Multi-channel joining at server connection.
*   Simple command aliases and full support for all RFC 2812 commands.
*   Easy color scheme definition via ANSI 8-bit colors.

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

## Usage

Consult `man kirc` for a full list and explanation of available `kirc` arguments.

```shell
kirc [-s server] [-p port] [-n nick] [-c chan] ...
```

### Command Aliases

```shell
<message>                 Send a PRIVMSG to the current channel.
@<channel|nick> <message> Send a message to a specified channel or nick 
/<command>                Send command to IRC server (see RFC 2812 for full list).
/#<channel>               Assign new default message channel.
```

### User Input Key Bindings

| Key Binding           | Behavior Description                               |
|-----------------------|----------------------------------------------------|
| CTRL+B or LEFT ARROW  | moves the cursor one character to the left.        |
| CTRL+F or RIGHT ARROW | moves the cursor one character to the right.       |
| CTRL+A                | moves the cursor to the end of the line.           |
| CTRL+E or HOME        | moves the cursor to the start of the line.         |
| CTRL+W                | deletes the previous word.                         |
| CTRL+U                | deletes the entire line.                           |
| CTRL+K                | deletes the from current character to end of line. |