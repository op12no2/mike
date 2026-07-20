# Progress photos

Curated build photos, in date order. Conventions:

- Name: `YYYY-MM-DD-short-desc.jpg`, so the listing sorts
  chronologically.
- JPEG only — GitHub won't render HEIC.
- Web size, not full-res: long edge ~1600 px, quality ~85. Originals
  stay in the photo library; the repo gets copies. Git keeps every
  binary in history forever, so full-res phone photos would bloat every
  future clone.
- Strip metadata before committing — phone photos carry EXIF including
  the GPS coordinates of wherever they were taken, and this repo is
  public.

Import flow: copy exports in, then in this directory:

    mogrify -resize '1600x1600>' -quality 85 -strip *.jpg

`-strip` removes all EXIF including location; `'1600x1600>'` only ever
shrinks. Needs `sudo apt install imagemagick` (not yet installed).

A photo that illustrates a design decision gets linked from the relevant
spec doc — e.g. the finished harness from `hardware.md`.
