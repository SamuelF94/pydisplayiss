#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "font.h"

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8
#define R_VALUE 0
#define G_VALUE 0
#define B_VALUE 255

#define BYTE	uint8_t
#define WORD	uint16_t
#define DWORD	uint32_t

#define LOBYTE(w)           ((BYTE)(((DWORD)(w)) & 0xff))


#define RGB(r,g,b)      ((uint32_t)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GETRVALUE(rgb)      (LOBYTE(rgb))
#define GETGVALUE(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))
#define GETBVALUE(rgb)      (LOBYTE((rgb)>>16))



#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


typedef struct tagRGB24
{
	uint8_t blue;
	uint8_t green;
	uint8_t red;
} RGB24;


typedef struct tagRGB565
{
	uint16_t b:5;
	uint16_t g:6;
	uint16_t r:5;
} RGB565;



/*--------------------------------------------------------------------------}
{                        BITMAP FILE HEADER DEFINITION                      }
{--------------------------------------------------------------------------*/
#pragma pack(1)									// Must be packed as read from file
typedef struct tagBITMAPFILEHEADER {
	uint16_t  bfType; 							// Bitmap type should be "BM"
	uint32_t  bfSize; 							// Bitmap size in bytes
	uint16_t  bfReserved1; 						// reserved short1
	uint16_t  bfReserved2; 						// reserved short2
	uint32_t  bfOffBits; 						// Offset to bmp data
} BITMAPFILEHEADER, * LPBITMAPFILEHEADER, * PBITMAPFILEHEADER;
#pragma pack()

/*--------------------------------------------------------------------------}
{                    BITMAP FILE INFO HEADER DEFINITION						}
{--------------------------------------------------------------------------*/
#pragma pack(1)									// Must be packed as read from file
typedef struct tagBITMAPINFOHEADER {
	uint32_t biSize; 							// Bitmap file size
	uint32_t biWidth; 							// Bitmap width
	uint32_t biHeight;							// Bitmap height
	uint16_t biPlanes; 							// Planes in image
	uint16_t biBitCount; 						// Bits per byte
	uint32_t biCompression; 					// Compression technology
	uint32_t biSizeImage; 						// Image byte size
	uint32_t biXPelsPerMeter; 					// Pixels per x meter
	uint32_t biYPelsPerMeter; 					// Pixels per y meter
	uint32_t biClrUsed; 						// Number of color indexes needed
	uint32_t biClrImportant; 					// Min colours needed
} BITMAPINFOHEADER, * PBITMAPINFOHEADER;
#pragma pack()
/*--------------------------------------------------------------------------}
{				  BITMAP VER 4 FILE INFO HEADER DEFINITION					}
{--------------------------------------------------------------------------*/
#pragma pack(1)
typedef struct tagWIN4XBITMAPINFOHEADER
{
	uint32_t RedMask;       /* Mask identifying bits of red component */
	uint32_t GreenMask;     /* Mask identifying bits of green component */
	uint32_t BlueMask;      /* Mask identifying bits of blue component */
	uint32_t AlphaMask;     /* Mask identifying bits of alpha component */
	uint32_t CSType;        /* Color space type */
	uint32_t RedX;          /* X coordinate of red endpoint */
	uint32_t RedY;          /* Y coordinate of red endpoint */
	uint32_t RedZ;          /* Z coordinate of red endpoint */
	uint32_t GreenX;        /* X coordinate of green endpoint */
	uint32_t GreenY;        /* Y coordinate of green endpoint */
	uint32_t GreenZ;        /* Z coordinate of green endpoint */
	uint32_t BlueX;         /* X coordinate of blue endpoint */
	uint32_t BlueY;         /* Y coordinate of blue endpoint */
	uint32_t BlueZ;         /* Z coordinate of blue endpoint */
	uint32_t GammaRed;      /* Gamma red coordinate scale value */
	uint32_t GammaGreen;    /* Gamma green coordinate scale value */
	uint32_t GammaBlue;     /* Gamma blue coordinate scale value */
} WIN4XBITMAPINFOHEADER;

#pragma pack()

#define bit_set(val, bit_no) (((val) >> (bit_no)) & 1)

// 'global' variables to store screen info
uint8_t* fbp = 0;
int fbfd = 0;
struct fb_var_screeninfo vinfo = { 0 };
struct fb_fix_screeninfo finfo = { 0 };



uint16_t RGB16(uint8_t r, uint8_t g, uint8_t b )
{
    uint16_t c = b>>3; // b
    c |= ((g>>2)<<5); // g
    c |= ((r>>3)<<11); // r
	return c;
}



/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}


void put_pixel_RGB565(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{

    // calculate the pixel's byte offset inside the buffer
    // note: x * 2 as every pixel is 2 consecutive bytes
	// 16 bits par pixel
    unsigned int pix_offset = x * 2 + y*vinfo.xres*2;

    // now this is about the same as 'fbp[pix_offset] = value'
    // but a bit more complicated for RGB565
    uint16_t c = b>>3; // b
    c |= ((g>>2)<<5); // g
    c |= ((r>>3)<<11); // r

	//   uint16_t c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
    // or: c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
    // write 'two bytes at once'
	/*	printf("pix offset:%d , color %d\n",pix_offset, c ); */
    *((unsigned short*)(fbp + pix_offset)) = c;

}


void DrawChar(char c, uint16_t x, uint16_t y,int8_t fontsize, uint32_t color, uint8_t erase)
{
    uint8_t i,j;
	int8_t fontsize_x, fontsize_y;
	
	if ( fontsize > 0 )
	{
		fontsize_x = fontsize_y = fontsize;
	}	
	else
	{
		fontsize_x = 1;
		fontsize_y = 2;
	}


    // Convert the character to an index
    c = c & 0x7F;

    // 'font' is a multidimensional array of [128][char_width]
    // which is really just a 1D array of size 128*char_width.
    const uint8_t* chr = font8x8_basic[(uint8_t)c];
    uint8_t r,g,b;
	r = GETRVALUE(color);
	g = GETGVALUE(color);
	b = GETBVALUE(color);
    // Draw pixels
    for (j=0; j<CHAR_WIDTH; j++)
    {
        uint8_t val = chr[j];
        uint8_t isize, jsize;
        for (i=0; i<CHAR_HEIGHT; i++)
        {
            if (bit_set(val,i))
            {
	            for ( jsize = 0 ; jsize < fontsize_y; jsize++)
    	        {
        	        for ( isize = 0 ; isize < fontsize_x; isize++)
            	        put_pixel_RGB565(x+i*fontsize_x+jsize, y+j*fontsize_y+isize, r,g,b);
	            }
			}
			else if ( erase != 0 )
			{
	            for ( jsize = 0 ; jsize < fontsize_y; jsize++)
    	        {
        	        for ( isize = 0 ; isize < fontsize_x; isize++)
            	        put_pixel_RGB565(x+i*fontsize_x+jsize, y+j*fontsize_y+isize, 0,0,0);
	            }
			}
        }
    }
}


void DrawString(const char* str, uint16_t x, uint16_t y, int8_t fontsize, uint32_t color, uint8_t erase)
{

#ifdef DEBUG_LOG
     printf("fontsize:%d\n",fontsize);
#endif
    while (*str)
	{
    	DrawChar(*str++, x, y,fontsize, color,erase);
        x += CHAR_WIDTH*(fontsize > 0 ? fontsize : 1 );
    }
}




uint8_t star[8] =  { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00};
void DrawISS( int lon, int lat)
{
    int i,j;
    const uint8_t* chr = star;
	uint8_t fontsize = 2;
	char szPos[100];

	// longitude varies from -180 to 180, 
	int x = (lon*480)/360 + 240;
	int y = ((-1)*lat*320)/180 + 160;


	snprintf(szPos,sizeof(szPos)-1,"Lat:%d%s - Lon:%d%s",abs(lat),lat>0?"W":"E",abs(lon),lon>0?"N":"S");
//	DrawString(szPos,100,300,2,RGB(0,0,255),0);

    // Draw pixels
//	printf("%d=>%d - %d=>%d\n",lon, x,lat, y);
	// re-centering coordinates  as we display a 16x16 star
	if ( x > 8 )
		x-= 8;
	if ( y < 314 )
		y += 16;

	DrawChar('*', x, y,2,RGB(255,253,35),0);
	msleep(100);
	DrawChar('*', x, y,2,RGB(255,0,0),0);
	msleep(100);
	DrawChar('*', x, y,2,RGB(255,253,35),0);
	msleep(100);
	DrawChar('*', x, y,2,RGB(255,0,0),0);
	msleep(100);
	DrawChar('*', x, y,2,RGB(255,253,35),0);


}


void Draw_Bitmap (const char* BitmapName,  uint8_t* FrameBuffer)
{
  unsigned int fpos = 0;                      // File position
  if (BitmapName)                         // We have a valid name string
  {
    FILE* fb = fopen(BitmapName, "rb");             // Open the bitmap file
    if (fb > 0)                         // File opened successfully
    {
      size_t br;
      BITMAPFILEHEADER bmpHeader;
      br = fread(&bmpHeader, 1, sizeof(bmpHeader), fb);   // Read the bitmap header
      if (br == sizeof(bmpHeader) && bmpHeader.bfType == 0x4D42) // Check it is BMP file
      {
        BITMAPINFOHEADER bmpInfo;
        fpos = sizeof(bmpHeader);             // File position is at sizeof(bmpHeader)
        br = fread(&bmpInfo, 1, sizeof(bmpInfo), fb);   // Read the bitmap info header
        /** printf("biBitCount:%d, width:%d, heigth:%d\n",bmpInfo.biBitCount, bmpInfo.biWidth,bmpInfo.biHeight); **/

        if (br == sizeof(bmpInfo))              // Check bitmap info read worked
        {
          fpos += sizeof(bmpInfo);            // File position moved sizeof(bmpInfo)
          if (bmpInfo.biSize == 108)
          {
            WIN4XBITMAPINFOHEADER bmpInfo4x;
            br = fread(&bmpInfo4x, 1, sizeof(bmpInfo4x), fb); // Read the bitmap v4 info header exta data
            fpos += sizeof(bmpInfo4x);          // File position moved sizeof(bmpInfo4x)
          }
          if (bmpHeader.bfOffBits > fpos)
          {
            uint8_t b;
			int iMax = bmpHeader.bfOffBits - fpos;
            for (int i = 0; i < iMax; i++)
              fread(&b, 1, sizeof(b), fb);
            fpos = bmpHeader.bfOffBits;
          }
          /* file positioned at image we must transfer it to screen */
          int y = 0;
          short *destination = (short *)FrameBuffer;
		  int yMax = bmpInfo.biHeight*bmpInfo.biWidth;
          while (!feof(fb) && y < yMax) // While no error and not all line read
          {
            RGB24 theRgb;
            fread(&theRgb, 1, 3, fb);
            uint32_t coordY = vinfo.yres - y/vinfo.xres;
            uint32_t coordX = y%vinfo.xres;
            destination[coordX + (coordY-1)*480] = RGB16(theRgb.red, theRgb.green, theRgb.blue);
            y++;
          }
        }
      }
      else perror("Header open failed");
      fclose(fb);
    }
  }
}


int initFramebuffer()
{
	fbfd = open("/dev/fb1", O_RDWR);
	if (fbfd == -1) 
	{
		perror("Error: cannot open framebuffer device.\n");
		return(0);
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) 
	{
		perror("Error reading variable information.\n");
		close(fbfd);
		return(0);
	}
//	printf("bits per pixel:%d, width:%d, heigth:%d\n",vinfo.bits_per_pixel,vinfo.xres,vinfo.yres); 


	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		perror("Error reading fixed information.\n");
		close(fbfd);
		return(0);
	}
/**	printf("mem_size:%d\n",finfo.smem_len);  **/


	// map fb to user mem
	fbp = (uint8_t*)mmap(0,	finfo.smem_len,		PROT_READ | PROT_WRITE,		MAP_SHARED,		fbfd,		0);

	if ( fbp == (uint8_t*)-1 )
		return(0);

	return 1;
}


void freeFramebuffer()
{
	munmap(fbp, finfo.smem_len);
	// close fb fil
	close(fbfd);
}



static PyObject *pydisplay_showISS(PyObject *self, PyObject *args)
{
	int lat, lon;
	if (!PyArg_ParseTuple(args, "ii", &lon, &lat))
	{
		return NULL;
	}

	if ( initFramebuffer() == 0 )
		return NULL;

	DrawISS(lon, lat);

	freeFramebuffer();

    return PyLong_FromLong(1);
}


static PyObject *pydisplay_showBMP(PyObject *self, PyObject *args)
{
    const char *filepath;

    if (!PyArg_ParseTuple(args, "s", &filepath))
	{
		return NULL;
	}

	if ( initFramebuffer() == 0 )
		return NULL;

	Draw_Bitmap (filepath, fbp);

	freeFramebuffer();

    return PyLong_FromLong(1);
}



static PyObject *pydisplay_clearScreen(PyObject *self,  PyObject *args)
{
	if ( initFramebuffer() == 0 )
		return NULL;

	memset(fbp,0,finfo.smem_len);

	freeFramebuffer();

	Py_RETURN_TRUE;
}


static PyObject *pydisplay_clearScreenBottom(PyObject *self,  PyObject *args)
{
	if ( initFramebuffer() == 0 )
		return NULL;

	memset(fbp+280*480*2,0,finfo.smem_len - (280*480)*2 );

	freeFramebuffer();

	Py_RETURN_TRUE;
}



static PyObject *pydisplay_drawString(PyObject *self, PyObject *args)
{
    const char *str;
	int  x,y,size, erase;
	uint32_t color;

    if (!PyArg_ParseTuple(args, "siiili", &str,&x,&y,&size,&color,&erase))
    {
        return NULL;
    }

    if ( initFramebuffer() == 0 )
	{
        return NULL;
	}

	DrawString(str, x, y, size,color,erase);

    freeFramebuffer();

    return PyLong_FromLong(1);
}


static PyMethodDef pydisplayMethods[] = {
    {"showBMP", pydisplay_showBMP, METH_VARARGS, "Displays a 480x320 24bits BMP."},
    {"showISS", pydisplay_showISS, METH_VARARGS, "Displays a yellow star at the ISS's position."},
	{ "clearScreen", pydisplay_clearScreen, METH_NOARGS, "Clears the screen" },
	{ "clearScreenBottom", pydisplay_clearScreenBottom, METH_NOARGS, "Clears the bottom of the screen" },
	{ "drawString", pydisplay_drawString, METH_VARARGS, "displays a string" },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


static struct PyModuleDef pydisplaymodule = {
    PyModuleDef_HEAD_INIT,
    "pydisplay",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    pydisplayMethods
};



PyMODINIT_FUNC PyInit_pydisplay(void)
{
    return PyModule_Create(&pydisplaymodule);
}
