#	$NetBSD: brotli-rename.mk,v 1.1.2.1 2026/11/04 23:15:25 perseant Exp $

#
# functions exported by freetype's private brotli copy are renamed to have
# a "nbft_" prefix.
#

RENAME_FUNCS= \
	BrotliBuildCodeLengthsHuffmanTable \
	BrotliBuildHuffmanTable \
	BrotliBuildSimpleHuffmanTable \
	BrotliCalculateRingBufferSize \
	BrotliDecoderAttachDictionary \
	BrotliDecoderCreateInstance \
	BrotliDecoderDecompress \
	BrotliDecoderDecompressStream \
	BrotliDecoderDestroyInstance \
	BrotliDecoderEnsureStaticInit \
	BrotliDecoderErrorString \
	BrotliDecoderGetErrorCode \
	BrotliDecoderHasMoreOutput \
	BrotliDecoderHuffmanTreeGroupInit \
	BrotliDecoderIsFinished \
	BrotliDecoderIsUsed \
	BrotliDecoderSetMetadataCallbacks \
	BrotliDecoderSetParameter \
	BrotliDecoderStateCleanup \
	BrotliDecoderStateCleanupAfterMetablock \
	BrotliDecoderStateInit \
	BrotliDecoderStateMetablockBegin \
	BrotliDecoderTakeOutput \
	BrotliDecoderVersion \
	BrotliDefaultAllocFunc \
	BrotliDefaultFreeFunc \
	BrotliEnsureRingBuffer \
	BrotliGetDictionary \
	BrotliGetTransforms \
	BrotliInitBitReader \
	BrotliSafeReadBits32Slow \
	BrotliSetDictionaryData \
	BrotliSharedDictionaryAttach \
	BrotliSharedDictionaryCreateInstance \
	BrotliSharedDictionaryDestroyInstance \
	BrotliTransformDictionaryWord \
	BrotliWarmupBitReader \
	_kBrotliContextLookupTable \
	_kBrotliPrefixCodeRanges \
	kBrotliBitMask \
	kBrotliDictionary \
	kBrotliDictionaryData \
	kBrotliTransforms \

.for _f in ${RENAME_FUNCS}
CPPFLAGS+= -D${_f}=nbft_${_f}
.endfor
