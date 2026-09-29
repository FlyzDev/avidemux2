# Waveform UI fork

The waveform feature is intentionally split into two layers:

1. `ADM_mwWaveform`: Qt rendering / interaction only.
2. Waveform indexer + cache: decodes audio independently from live playback and feeds normalized peak envelopes to the widget.

## Display behavior

- Default: one master/combined visual overview across all active audio tracks. It is a max-peak envelope, not an audio sum, so independent tracks cannot phase-cancel or create artificial clipping in the display.
- Optional: separate rows for each active audio track, or each decoded channel inside each track (right-click the waveform).
- A/B markers, current playhead and selection are rendered on top of the waveform.
- Left-clicking the waveform seeks through the existing navigation slider path.

## Data contract

The widget accepts per-channel peak envelopes, derives a max-peak envelope per track, and derives the default master/combined overview across active tracks. It can also accept per-track peaks when channel data is unavailable. Peak extraction must not advance or seek the editor's live audio stream. The indexer will therefore use an independent decoder / stream instance or an equivalent isolated editor-side path before real peak data is connected.

This separation keeps the first UI change low-risk and lets indexing, caching and multitrack decoding evolve independently.

## Peak accumulator

`ADM_WaveformPeakAccumulator` converts timestamped interleaved float PCM into fixed-size per-channel max-peak envelopes. It contains no Qt or playback state, so the decoder/indexer can run independently and feed the UI without coupling waveform math to the live editor stream.

## Editor snapshot

`ADM_buildWaveformSnapshot` copies only immutable indexing inputs out of the live editor: source filenames, edit-segment timing, active track identity, channel count and sample rate. The future worker must consume this snapshot rather than retaining pointers to the live `ADM_Composer` or its audio streams. Internal and external audio tracks are both represented.

## Disk cache

Waveform cache identity includes a schema version, requested bin count, timeline duration, source path/size/mtime, edit-segment timing, and active track/channel metadata. Cache files use a small versioned binary format and are written through a temporary file before rename; a mismatched key or malformed payload is treated as a cache miss.

## Independent indexer

`ADM_generateWaveform` reopens internal media through fresh demuxer/audio-decoder instances and projects each edit segment back onto timeline time before adding PCM to the peak accumulator. One decoder is retained per source for each active track so heavily cut timelines can seek without reopening the same file for every cut. External audio uses a separate `create_edAudioExternal()` instance. No live editor audio stream is consumed or seeked.
