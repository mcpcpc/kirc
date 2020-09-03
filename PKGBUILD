pkgname=kirc-git
pkgver=0.0.6.r0.6a019c7
pkgrel=1
pkgdesc="KISS for IRC, an IRC client written in POSIX C99"
arch=(any)
url="https://github.com/mcpcpc/kirc"
license=('MIT')
makedepends=('git')
provides=("${pkgname%-git}")
conflicts=("${pkgname%-git}")
replaces=()
backup=()
options=()
install=
source=('kirc::git+https://github.com/mcpcpc/kirc.git')
md5sums=('SKIP')

pkgver() {
	cd "$srcdir/${pkgname%-git}"

	printf "%s" "$(git describe --tags --long | sed 's/\([^-]*-\)g/r\1/;s/-/./g')"
}

build() {
	cd "$srcdir/${pkgname%-git}"
	make
}

package() {
	cd "$srcdir/${pkgname%-git}"
	make DESTDIR="$pkgdir" install
}
