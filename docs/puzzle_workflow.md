# Current puzzle workflow

The immediate research branch is the supplied PNG. Keep the source image read-only and record each result directory with the image hash and command used. The currently retained public mirror has no `sBIT` chunk, so the earlier `sBIT 4` finding must be treated as a property of a different file until its original bytes can be obtained.

1. Run `png_report` on the original PNG. This writes `report.json`, `extracted_text.txt`, residual images, and bit-plane exports.
2. Compare the 16 low-nibble histogram bins. A highly non-uniform distribution can indicate structured low-bit content.
3. Interpret `low4.raw` in several documented packings (one nibble per byte, high/low nibble pairs, channel-separated streams); retain only reproducible outputs.
4. Run `evidence_text_report` on extracted text or OCR notes. Treat repeated words and numbers as hypotheses, not conclusions.
5. Use public-address report tools only for public address strings. Record provenance, ordering, and transforms before any separate manual validation.

The CUDA path accelerates byte-wise extraction only. It produces the same `low4.raw` bytes as the CPU path and should be checked against a CPU run on at least one image before using it for larger evidence batches.
