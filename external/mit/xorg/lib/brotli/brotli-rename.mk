#	$NetBSD: brotli-rename.mk,v 1.1 2023/01/29 07:54:11 mrg Exp $

#
# functions exported by freetype's private brotli copy are renamed to have
# a "nbft_" prefix.
#

RENAME_FUNCS= \
	BrotliBuildCodeLengthsHuffmanTable \
	BrotliBuildHuffmanTable \
	BrotliBuildSimpleHuffmanTable \
	BrotliDecoderCreateInstance \
	BrotliDecoderDecompress \
	BrotliDecoderDecompressStream \
	BrotliDecoderDestroyInstance \
	BrotliDecoderErrorString \
	BrotliDecoderGetErrorCode \
	BrotliDecoderHasMoreOutput \
	BrotliDecoderHuffmanTreeGroupInit \
	BrotliDecoderIsFinished \
	BrotliDecoderIsUsed \
	BrotliDecoderSetParameter \
	BrotliDecoderStateCleanup \
	BrotliDecoderStateCleanupAfterMetablock \
	BrotliDecoderStateInit \
	BrotliDecoderStateMetablockBegin \
	BrotliDecoderTakeOutput \
	BrotliDecoderVersion \
	BrotliDefaultAllocFunc \
	BrotliDefaultFreeFunc \
	BrotliGetDictionary \
	BrotliGetTransforms \
	BrotliInitBitReader \
	BrotliSafeReadBits32Slow \
	BrotliSetDictionaryData \
	BrotliTransformDictionaryWord \
	BrotliWarmupBitReader \

.for _f in ${RENAME_FUNCS}
CPPFLAGS+= -D${_f}=nbft_${_f}
.endfor
