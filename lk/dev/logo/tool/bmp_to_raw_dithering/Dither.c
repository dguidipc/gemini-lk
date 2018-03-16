const uint16_t gDitherMatrix_3Bit_16[4] = {
    0x5140, 0x3726, 0x4051, 0x2637
};


static inline uint16_t SkPackRGB16(unsigned r, unsigned g, unsigned b) {
 

    return ((uint16_t)((r << (5 + 6)) | (g << (5)) | (b << 0)));
}

static inline uint16_t SkDitherRGB32To565(SkPMColor c, unsigned dither)
{
    

    unsigned sr = ((uint32_t)((c) << (24 - 0)) >> 24);
    unsigned sg = ((uint32_t)((c) << (24 - 8)) >> 24);
    unsigned sb = ((uint32_t)((c) << (24 - 16)) >> 24);
    sr = ((unsigned)((sr + dither - (sr >> 5))) >> (8 - 5));
    sg = ((unsigned)((sg + (dither >> 1) - (sg >> 6))) >> (8 - 6));
    sb = ((unsigned)((sb + dither - (sb >> 5))) >> (8 - 5));

    return SkPackRGB16(sr, sg, sb);
}

static void S32_D565_Opaque_Dither(uint16_t* __restrict__ dst,
                                     const SkPMColor* __restrict__ src,
                                     int count, U8CPU alpha, int x, int y) {
   

    if (count > 0) {
        const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
        do {
            SkPMColor c = *src++;
      

            unsigned dither = ((dither_scan >> (((x) & 3) << 2)) & 0xF);
            *dst++ = SkDitherRGB32To565(c, dither);
            ++(x);
        } while (--count != 0);
    }
}

