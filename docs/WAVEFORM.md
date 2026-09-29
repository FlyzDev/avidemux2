# Waveform UI fork

The waveform feature is intentionally split into two layers:

1. `ADM_mwWaveform`: Qt rendering / interaction only.
2. Waveform indexer + cache: decodes audio independently from live playback and feeds normalized peak envelopes to the widget.

## Display behavior

- Default: one combined overview waveform across active audio tracks. This is a max-peak envelope, not an audio sum, so independent streams cannot cancel or clip each other visually.
- Optional: separate rows for each active audio track (right-click the waveform).
- A/B markers, current playhead and selection are rendered on top of the waveform.
- Left-clicking the waveform seeks through the existing navigation slider path.

## Data contract

The widget accepts a combined peak envelope and optional per-track peak envelopes. Peak extraction must not advance or seek the editor's live audio stream. The indexer will therefore use an independent decoder / stream instance or an equivalent isolated editor-side path before real peak data is connected.

This separation keeps the first UI change low-risk and lets indexing, caching and multitrack decoding evolve independently.
