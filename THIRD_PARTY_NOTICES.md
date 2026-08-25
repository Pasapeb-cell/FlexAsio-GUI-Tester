# Third-party notices

| Component | Pinned revision/version | Source | License | Purpose and advisory disposition |
| --- | --- | --- | --- | --- |
| Qt | 6.11.2 (release builds) | https://download.qt.io/official_releases/qt/6.11/6.11.2/ | LGPL-3.0-only/GPL/commercial | GUI runtime. Release builds must use 6.11.2 rather than the vulnerable local 6.8.1 runtime. |
| PortAudio | `18a606e` (`v19.7.0-RC2-160`, reports 19.8) | https://github.com/PortAudio/portaudio | MIT | Audio I/O. Retained rather than regressing to the older 19.7.0 tag; scan in CI gates release findings. |
| libsndfile | `72f6af15` (1.2.2) | https://github.com/libsndfile/libsndfile | LGPL-2.1-or-later | Test tooling dependency; current pinned 1.2.2 release. |
| cxxopts | `44380e5` (3.3.1) | https://github.com/jarro2783/cxxopts | MIT | CLI parsing; update fixes CMake 4 compatibility. |
| tinytoml | `f5a2013` (0.4-2) | https://github.com/mayah/tinytoml | MIT | FlexASIO TOML parser. |
| dechamps_cpputil / cpplog / CMakeUtils / ASIOUtil | pinned submodules | https://github.com/dechamps | MIT | Upstream FlexASIO support utilities. |
| ASIO SDK / ASIOTest | pinned submodules | https://www.steinberg.net/developers/ | Steinberg ASIO SDK license | ASIO driver interface and tests; distributed subject to its upstream terms. |

Exact submodule commits are recorded by the release CI in the SPDX SBOM. Licenses
for distributed components are included in the portable ZIP and installer.
