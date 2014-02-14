// RUN: %clang_cc1 -triple x86_64-apple-macos10.7.0 -fsyntax-only -verify -fno-lax-vector-conversions %s

typedef __attribute__(( ext_vector_type(2) )) float float2;
typedef __attribute__(( ext_vector_type(3) )) float float3;
typedef __attribute__(( ext_vector_type(4) )) int int4;
typedef __attribute__(( ext_vector_type(8) )) short short8;
typedef __attribute__(( ext_vector_type(4) )) float float4;
typedef float t3 __attribute__ ((vector_size (16)));
typedef __typeof__(sizeof(int)) size_t;
typedef unsigned long ulong2 __attribute__ ((ext_vector_type(2)));
typedef size_t stride4 __attribute__((ext_vector_type(4)));

static void test() {
    float2 vec2;
    float3 vec3;
    float4 vec4, vec4_2;
    int4 ivec4;
    short8 ish8;
    t3 vec4_3;
    int *ptr;
    int i;

    vec3 += vec2; // expected-error {{can't convert between vector values of different size}}
    vec4 += vec3; // expected-error {{can't convert between vector values of different size}}
    
    vec4 = 5.0f;
    vec4 = (float4)5.0f;
    vec4 = (float4)5;
    vec4 = (float4)vec4_3;
    
    ivec4 = (int4)5.0f;
    ivec4 = (int4)5;
    ivec4 = (int4)vec4_3;
    
    i = (int)ivec4; // expected-error {{invalid conversion between vector type 'int4' and integer type 'int' of different size}}
    i = ivec4; // expected-error {{assigning to 'int' from incompatible type 'int4'}}
    
    ivec4 = (int4)ptr; // expected-error {{invalid conversion between vector type 'int4' and scalar type 'int *'}}
    
    vec4 = (float4)vec2; // expected-error {{invalid conversion between ext-vector type 'float4' and 'float2'}}
    
    ish8 += 5; // expected-error {{can't convert between vector values of different size ('short8' and 'int')}}
    ish8 += (short)5;
    ivec4 *= 5;
     vec4 /= 5.2f;
     vec4 %= 4; // expected-error {{invalid operands to binary expression ('float4' and 'int')}}
    ivec4 %= 4;
    ivec4 += vec4; // expected-error {{can't convert between vector values of different size ('int4' and 'float4')}}
    ivec4 += (int4)vec4;
    ivec4 -= ivec4;
    ivec4 |= ivec4;
    ivec4 += ptr; // expected-error {{can't convert between vector and non-scalar values ('int4' and 'int *')}}
}

typedef __attribute__(( ext_vector_type(2) )) float2 vecfloat2; // expected-error{{invalid vector element type 'float2'}}

void inc(float2 f2) {
  f2++; // expected-error{{cannot increment value of type 'float2'}}
  __real f2; // expected-error{{invalid type 'float2' to __real operator}}
}

typedef enum
{
    uchar_stride = 1,
    uchar4_stride = 4,
    ushort4_stride = 8,
    short4_stride = 8,
    uint4_stride = 16,
    int4_stride = 16,
    float4_stride = 16,
} PixelByteStride;

stride4 RDar15091442_get_stride4(int4 x, PixelByteStride pixelByteStride);
stride4 RDar15091442_get_stride4(int4 x, PixelByteStride pixelByteStride)
{
    stride4 stride;
    // This previously caused an assertion failure.
    stride.lo = ((ulong2) x) * pixelByteStride; // no-warning
    return stride;
}

