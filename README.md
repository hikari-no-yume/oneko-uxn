oneko-uxn
=========

> The program oneko creates a cute cat chasing around your mouse cursor.

This is a port of [oneko-sakura](http://www.daidouji.com/oneko/) to [uxn](https://wiki.xxiivv.com/site/uxn.html) using [chibicc-uxn](https://github.com/lynn/chibicc).

History
-------

oneko-sakura is one of the the many versions of “Neko”. Lineage: oneko-sakura by Kiichiroh Mukose et al (see [README](http://www.daidouji.com/oneko/distfiles/README)) ← oneko by Tatsuya Kato et al (see [history website](https://web.archive.org/web/20010502181733/http://hp.vector.co.jp/authors/VA004959/oneko/nekohist.html)) ← xneko by Masayuki Koba. The original Neko is Neko.COM by naoshi, see [Japanese Wikipedia](https://ja.wikipedia.org/wiki/Neko_(%E3%82%BD%E3%83%95%E3%83%88%E3%82%A6%E3%82%A7%E3%82%A2)).

This port is based on version `oneko-1.2.sakura.5` from <http://www.daidouji.com/oneko/distfiles/oneko-1.2.sakura.5.tar.gz>. An unmodified copy is included at `original/oneko-1.2.sakura.5.tar.gz`. The rest of the files in this repo are the port. Note that the `README`, `README-NEW`, `README-SUPP`, `oneko.man` and `oneko.man.jp` files are unmodified from the original except for character set conversion.

Changes from oneko-sakura
-------------------------

You can see a diff here: <https://github.com/hikari-no-yume/oneko-uxn/compare/original-UTF-8..trunk>.

New features:

* `-mask` lets you give the neko/cursor mask a different color to the background.

Changed features:

* `-fg`/`-foreground` and `-bg`/`-background` use uxn-style 3-digit hex colours (e.g. `f77` for pink #ff7777).
* `-time` now takes milliseconds rather than microseconds.
* The quit shortcut is Ctrl-Q rather than Alt-Q because the latter doesn't work for me on macOS.
* uxn/varvara does not have an equivalent of `.Xresources`, so instead configuration goes in a `oneko-uxn.defaults` file. An example file:

      foreground: f70
      background: black
      speed: 10
      tora: true

Removed features:

* uxn/varvara does not have a built-in windowing system, so in this port, the neko is stuck inside the window and can't follow other windows. In this way it is like the original xneko.
* `-name` (customizing the window name). Technically this could be supported with the varvara metadata port, but it does not seem to be intended for dynamic names.
* `-display`, because uxn/varvara does not let the application pick which display the window appears on.
* `-debug`, because it used some "synchronize" X feature that is not relevant to uxn.
