#include <machine/asm.h>
.text
.balign 16
.globl rv32i_zkne_encrypt
.type   rv32i_zkne_encrypt,@function
rv32i_zkne_encrypt:
    addi    sp,sp,-16
    sw      x8,8(sp)
    sw      x9,0(sp)
    # Load input to block cipher
    lw      x6,0(x10)
    lw      x7,4(x10)
    lw      x8,8(x10)
    lw      x9,12(x10)

    # Load key
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

    # Load number of rounds
    lw      x30,240(x12)

    # initial transformation
    xor     x6,x6,x13
    xor     x7,x7,x14
    xor     x8,x8,x15
    xor     x9,x9,x16

    # The main loop only executes the first N-2 rounds, each loop consumes two rounds
    add     x30,x30,-2
    srli    x30,x30,1
1:
    # Grab next key in schedule
    add     x12,x12,16
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

        .word 644253363
    .word 1719043763
    .word 2793834163
    .word 3868624563

        .word 645334835
    .word 1720125235
    .word 2794915635
    .word 3865511731

        .word 646416307
    .word 1721206707
    .word 2791802803
    .word 3866593203

        .word 647497779
    .word 1718093875
    .word 2792884275
    .word 3867674675

    # now T0~T3 hold the new state

    # Grab next key in schedule
    add     x12,x12,16
    lw      x6,0(x12)
    lw      x7,4(x12)
    lw      x8,8(x12)
    lw      x9,12(x12)

        .word 651363123
    .word 1726153523
    .word 2800943923
    .word 3875734323

        .word 652444595
    .word 1727234995
    .word 2802025395
    .word 3872621491

        .word 653526067
    .word 1728316467
    .word 2798912563
    .word 3873702963

        .word 654607539
    .word 1725203635
    .word 2799994035
    .word 3874784435

    # now Q0~Q3 hold the new state

    add     x30,x30,-1
    bgtz    x30,1b

# final two rounds
    # Grab next key in schedule
    add     x12,x12,16
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

        .word 644253363
    .word 1719043763
    .word 2793834163
    .word 3868624563

        .word 645334835
    .word 1720125235
    .word 2794915635
    .word 3865511731

        .word 646416307
    .word 1721206707
    .word 2791802803
    .word 3866593203

        .word 647497779
    .word 1718093875
    .word 2792884275
    .word 3867674675

    # now T0~T3 hold the new state

    # Grab next key in schedule
    add     x12,x12,16
    lw      x6,0(x12)
    lw      x7,4(x12)
    lw      x8,8(x12)
    lw      x9,12(x12)

    # no mix column now
        .word 584254259
    .word 1659044659
    .word 2733835059
    .word 3808625459

        .word 585335731
    .word 1660126131
    .word 2734916531
    .word 3805512627

        .word 586417203
    .word 1661207603
    .word 2731803699
    .word 3806594099

        .word 587498675
    .word 1658094771
    .word 2732885171
    .word 3807675571

    # now Q0~Q3 hold the new state

    sw      x6,0(x11)
    sw      x7,4(x11)
    sw      x8,8(x11)
    sw      x9,12(x11)

    # Pop registers and return
    lw      x8,8(sp)
    lw      x9,0(sp)
    addi    sp,sp,16
    ret
.text
.balign 16
.globl rv32i_zknd_decrypt
.type   rv32i_zknd_decrypt,@function
rv32i_zknd_decrypt:
    addi    sp,sp,-16
    sw      x8,8(sp)
    sw      x9,0(sp)
    # Load input to block cipher
    lw      x6,0(x10)
    lw      x7,4(x10)
    lw      x8,8(x10)
    lw      x9,12(x10)

    # Load number of rounds
    lw      x30,240(x12)

    # Load the last key
    # use T0 as temporary now
    slli    x13,x30,4
    add     x12,x12,x13
    # Load key
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

    # initial transformation
    xor     x6,x6,x13
    xor     x7,x7,x14
    xor     x8,x8,x15
    xor     x9,x9,x16

    # The main loop only executes the first N-2 rounds, each loop consumes two rounds
    add     x30,x30,-2
    srli    x30,x30,1
1:
    # Grab next key in schedule
    add     x12,x12,-16
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

        .word 778471091
    .word 1855358643
    .word 2928051891
    .word 4000745139

        .word 779552563
    .word 1852245811
    .word 2929133363
    .word 4001826611

        .word 780634035
    .word 1853327283
    .word 2926020531
    .word 4002908083

        .word 781715507
    .word 1854408755
    .word 2927102003
    .word 3999795251

    # now T0~T3 hold the new state

    # Grab next key in schedule
    add     x12,x12,-16
    lw      x6,0(x12)
    lw      x7,4(x12)
    lw      x8,8(x12)
    lw      x9,12(x12)

        .word 785580851
    .word 1862468403
    .word 2935161651
    .word 4007854899

        .word 786662323
    .word 1859355571
    .word 2936243123
    .word 4008936371

        .word 787743795
    .word 1860437043
    .word 2933130291
    .word 4010017843

        .word 788825267
    .word 1861518515
    .word 2934211763
    .word 4006905011

    # now Q0~Q3 hold the new state

    add     x30,x30,-1
    bgtz    x30,1b

# final two rounds
    # Grab next key in schedule
    add     x12,x12,-16
    lw      x13,0(x12)
    lw      x14,4(x12)
    lw      x15,8(x12)
    lw      x16,12(x12)

        .word 778471091
    .word 1855358643
    .word 2928051891
    .word 4000745139

        .word 779552563
    .word 1852245811
    .word 2929133363
    .word 4001826611

        .word 780634035
    .word 1853327283
    .word 2926020531
    .word 4002908083

        .word 781715507
    .word 1854408755
    .word 2927102003
    .word 3999795251

    # now T0~T3 hold the new state

    # Grab next key in schedule
    add     x12,x12,-16
    lw      x6,0(x12)
    lw      x7,4(x12)
    lw      x8,8(x12)
    lw      x9,12(x12)

    # no mix column now
        .word 718471987
    .word 1795359539
    .word 2868052787
    .word 3940746035

        .word 719553459
    .word 1792246707
    .word 2869134259
    .word 3941827507

        .word 720634931
    .word 1793328179
    .word 2866021427
    .word 3942908979

        .word 721716403
    .word 1794409651
    .word 2867102899
    .word 3939796147

    # now Q0~Q3 hold the new state

    sw      x6,0(x11)
    sw      x7,4(x11)
    sw      x8,8(x11)
    sw      x9,12(x11)

    # Pop registers and return
    lw      x8,8(sp)
    lw      x9,0(sp)
    addi    sp,sp,16
    ret
.text
.balign 16
.globl rv32i_zkne_set_encrypt_key
.type rv32i_zkne_set_encrypt_key,@function
rv32i_zkne_set_encrypt_key:
    addi    sp,sp,-0
    bnez    x10,1f        # if (!userKey || !key) return -1;
    bnez    x12,1f
    li      a0,-1
    ret
1:
    # Determine number of rounds from key size in bits
    li      x13,128
    bne     x11,x13,1f
    li      x14,10          # key->rounds = 10 if bits == 128
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,1
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,2
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,4
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,8
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,16
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,32
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,64
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,128
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,27
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,54
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
1:
    li      x13,192
    bne     x11,x13,2f
    li      x14,12          # key->rounds = 12 if bits == 192
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,1
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,2
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,4
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,8
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,16
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,32
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,64
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,128
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
2:
    li      x14,14          # key->rounds = 14 if bits == 256
    li      x13,256
    beq     x11,x13,3f
    li      a0,-2           # If bits != 128, 192, or 256, return -2
    j       5f
3:
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)
    lw      x29,24(x10)
    lw      x30,28(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,1
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,2
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,4
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,8
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,16
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,32
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,64
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

4:  # return 0
    li      a0,0
5:  # return a0
    addi    sp,sp,0
    ret
.text
.balign 16
.globl rv32i_zbkb_zkne_set_encrypt_key
.type rv32i_zbkb_zkne_set_encrypt_key,@function
rv32i_zbkb_zkne_set_encrypt_key:
    addi    sp,sp,-0
    bnez    x10,1f        # if (!userKey || !key) return -1;
    bnez    x12,1f
    li      a0,-1
    ret
1:
    # Determine number of rounds from key size in bits
    li      x13,128
    bne     x11,x13,1f
    li      x14,10          # key->rounds = 10 if bits == 128
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,1
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,2
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,4
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,8
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,16
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,32
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,64
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,128
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,27
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # use T4 to store rcon
    li      x17,54
    # as xor is associative and commutative
    # we fist xor T0 with RCON, then use T0 to
    # xor the result of each SBOX result of T3
    xor     x13,x13,x17
    # use T4 to store rotated T3
    .word 1619548307
    # update T0
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307


    # update new T1~T3
    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
1:
    li      x13,192
    bne     x11,x13,2f
    li      x14,12          # key->rounds = 12 if bits == 192
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,1
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,2
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,4
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,8
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,16
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,32
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,64
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    xor     x17,x17,x16
    xor     x28,x28,x17
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    # see the comment in ke128enc
    li      x29,128
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,24
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
2:
    li      x14,14          # key->rounds = 14 if bits == 256
    li      x13,256
    beq     x11,x13,3f
    li      a0,-2           # If bits != 128, 192, or 256, return -2
    j       5f
3:
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)
    lw      x29,24(x10)
    lw      x30,28(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,1
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,2
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,4
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,8
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,16
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,32
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29
    sw      x17,16(x12)
    sw      x28,20(x12)
    sw      x29,24(x12)
    sw      x30,28(x12)
    # see the comment in ke128enc
    li      x31,64
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

4:  # return 0
    li      a0,0
5:  # return a0
    addi    sp,sp,0
    ret
.text
.balign 16
.globl rv32i_zknd_zkne_set_decrypt_key
.type   rv32i_zknd_zkne_set_decrypt_key,@function
rv32i_zknd_zkne_set_decrypt_key:
    addi    sp,sp,-0
    bnez    x10,1f        # if (!userKey || !key) return -1;
    bnez    x12,1f
    li      a0,-1
    ret
1:
    # Determine number of rounds from key size in bits
    li      x13,128
    bne     x11,x13,1f
    li      x14,10          # key->rounds = 10 if bits == 128
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see comments in ke128enc
    li      x17,1
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,2
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,4
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,8
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,16
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,32
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,64
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,128
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,27
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,54
    xor     x13,x13,x17
    srli    x17,x16,8
    slli    x28,x16,24
    or      x17,x17,x28
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
1:
    li      x13,192
    bne     x11,x13,2f
    li      x14,12          # key->rounds = 12 if bits == 192
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,1
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,2
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,4
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,8
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,16
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,32
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,64
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,128
    xor     x13,x13,x29
    srli    x29,x28,8
    slli    x30,x28,24
    or      x29,x29,x30
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    # the reason for dropping T4/T5 is in ke192enc
    # the reason for not invm4 is in ke128dec
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
2:
    li      x14,14          # key->rounds = 14 if bits == 256
    li      x13,256
    beq     x11,x13,3f
    li      a0,-2           # If bits != 128, 192, or 256, return -2
    j       5f
3:
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)
    lw      x29,24(x10)
    lw      x30,28(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see the comment in ke128dec
    # BITS and T8 are temp variables
    # BITS are not used anymore
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,1
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,2
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,4
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,8
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,16
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,32
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,64
    xor     x13,x13,x31
    srli    x31,x30,8
    slli    x11,x30,24
    or      x31,x31,x11
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # last 16 bytes are dropped
    # see the comment in ke256enc

4:  # return 0
    li      a0,0
5:  # return a0
    addi    sp,sp,0
    ret
.text
.balign 16
.globl rv32i_zbkb_zknd_zkne_set_decrypt_key
.type rv32i_zbkb_zknd_zkne_set_decrypt_key,@function
rv32i_zbkb_zknd_zkne_set_decrypt_key:
    addi    sp,sp,-0
    bnez    x10,1f        # if (!userKey || !key) return -1;
    bnez    x12,1f
    li      a0,-1
    ret
1:
    # Determine number of rounds from key size in bits
    li      x13,128
    bne     x11,x13,1f
    li      x14,10          # key->rounds = 10 if bits == 128
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see comments in ke128enc
    li      x17,1
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,2
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,4
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,8
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,16
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,32
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,64
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,128
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,27
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    # T4 and T5 are temp variables
        li      x17,0
    li      x28,0
        .word 584616115
    .word 1658357939
    .word 2732099763
    .word 3805841587

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,0(x12)
        li      x17,0
    li      x28,0
        .word 585664691
    .word 1659406515
    .word 2733148339
    .word 3806890163

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,4(x12)
        li      x17,0
    li      x28,0
        .word 586713267
    .word 1660455091
    .word 2734196915
    .word 3807938739

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,8(x12)
        li      x17,0
    li      x28,0
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

        .word 790498867
    .word 1864240691
    .word 2937982515
    .word 4011724339


    sw      x28,12(x12)
    # see comments in ke128enc
    li      x17,54
    xor     x13,x13,x17
    .word 1619548307
        .word 588678835
    .word 1662420659
    .word 2736162483
    .word 3809904307

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15
    add     x12,x12,16
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
1:
    li      x13,192
    bne     x11,x13,2f
    li      x14,12          # key->rounds = 12 if bits == 192
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,1
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,2
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,4
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,8
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,16
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,32
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,64
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    xor     x17,x17,x16
    xor     x28,x28,x17

    # see the comment in ke128dec
    # T7 and T6 are temp variables
        li      x29,0
    li      x30,0
        .word 585010867
    .word 1658752691
    .word 2732494515
    .word 3806236339

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,0(x12)
        li      x29,0
    li      x30,0
        .word 586059443
    .word 1659801267
    .word 2733543091
    .word 3807284915

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,4(x12)
        li      x29,0
    li      x30,0
        .word 587108019
    .word 1660849843
    .word 2734591667
    .word 3808333491

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,8(x12)
        li      x29,0
    li      x30,0
        .word 588156595
    .word 1661898419
    .word 2735640243
    .word 3809382067

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,12(x12)
        li      x29,0
    li      x30,0
        .word 589205171
    .word 1662946995
    .word 2736688819
    .word 3810430643

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,16(x12)
        li      x29,0
    li      x30,0
        .word 600739507
    .word 1674481331
    .word 2748223155
    .word 3821964979

        .word 803147571
    .word 1876889395
    .word 2950631219
    .word 4024373043


    sw      x30,20(x12)
    # see the comment in ke128enc
    li      x29,128
    xor     x13,x13,x29
    .word 1619943059
        .word 601261747
    .word 1675003571
    .word 2748745395
    .word 3822487219

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,24
    # the reason for dropping T4/T5 is in ke192enc
    # the reason for not invm4 is in ke128dec
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)

    j       4f
2:
    li      x14,14          # key->rounds = 14 if bits == 256
    li      x13,256
    beq     x11,x13,3f
    li      a0,-2           # If bits != 128, 192, or 256, return -2
    j       5f
3:
    sw      x14,240(x12)  # store key->rounds
    lw      x13,0(x10)
    lw      x14,4(x10)
    lw      x15,8(x10)
    lw      x16,12(x10)
    lw      x17,16(x10)
    lw      x28,20(x10)
    lw      x29,24(x10)
    lw      x30,28(x10)

    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # see the comment in ke128dec
    # BITS and T8 are temp variables
    # BITS are not used anymore
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,1
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,2
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,4
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,8
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,16
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,32
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    # for aes256, T3->T4 needs 4sbox but no rotate/rcon
        .word 587761843
    .word 1661503667
    .word 2735245491
    .word 3808987315

    xor     x28,x28,x17
    xor     x29,x29,x28
    xor     x30,x30,x29

    # see the comment in ke128dec
    # T8 and BITS are temp variables
        li      x11,0
    li      x31,0
        .word 584418739
    .word 1658160563
    .word 2731902387
    .word 3805644211

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,0(x12)
        li      x11,0
    li      x31,0
        .word 585467315
    .word 1659209139
    .word 2732950963
    .word 3806692787

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,4(x12)
        li      x11,0
    li      x31,0
        .word 586515891
    .word 1660257715
    .word 2733999539
    .word 3807741363

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,8(x12)
        li      x11,0
    li      x31,0
        .word 587564467
    .word 1661306291
    .word 2735048115
    .word 3808789939

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,12(x12)
        li      x11,0
    li      x31,0
        .word 588613043
    .word 1662354867
    .word 2736096691
    .word 3809838515

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,16(x12)
        li      x11,0
    li      x31,0
        .word 600147379
    .word 1673889203
    .word 2747631027
    .word 3821372851

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,20(x12)
        li      x11,0
    li      x31,0
        .word 601195955
    .word 1674937779
    .word 2748679603
    .word 3822421427

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,24(x12)
        li      x11,0
    li      x31,0
        .word 602244531
    .word 1675986355
    .word 2749728179
    .word 3823470003

        .word 784306099
    .word 1858047923
    .word 2931789747
    .word 4005531571


    sw      x31,28(x12)
    # see the comment in ke128enc
    li      x31,64
    xor     x13,x13,x31
    .word 1620008851
        .word 603358899
    .word 1677100723
    .word 2750842547
    .word 3824584371

    xor     x14,x14,x13
    xor     x15,x15,x14
    xor     x16,x16,x15

    add     x12,x12,32
    sw      x13,0(x12)
    sw      x14,4(x12)
    sw      x15,8(x12)
    sw      x16,12(x12)
    # last 16 bytes are dropped
    # see the comment in ke256enc

4:  # return 0
    li      a0,0
5:  # return a0
    addi    sp,sp,0
    ret
