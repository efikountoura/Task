# 5G NR Downlink Link Adaptation Simulator

Standalone C++17 app that simulates a simplified downlink link-adaptation loop for a single UE, over ~1,000 transmission intervals:

## What it does

- Generates a time-varying SINR: `SINR(t) = average SINR + random variation` (Gaussian)
- Maps SINR → CQI using fixed thresholds (-6 to 22 dB, 2 dB steps, 16 CQI levels)
- Maps CQI → MCS dynamically: picks the highest MCS whose spectral efficiency doesn't exceed the CQI's (per 3GPP TS 38.214 tables)
- Estimates throughput = SE(MCS) × Bandwidth (20 MHz) × (1 − overhead)
- Reports average/min/max over the run, and exports per-interval results to CSV

## Key assumptions

- SINR-CQI thresholds and CQI-MCS matching are simplifications of the real, non-linear BLER-based process (3GPP TS 38.214, Tables 5.2.2.1-2 and 5.1.3.1-1)
- CQI 1 has no MCS satisfying `SE(MCS) ≤ SE(CQI)` — handled by explicitly falling back to MCS 0 (most robust), since CQI 1 is a valid operating point, not "out of range"
- Fixed bandwidth/overhead (no multi-user scheduling, OLLA, HARQ, or MIMO modeled)


