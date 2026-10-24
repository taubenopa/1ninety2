# 1ninety-bass

**808 Bass Boost / Crunch / Sub Destroyer** (VST3, Windows + macOS) von **taubenopa**.

## Signalfluss
Input → Crossover (LR4) →
- **Low Band (808 Sub):** Tight (Low-Cut) → Sub Boost (Freq/Q) → Punch (Transient Shaper) → Crunch (6 Saturation-Typen, Bias, Harmonics, Low Clip) 
- **High Band (Grit):** Click (Presence) → Grit Drive (6 Typen)

→ Tone (Hi-Cut) → DC-Filter → Output → Limiter → Mix (latenzkorrigiert). Oversampling 1x/2x/4x, 6 Factory-Presets.

## Bauen lassen (GitHub)
1. Repo pushen. Der Workflow `.github/workflows/build.yml` baut Windows + macOS (Universal arm64/x86_64) und erzeugt:
   - `1ninety-bass-1.0.0-Windows-Setup.exe`
   - `1ninety-bass-1.0.0-macOS.pkg`
2. Download unter *Actions → Run → Artifacts*.
3. Release: `git tag v1.0.0 && git push --tags` – Installer werden automatisch an das GitHub-Release gehaengt.

## Lokal bauen
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Ergebnis: `build/NinetyBass_artefacts/Release/VST3/1ninety-bass.vst3`

## Hinweise zu Signierung
- **macOS:** ohne Apple Developer ID ist der Installer nur ad-hoc signiert (Rechtsklick → Oeffnen). Fuer Notarisierung die Secrets `APP_SIGN_ID` / `PKG_SIGN_ID` setzen.
- **Windows:** ohne Code-Signing-Zertifikat zeigt SmartScreen eine Warnung.

## Hinweis Logo
`assets/logo.png` enthaelt ein Foto einer realen Person. Fuer eine oeffentliche/kommerzielle Veroeffentlichung brauchst du dafuer die Rechte.
