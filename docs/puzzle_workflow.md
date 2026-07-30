# Current puzzle workflow

The immediate research branch is the supplied PNG. Keep the source image read-only and record each result directory with the image hash and command used. The currently retained public mirror has no `sBIT` chunk, so the earlier `sBIT 4` finding must be treated as a property of a different file until its original bytes can be obtained.

1. Run `analyze-image` on the original PNG. This writes `report.txt` and `low4.raw`.
2. Compare the 16 low-nibble histogram bins. A highly non-uniform distribution can indicate structured low-bit content.
3. Interpret `low4.raw` in several documented packings (one nibble per byte, high/low nibble pairs, channel-separated streams); retain only reproducible outputs.
4. Treat extracted text or word lists as hypotheses. Record provenance, ordering, and transform before any address validation.
5. Use the `address` command to independently derive a P2PKH address for a known candidate private key, and retain the exact input/output log.

The CUDA path accelerates byte-wise extraction only. It produces the same `low4.raw` bytes as the CPU path and should be checked against a CPU run on at least one image before using it for larger evidence batches.
