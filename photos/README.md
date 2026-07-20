# Progress photos

Curated build photos, in date order.

Workflow: resize and export in Lightroom (Windows side), upload through
the GitHub web UI. Nothing in this directory is ever created or edited
on the Pi — the Pi clone just receives photos read-only via `git pull`.

Conventions:

- Name: `YYYY-MM-DD-short-desc.jpg`, so the listing sorts
  chronologically.
- JPEG only — GitHub won't render HEIC.
- Web size, not full-res: long edge ~1600 px. Git keeps every binary in
  history forever; the originals stay in Lightroom.
- Strip metadata on export — in Lightroom tick "Remove Location Info"
  (or set exported metadata to Copyright Only). Phone photos carry GPS
  coordinates and this repo is public.

A photo that illustrates a design decision gets linked from the relevant
spec doc — e.g. the finished harness from `hardware.md`.
