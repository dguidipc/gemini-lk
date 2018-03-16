#include <iostream>
#include <fstream>
#include <vector>

#include "bmpdecoderhelper.h"

using namespace std;
using namespace image_codec;

// -------------------------------------------------------------------------------------

class BmpToLogo : public BmpDecoderCallback
{
public:
    BmpToLogo() : BmpDecoderCallback(), m_width(0), m_height(0) {}

    bool DoBmpToLogo(const char *bmpFilename, const char *logoFilename, bool append)
    {
        // Open input BMP file
        ifstream bmpFile;
        bmpFile.open(bmpFilename, ios::in | ios::binary);
        if (!bmpFile.is_open()) {
            cout << "open " << bmpFilename << " failed!" << endl;
            return false;
        }

        // Open output LOGO file
        ofstream logoFile;
        ios_base::openmode outMode = (ofstream::out | ofstream::binary);
        if (append) outMode |= ofstream::app;
        logoFile.open(logoFilename, outMode);
        if (!logoFile.is_open()) {
            cout << "open " << logoFile << " failed!" << endl;
            return false;
        }

        // Read in BMP file content
        int bmpFileSize = GetFileSize(bmpFile);
        m_bitstream.resize(bmpFileSize);
        bmpFile.read(&m_bitstream[0], m_bitstream.size());

        // Decode BMP file to destination buffer
        BmpDecoderHelper bmpDec;
        bmpDec.DecodeImage(&m_bitstream[0], m_bitstream.size(), INT_MAX, this);

        // ConvertToRGB565();
        // ConvertToRGB565Dither();
        // Convert RGB888 buffer to RGB565
        ConvertToARGB8888();

        // Write to LOGO file
        // logoFile.write(&m_rgb565Buffer[0], m_rgb565Buffer.size());
        logoFile.write(&m_argb8888Buffer[0], m_argb8888Buffer.size());

        // Close Files
        bmpFile.close();
        logoFile.close();

        return true;
    }

    virtual uint8* SetSize(int width, int height)
    {
        m_width  = (uint32)width;
        m_height = (uint32)height;
        m_rgb888Buffer.resize(width * height * 3);
        m_rgb565Buffer.resize(width * height * 2);
        m_argb8888Buffer.resize(width * height * 4);

        return (uint8 *) &m_argb8888Buffer[0];
    }

private:
    int GetFileSize(ifstream& file)
    {
        ios::pos_type backup = file.tellg();
        file.seekg(0, std::ios::end);
        ios::pos_type size = file.tellg();
        file.seekg(backup, std::ios::beg);
        return (int)size;
    }

    void ConvertToRGB565()
    {
        uint8  *rgb888 = (uint8*)  &m_rgb888Buffer[0];
        uint16 *rgb565 = (uint16*) &m_rgb565Buffer[0];
        uint32 R, G, B;

        for(uint32 i = 0; i < m_width * m_height; ++ i) {
            R = rgb888[0]; G = rgb888[1]; B = rgb888[2];
            *rgb565 = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | ((B & 0xF8) >> 3);
            ++ rgb565;
            rgb888 += 3;
        }
    }
    
    void ConvertToARGB8888()
    {
        uint8  *rgb888   = (uint8*)  &m_rgb888Buffer[0];
        uint32 *argb8888 = (uint32*) &m_argb8888Buffer[0];
        uint32 R, G, B;

        for(uint32 i = 0; i < m_width * m_height; ++ i) {
            R = rgb888[0]; G = rgb888[1]; B = rgb888[2];
            *argb8888 = (0xFF << 24) | (R << 16) | (G << 8) | (B);
            ++argb8888;
            rgb888 += 3;
        }
    }

    #define CLIP_255(x) ((x)>255?255:(x))

    void ConvertToRGB565Dither(void) 
    {
        unsigned short DitherMatrix_3Bit_16[4] = {0x5140, 0x3726, 0x4051, 0x2637};
        unsigned int count = 0;
        unsigned int sr,sg,sb;
        unsigned int x, y;
        unsigned short dither_scan = 0;
        uint8  *src= (uint8*)  &m_rgb888Buffer[0];
        uint16 *dst= (uint16*) &m_rgb565Buffer[0];

        for(count = 0;count < m_width*m_height;count++)	
        {
            x = count % m_width;
            y = count / m_width;
            dither_scan = DitherMatrix_3Bit_16[(y) & 3];

            unsigned short dither = ((dither_scan >> (((x) & 3) << 2)) & 0xF);
            sr = ((CLIP_255(((*(src+0) + dither - (*(src+0) >> 5)))) >> (8 - 5)));
            sg = ((CLIP_255(((*(src+1) + (dither >> 1) - (*(src+1) >> 6)))) >> (8 - 6)));
            sb = ((CLIP_255(((*(src+2) + dither - (*(src+2) >> 5)))) >> (8 - 5)));
            printf("%3d,%3d,%3d|%3d,%3d,%3d|%3d,%3d,%3d|%d,%d|%d,%d\n",
                  (sr<<3)-*(src+0), (sg<<2)-*(src+1),(sb<<3)-*(src+2),
                   sr<<3,sg<<2,sb<<3,
                   *(src+0), *(src+1),*(src+2),
                   x,y,
                   dither_scan, dither);
            src += 3;

            *dst++ = ((uint16_t)((sr << (5 + 6)) | (sg << (5)) | (sb << 0)));
    	}
    }


private:
    vector<char> m_bitstream;
    vector<char> m_rgb888Buffer;
    vector<char> m_argb8888Buffer;
    vector<char> m_rgb565Buffer;
    uint32 m_width, m_height;
};

// -------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc < 3) {
        cout << endl << "[Usage] bmp_to_logo logofile bmpfile1 [bmpfile2] ..." << endl << endl;
        cout << "Example: bmp_to_logo fhd.raw fhd_uboot.bmp fhd_kernel.bmp ..." << endl << endl;
        return -1;
    }

    const char *logo_filename = argv[1];
    const char *bmp_filename  = argv[2];

    BmpToLogo bmpToLogo;
    if (!bmpToLogo.DoBmpToLogo(bmp_filename, logo_filename, false)) {
        return -2;
    }

    for (int i = 3; i < argc; ++ i) {
        bmp_filename = argv[i];
        if (!bmpToLogo.DoBmpToLogo(bmp_filename, logo_filename, true)) {
            return -2;
        }
    }
}

// -------------------------------------------------------------------------------------
