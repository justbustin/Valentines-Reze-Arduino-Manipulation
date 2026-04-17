# Valentines-Reze-Arduino 

<div align="center">
<img width="359" height="406" alt="Untitled1-ezgif com-optimize" src="https://github.com/user-attachments/assets/550a7999-4d55-4785-90c0-36931ccc37d6" />


</div>

Simple interactive touchscreen project powered by arduino! Specifically the Arduino Uno + Elegoo 2.8" TFT shield. Made as a cute gift, and can cloned and customized for your use case!

All user-editable config lives in one block at the top of the sketch. swap the images, tweak the text, flash, done.

## What it does

1. **The chase.** "Will you be my VALENTINE?" with YES and NO buttons. YES gets a taunt. NO teleports to a random spot. After 3 chases, act 2 begins.
2. **The animation.** RAW frames flash on screen.
3. **The carousel.** A final image appears with a second YES prompt. Pressing it triggers a win screen and an infinite BMP photo slideshow. The slideshow is meant to act as a desk decoration to see images loop throughout the day!

## Hardware

| Part | Link |
|---|---|
| Arduino Uno R3 | https://www.amazon.com/dp/B008GRTSV6 |
| Elegoo 2.8" TFT Touch Shield (320×240, with microSD slot) | https://www.amazon.com/dp/B01EUVJYME |
| 9V battery holder w/ switch | https://www.amazon.com/dp/B0756BFGWY |
| microSD card (≤32 GB, **FAT32**) | any |
| 9V battery | any |

A 9V wall adapter (5.5×2.1mm center-positive) works too if you'd rather stay plugged in.

## Setup

1. Install libraries in the Arduino IDE: `Elegoo_GFX`, `Elegoo_TFTLCD`, `TouchScreen`. (`SD` and `SPI` are built in.)
2. Load your images onto the SD card (see below).
3. Open `valentine_display.ino`, edit the `USER CONFIGURATION` block, and flash to an Arduino Uno.
4. Seat the shield on the Uno, insert the SD card, plug in the battery.

## Images

Drop files on the SD card using these names — counts are auto-detected, so add as many frames as you want:

| File | Format | When shown |
|---|---|---|
| `ANIMATION_01.RAW`, `ANIMATION_02.RAW`, ... | RGB565 raw | Flashes before the second YES |
| `FINAL.BMP` | 24-bit BMP | Shown with the second YES prompt |
| `CAROUSEL_1.BMP`, `CAROUSEL_2.BMP`, ... | 24-bit BMP | Loops forever after the final YES |

**Format rules:**
- **BMPs** must be **24-bit**, uncompressed, ≤ 320×240. 32-bit or indexed BMPs will silently fail to draw.
- **RAW** files are RGB565, little-endian, no header. Every animation frame must match `ANIMATION_WIDTH × ANIMATION_HEIGHT` exactly (`width × height × 2` bytes). Default is 224×168. I use [LCD Image Converter](https://lcd-image-converter.riuson.com/) — export as `RGB 565`, rename to `.RAW`.
- **Filenames** are limited to 8.3 by the Arduino SD library. `ANIMATION_01.RAW` gets truncated — shorten the prefixes (e.g. `ANIM_`, `CARO_`) in the sketch if you hit this.

An example animation is included at [`reze-animation/`](reze-animation/) — RAW frames you can drop straight onto the card to test. Carousel BMPs are not included; those are meant to be your own photos.

## Customization

All at the top of the sketch:

- **Text** — every on-screen string is a `TXT_*` define
- **Behavior** — `NO_PRESSES_TO_WIN`, `CAROUSEL_FRAME_MS`
- **Image positioning** — `ANIMATION_X/Y`, `ANIMATION_WIDTH/HEIGHT`, `FINAL_IMAGE_X/Y`


## License

MIT.
