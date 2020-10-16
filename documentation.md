---
layout: default
---

## Support Documentation

Examples and troubleshooting information for `kirc`.

### Transport Layer Security (TLS) Support

There is no native TLS/SSL support. Instead, users can achieve this functionality by using third-party utilities (e.g. stunnel, [socat](https://linux.die.net/man/1/socat), ghosttunnel, etc).

*   Example using `socat`. Remember to replace items enclosed with `<>`.

```shell
socat tcp-listen:6667,reuseaddr,fork,bind=127.0.0.1 ssl:<irc-server>:6697
kirc -s 127.0.0.1 -c 'channel' -n 'name' -r 'realname'
```

### SASL PLAIN Authentication

In order to connect using `SASL PLAIN` mechanism authentication, the user must provide the required token during the initial connection. If the authentication token is base64 encoded and, therefore, can be generated a number of ways. For example, using Python, one could use the following:

```shell
python -c 'import base64; print(base64.encodebytes(b"nick\x00nick\x00password"))'
```

For example, lets assume an authentication identity of `jilles` and password `sesame`:

```shell
$ python -c 'import base64; print(base64.encodebytes(b"jilles\x00jilles\x00sesame"))'
b 'amlsbGVzAGppbGxlcwBzZXNhbWU=\n'
$ kirc -n jilles -a amlsbGVzAGppbGxlcwBzZXNhbWU=
```

### SASL EXTERNAL Authentication

Similar to `SASL PLAIN`, the `SASL EXTERNAL` mechanism allows us to authenticate using credentials by external means. An example where this might be required is when trying to connect to an IRC host through [Tor](https://www.torproject.org/). To do so, we can using third-party utilities (e.g. stunnel, socat, ghosttunnel, etc).

*   Example using `socat`. Remember to replace items enclosed with `<>`.

```shell
socat TCP4-LISTEN:1110,fork,bind=0,reuseaddr SOCKS4A:127.0.0.1:<onion_address.onion>:<onion_port>,socksport=9050
socat TCP4-LISTEN:1111,fork,bind=0,reuseaddr 'OPENSSL:127.0.0.1:1110,verify=0,cert=<path_to_pem>'
kirc -e -s 127.0.0.1 -p 1111 -n <nick> -x 'wait 5000'
```

### Color Scheme Definition

Applying a new color scheme is easy! One of the quickest ways is to use an application, such as [kfc](https://github.com/mcpcpc/kfc), to apply pre-made color palettes. Alternatively, you can manually apply escape sequences to change the default terminal colors.

*   Example using `kfc`

```shell
kfc -s gruvbox
```

*   Example using ANSI escape sequences

```shell
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
```

[back](./)
