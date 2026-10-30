#pragma once
#include <stdint.h>

// Türlicht (30.10.2026): Meldet die Wohnzimmertür „offen“ und ist „Alles AUS“ aktiv,
// wacht der Schlauch von der Tür aus Block für Block auf wie alte Leuchtstoffröhren im
// Horrorfilm: Jeder Block flackert unregelmäßig an und aus, blitzt kurz etwas heller und
// bleibt dann als gedimmtes, warmes Nachtlicht stehen. Die Welle stockt ab und zu, als
// müsste sie sich sammeln. Bei „zu“ geht er ruhig von den Enden zurück zur Tür aus.
// Kein Heap, kein delay, kein echter Zufall (fester Hash, jedes Mal gleich);
// vorzeichenlose Zeitdifferenzen überleben den millis()-Überlauf.
namespace door_light {

// Werksstand der Türlage am Schlauch (0-basiert; Webseite zeigt 335–370).
static const uint16_t DOOR_FIRST = 334;
static const uint16_t DOOR_LAST = 369;
// Eingestellte Türlage (Webseite „Tür: erste/letzte LED“), nur unter xMutex ändern.
struct Range { uint16_t first, last; };
inline Range &doorRange() {
  static Range range = {DOOR_FIRST, DOOR_LAST};
  return range;
}
static const uint16_t BLOCK = 6;            // LEDs je „Startschritt“
static const uint16_t MAX_BLOCKS = 600 / BLOCK + 2;
static const uint32_t RISE_STEP_MS = 120;    // Abstand der Blöcke beim Hochfahren
static const uint32_t FALL_STEP_MS = 60;     // Abstand der Blöcke beim Ausgehen
static const uint32_t DOOR_FADE_MS = 2000;   // Türbereich wacht langsamer auf
static const uint32_t BLOCK_FADE_MS = 900;
static const uint32_t FLICKER_SLOT_MS = 45;  // Länge eines Flacker-Zustands
static const uint8_t PAUSE_EVERY = 5;        // etwa jeder 5. Block stockt
static const uint32_t PAUSE_MIN_MS = 250, PAUSE_SPAN_MS = 400;
static const uint32_t MAX_ON_MS = 30UL * 60UL * 1000UL;  // Sicherheit, falls „zu“ fehlt
static const uint8_t NIGHT_R = 255, NIGHT_G = 120, NIGHT_B = 35;
static const uint8_t NIGHT_LEVEL = 48;       // 0..255, Nachtlicht gedimmt (Werksstand des Reglers)
static const uint8_t PEAK_LEVEL = 110;       // kurzes Aufblitzen des Blocks beim Werksstand

// Aufblitzen wächst mit dem Regler mit (bei 48 genau 110), höchstens 255.
inline uint8_t peakFor(uint8_t night) {
  const uint32_t peak = uint32_t(night) * PEAK_LEVEL / NIGHT_LEVEL;
  return peak > 255 ? 255 : uint8_t(peak);
}

enum Phase : uint8_t { Off = 0, Rising = 1, On = 2, Falling = 3 };

inline uint32_t mix(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}

class Renderer {
 public:
  Phase phase() const { return phase_; }
  bool active() const { return phase_ != Off; }

  void open(uint32_t now) {
    if (phase_ == Rising || phase_ == On) return;  // Wiederholte Meldung
    phase_ = Rising;
    start_ = now;
  }
  void close(uint32_t now) {
    if (phase_ == Off || phase_ == Falling) return;
    phase_ = Falling;
    start_ = now;
  }
  void cancel() { phase_ = Off; }

  // night: Nachtlicht-Stufe 0..255 vom Webregler „Türlicht bei Alles AUS“.
  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t night = NIGHT_LEVEL) {
    if (phase_ == Off || count == 0) return;
    const uint8_t peak = peakFor(night);
    const Range door = doorRange();
    const uint16_t first = door.first < count ? door.first : count - 1;
    const uint16_t last = door.last < count ? door.last : count - 1;
    const uint16_t maxBlock = blockOf(0, first, last) > blockOf(count - 1, first, last)
                                  ? blockOf(0, first, last) : blockOf(count - 1, first, last);
    // Startzeit je Block: gleichmäßiger Schritt plus gelegentliches Stocken.
    uint32_t delay = 0;
    for (uint16_t b = 0; b <= maxBlock && b < MAX_BLOCKS; ++b) {
      riseDelay_[b] = delay;
      delay += b == 0 ? DOOR_FADE_MS / 2 : RISE_STEP_MS;
      if (b > 0 && mix(b * 7919U) % PAUSE_EVERY == 0) delay += PAUSE_MIN_MS + mix(b) % PAUSE_SPAN_MS;
    }
    if (phase_ == Rising && now - start_ >= riseDelay_[maxBlock] + BLOCK_FADE_MS) phase_ = On;
    if (phase_ == On && now - start_ >= MAX_ON_MS) close(now);  // setzt start_ neu
    if (phase_ == Falling && now - start_ >= uint32_t(maxBlock) * FALL_STEP_MS + DOOR_FADE_MS) {
      phase_ = Off;
    }
    const uint32_t tt = now - start_;
    for (uint16_t i = 0; i < count; ++i) {
      const uint16_t block = blockOf(i, first, last);
      const uint32_t fade = block == 0 ? DOOR_FADE_MS : BLOCK_FADE_MS;
      uint16_t level = 0;
      if (phase_ == On) {
        level = night;
      } else if (phase_ == Rising) {
        const uint32_t start = riseDelay_[block];
        if (tt > start) level = wakeLevel(tt - start, fade, block, night, peak);
      } else if (phase_ == Falling) {
        // Von außen zur Tür zurück: der Türbereich geht zuletzt aus.
        const uint32_t start = uint32_t(maxBlock - block) * FALL_STEP_MS;
        const uint32_t x = tt > start ? tt - start : 0;
        const uint32_t out = block == 0 ? DOOR_FADE_MS : BLOCK_FADE_MS * 2 / 3;
        level = x >= out ? 0 : uint16_t(uint32_t(night) * (out - x) / out);
      }
      pixels[i].r = uint32_t(NIGHT_R) * level / 255;
      pixels[i].g = uint32_t(NIGHT_G) * level / 255;
      pixels[i].b = uint32_t(NIGHT_B) * level / 255;
    }
  }

  // Blockabstand zur Tür; der Türbereich selbst ist Block 0.
  static uint16_t blockOf(uint16_t i, uint16_t first, uint16_t last) {
    if (i >= first && i <= last) return 0;
    const uint16_t distance = i < first ? first - i : i - last;
    const uint16_t block = uint16_t((distance + BLOCK - 1) / BLOCK);
    return block < MAX_BLOCKS ? block : MAX_BLOCKS - 1;
  }

  // Aufwachen: in den ersten zwei Dritteln flackert der Block in kurzen Zeitscheiben
  // an/aus, wobei „an“ immer wahrscheinlicher wird; danach ruhiger Übergang zum Nachtlicht.
  static uint16_t wakeLevel(uint32_t x, uint32_t fade, uint16_t block,
                            uint8_t night = NIGHT_LEVEL, uint8_t peak = PEAK_LEVEL) {
    if (x >= fade) return night;
    const uint32_t flicker = fade * 2 / 3;
    if (x < flicker) {
      const uint32_t slot = x / FLICKER_SLOT_MS;
      const uint32_t chance = 40 + 200 * x / flicker;  // von selten an bis fast immer an
      const uint32_t roll = mix((uint32_t(block) << 12) ^ slot ^ 0x9e3779b9U) & 255;
      if (roll >= chance) return 0;
      return roll & 1 ? peak : peak * 2 / 3;  // unruhige Helligkeit beim Zünden
    }
    return uint16_t(peak - int32_t(peak - night) * int32_t(x - flicker) / int32_t(fade - flicker));
  }

 private:
  Phase phase_ = Off;
  uint32_t start_ = 0;
  uint32_t riseDelay_[MAX_BLOCKS] = {};
};

}  // namespace door_light
