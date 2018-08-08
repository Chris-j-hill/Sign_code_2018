#ifndef LUTS_H
#define LUTS_H



const byte parity[] =//LUT for parity check, byte value is index, content is 0 if even parity, 1 if odd
{
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0,
  1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1,
  1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
  0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
  1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
  0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0,
  0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1,
  0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1,
  0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0,
  0, 1, 0, 1, 1, 0
};


const uint32_t hamming_G_matrix[] = {0x52000000,    //matlab generation code >> [h, g, n, k] = hammgen(5);
                                     0x29000000,
                                     0x14800000,
                                     0x58400000,
                                     0x2C200000,
                                     0x44100000,
                                     0x70080000,
                                     0x38040000,
                                     0x1C020000,
                                     0x5C010000,
                                     0x7C008000,
                                     0x6C004000,
                                     0x64002000,
                                     0x60001000,
                                     0x30000800,
                                     0x18000400,
                                     0x0C000200,
                                     0x54000100,
                                     0x78000080,
                                     0x3C000040,
                                     0x4C000020,
                                     0x74000010,
                                     0x68000008,
                                     0x34000004,
                                     0x48000002,
                                     0x24000001
                                    };


                                    
#endif