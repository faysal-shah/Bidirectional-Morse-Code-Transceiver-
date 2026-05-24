# International Morse Code Reference

> Standard ITU-R M.1677-1 encoding used by MorseLink.

---

## Alphabet

| Letter | Code   | Letter | Code   | Letter | Code   | Letter | Code   |
|--------|--------|--------|--------|--------|--------|--------|--------|
| A      | `· −`  | H      | `· · · ·` | O  | `− − −` | V   | `· · · −` |
| B      | `− · · ·` | I  | `· ·`  | P      | `· − − ·` | W  | `· − −` |
| C      | `− · − ·` | J  | `· − − −` | Q   | `− − · −` | X  | `− · · −` |
| D      | `− · ·` | K     | `− · −` | R      | `· − ·` | Y    | `− · − −` |
| E      | `·`    | L      | `· − · ·` | S   | `· · ·` | Z    | `− − · ·` |
| F      | `· · − ·` | M   | `− −`  | T      | `−`     |      |       |
| G      | `− − ·` | N     | `− ·`  | U      | `· · −` |      |       |

---

## Numbers

| Digit | Code          | Digit | Code          |
|-------|---------------|-------|---------------|
| 0     | `− − − − −`   | 5     | `· · · · ·`   |
| 1     | `· − − − −`   | 6     | `− · · · ·`   |
| 2     | `· · − − −`   | 7     | `− − · · ·`   |
| 3     | `· · · − −`   | 8     | `− − − · ·`   |
| 4     | `· · · · −`   | 9     | `− − − − ·`   |

---

## Punctuation

| Char | Code           |
|------|----------------|
| `.`  | `· − · − · −`  |
| `,`  | `− − · · − −`  |
| `?`  | `· · − − · ·`  |
| `!`  | `− · − · − −`  |
| `/`  | `− · · − ·`    |
| `@`  | `· − − · − ·`  |

---

## Timing Reference (International Standard)

| Element                         | Duration         |
|---------------------------------|------------------|
| Dot (·)                         | 1 unit           |
| Dash (−)                        | 3 units          |
| Gap between symbols (same char) | 1 unit           |
| Gap between letters             | 3 units          |
| Gap between words               | 7 units          |

MorseLink uses **absolute millisecond timings** (not relative units):

| Event                | `config.h` constant  | Default |
|----------------------|----------------------|---------|
| Max press = dot      | `MORSE_DOT_MAX`      | 300 ms  |
| Silence = commit letter | `MORSE_LETTER_GAP` | 900 ms |
| Silence = insert space | `MORSE_WORD_GAP`   | 2500 ms |

---

## Common Prosigns & Abbreviations

| Prosign / Abbr | Meaning                |
|----------------|------------------------|
| CQ             | Calling any station    |
| DE             | From (this is…)        |
| K              | Go ahead (over)        |
| AR             | End of message         |
| SK             | End of contact (clear) |
| SOS            | Distress call          |
| 73             | Best regards           |
| 88             | Love and kisses        |
| QSL            | Acknowledge receipt    |

---

## Input Tips for MorseLink

- **One-button mode:** Use only the [DOT] button.
  - Tap quickly (< 300 ms) → dot
  - Hold longer (> 300 ms) → dash
- **Two-button mode:** [DOT] always enters a dot; [DASH] always enters a dash.
- After entering the last symbol of a character, wait ~0.9 s for auto-commit.
- After the last character of a word, wait ~2.5 s for an auto-space.
- Press [SEND] at any time to transmit the composed message immediately.
- Press [CLEAR] to wipe the current input without sending.
