# FlexASIO GUI Tester 0.1.0 Preview

This preview supports Windows 10 version 1809 or newer on x64 hardware. The
installer intentionally uses FlexASIO's existing installer identity, so it will
replace an upstream FlexASIO installation while retaining support for both x64 and
x86 DAWs. The GUI itself is x64 only.

The binaries are intentionally unsigned. Windows SmartScreen may show a warning;
download only from the project release, verify `SHA256SUMS.txt`, and use the
attached Sigstore provenance before proceeding.

The portable ZIP requires the supported Microsoft Visual C++ Redistributable
(x64). The installer bundles and invokes the official Microsoft redistributable.

The Tester verdict is about PortAudio callback and stream behavior only. It does
not establish audible or end-to-end stability. A future release will add loopback
capture for that purpose.
