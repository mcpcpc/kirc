---
layout: default
title: documentation
---

# Support Documentation

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Command Aliases](#command-aliases)
- [Key Bindings](#key-bindings)
- [Transport Layer Security Support](#transport-layer-security-support)
- [SASL PLAIN Authentication](#sasl-plain-authentication)
- [SASL EXTERNAL Authentication](#sasl-external-authentication)
- [Color Scheme Definition](#color-scheme-definition)
- [System Notifications](#system-notifications)
- [FAQs](#faqs)

## Installation

Building and installing on **KISS Linux** using the Community repository:

	kiss b kirc
	kiss i kirc

Building and installing on **Arch** and Arch-based distros using the AUR:

	git clone https://aur.archlinux.org/kirc-git.git
	cd kirc
	makepkg -si

Building and installing from source (works on **Raspbian**, **Debian**, **Ubuntu** and many other Unix distributions):

	git clone https://github.com/mcpcpc/kirc.git
	cd kirc
	make
	make install

## Usage

Consult `man kirc` for a full list and explanation of available arguments.

	kirc [-s server] [-p port] [-n nick] [-c chan] ...

## Command Aliases

	<message>                  Send a PRIVMSG to the current channel.
	@<channel|nick> <message>  Send a message to a specified channel or nick.
	@@<channel|nick> <message> Send a CTCP ACTION message to a specified channel or nick.
	/<command>                 Send command to IRC server (see RFC 2812 for full list).
	/#<channel>                Assign new default message channel.

## Key Bindings

A number of key bindings have been supplied to make text editing and string manipulation a breeze! 

	CTRL+B or LEFT ARROW       move the cursor one character to the left.
	CTRL+F or RIGHT ARROW      move the cursor one character to the right.
	CTRL+P or UP ARROW         move to previous record in the input history buffer.
	CTRL+N or DOWN ARROW       move to next record in the input history buffer.
	CTRL+E                     move the cursor to the end of the line.
	CTRL+A or HOME             move the cursor to the start of the line.
	CTRL+W                     delete the previous word.
	CTRL+U                     delete the entire line.
	CTRL+K                     delete the from current character to end of line.
	CTRL+D                     delete the character to the right of cursor.
	CTRL+C                     force quit kirc.
	CTRL+T                     swap character at cursor with previous character.
	CTRL+H                     equivalent to backspace.

## Transport Layer Security Support

There is no native TLS/SSL support. Instead, users can achieve this functionality by using third-party utilities (e.g. stunnel, socat, ghosttunnel, etc).

An example using `socat`. Remember to replace items enclosed with `<>`.

	socat tcp-listen:6667,reuseaddr,fork,bind=127.0.0.1 ssl:<irc-server>:6697
	kirc -s 127.0.0.1 -c 'channel' -n 'name' -r 'realname'


## HTTP/HTTPS Proxy Support

Similar to the TLS example, we can use third-party utilities, such as stunnel or socat, to connect to a proxy server.

	socat tcp-listen:6667,fork,reuseaddr,bind=127.0.0.1 proxy:<proxyurl>:irc.freenode.org:6667;,proxyport=<proxyport>
	kirc -s 127.0.0.1 -c 'channel' -n 'name' -r 'realname'

## SASL PLAIN Authentication

In order to connect using `SASL PLAIN` mechanism authentication, the user must provide the required token during the initial connection. If the authentication token is base64 encoded and, therefore, can be generated a number of ways. For example, using Python, one could use the following:

	python -c 'import base64; print(base64.encodebytes(b"nick\x00nick\x00password"))'

For example, lets assume an authentication identity of `jilles` and password `sesame`:

	$ python -c 'import base64; print(base64.encodebytes(b"jilles\x00jilles\x00sesame"))'
	b 'amlsbGVzAGppbGxlcwBzZXNhbWU=\n'
	$ kirc -n jilles -a amlsbGVzAGppbGxlcwBzZXNhbWU=

## SASL EXTERNAL Authentication

Similar to `SASL PLAIN`, the `SASL EXTERNAL` mechanism allows us to authenticate using credentials by external means. An example where this might be required is when trying to connect to an IRC host through Tor. To do so, we can using third-party utilities (e.g. stunnel, socat, ghosttunnel, etc).

An example using `socat`. Remember to replace items enclosed with `<>`:

	socat TCP4-LISTEN:1110,fork,bind=0,reuseaddr SOCKS4A:127.0.0.1:<onion_address.onion>:<onion_port>,socksport=9050
	socat TCP4-LISTEN:1111,fork,bind=0,reuseaddr 'OPENSSL:127.0.0.1:1110,verify=0,cert=<path_to_pem>'
	kirc -e -s 127.0.0.1 -p 1111 -n <nick> -x 'wait 5000'


## Color Scheme Definition

Applying a new color scheme is easy! One of the quickest ways is to use an application, such as kfc, to apply pre-made color palettes. Alternatively, you can manually apply escape sequences to change the default terminal colors.

An example using `kfc`:

	kfc -s gruvbox

An example using ANSI escape sequences:

	printf -e "\033]4;<color_number>;#<hex_color_code>"

	# Replace <hex_color_code> with the desired Hex code (e.g. #FFFFFF is white).
	# Replace <color_number> with the one of the numbers below:
	# 0 -  Regular Black
	# 1 -  Regular Red
	# 2 -  Regular Green
	# 3 -  Regular Yellow
	# 4 -  Regular Blue
	# 5 -  Regular Magenta
	# 6 -  Regular Cyan
	# 7 -  Regular White
	# 8 -  Bright Black
	# 9 -  Bright Red
	# 10 - Bright Green
	# 11 - Bright Yellow
	# 12 - Bright Blue
	# 13 - Bright Magenta
	# 14 - Bright Cyan
	# 15 - Bright White

## System Notifications

The following is an example script that can be used or modified to send custom system notifications to a specified tool (i.e herbe, wayeherb, etc).  Also, special thanks to soliwilos contributing this one:

	#!/bin/sh
	#
	# checks log file for substring and sends notification and
	# sends message to specified program.

	main () {
	    while true; do
	        tail -fn5 "$1" | awk '/PRIVMSG #.*nick.*/ {
	            system("wayherb \"kirc - new message\"")
	            print "new message recieved!"
	            exit
	        }'
	        sleep 5
	    done
	}

	main "$1"

## FAQs

**KISS** is an acronym for [Keep It Simple Stupid](https://en.wikipedia.org/wiki/KISS_principle), which is a design principle noted by the U.S. Navy in 1960s. The KISS principle states that most systems work best if they are kept simple rather than made complicated; therefore, simplicity should be a key goal in design, and unnecessary complexity should be avoided.

**POSIX** is an acronym for [Portable Operating System Interface](https://opensource.com/article/19/7/what-posix-richard-stallman-explains), which is a family of standards specified by the IEEE Computer Society for maintaining compatibility between operating systems. The *C99* Standard is preferred over other versions (e.g. *C89* or *C11*) since this currently the only one specified by [POSIX](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/c99.html).


