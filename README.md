# chopwatcher

This program uses the FSEvents API to monitor .wav files dropped into the directory you pass it. It will rename those files appropriately with a numeric prefix. That is to say, if you drop `foo.wav`, it will become `000_foo.wav`; if you then drop `bar.wav`, you'll yield `001_bar.wav`.

I wrote this because TidalCycles likes to refer to samples in a given directory with a numeric identifier, and I was tired of manually figuring out the number to which a given sample corresponded.

To run: `just run DIR`.

# Dependencies

* Bazel, with `bzlmod` support
* `just` if you want a quick way to build and run it

# Supported OSes

Only macOS for now 😞 Patches to make this use a cross-platform file watcher are most welcome.

# License

Public domain.
