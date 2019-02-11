#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <fat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "gfx/gosscreen1.h"
#include "gfx/dejfont.h"
#include "gfx/bubble.h"
#include "gfx.h"
#include "tools.h"
#include "libpng/pngu/pngu.h"
#include "sd.h"

#define DEFAULT_FIFO_SIZE	(256*1024)
#define fadespeed 4
#define number_of_stars 50
#define z_max 100

typedef struct {
	int x,y;		// screen co-ordinates 
	int yspeed;			// velocity
	float size;
}Star;

void *xfb[2] = { NULL, NULL};
int whichfb = 0;
void *gp_fifo = NULL;
int loadedpng = 0;

u8 widescreen = 0;
u8 alphapng = 0;

Mtx GXmodelView2D;
GXRModeObj *rmode;
u8 *tex_logo1;
u8 *tex_font1;
u8 *bubblegfx;
Star stars[number_of_stars];

int SinTab1[50]=
{100,100,100,100,100,100,100,100,100,100,
50,50,50,50,50,50,50,50,50,50,
0,0,0,0,0,0,0,0,0,0
-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,
0,0,0,0,0,0,0,0,0,0
};

//---------------------------------------------------------------------------------
void gfx_render_direct()
//---------------------------------------------------------------------------------
{
    GX_DrawDone ();
	whichfb ^= 1;		// flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
 	VIDEO_Flush();
 	VIDEO_WaitVSync();
}


//---------------------------------------------------------------------------------
void gfx_print(u8 *font,s32 x,s32 y,u8 alpha, char *fmt, ...)
//---------------------------------------------------------------------------------
{
	int i, len;
	
	len = strlen(fmt);
	
	for(i=0;i<len; i++, x+=10)
	{
		if(( fmt[i] < 33 ) || ( fmt[i] > 126 )){
			continue;
		}

		gfx_drawtile( x, y, 10, 24, font, 0, 1.0f, 1.0f, alpha, fmt[i]-33, 96 );
	}
}

//---------------------------------------------------------------------------------
void gfx_printf(u8 *font,s32 x,s32 y,u8 alpha, char *fmt, ...)
//---------------------------------------------------------------------------------
{
	int i;
	char buf[1024];
	int len;

	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
  
	for(i=0;i<len; i++, x+=10)
	{
		if(( buf[i] < 33 ) || ( buf[i] > 126 )){
			continue;
		}

		gfx_drawtile( x, y, 10, 24, font, 0, 1.0f, 1.0f, alpha, buf[i]-33, 96 );
	}
}

//---------------------------------------------------------------------------------
void gfx_draw_image(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha )
//---------------------------------------------------------------------------------
{	
	GXTexObj texObj;
	GX_InitTexObj(&texObj, data, width,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width *=.5;
	height*=.5;
	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis =(guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	
	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
  	GX_Position3f32(-width, -height,  0);
  	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 0);
  
  	GX_Position3f32(width, -height,  0);
 	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 0);
  
  	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 1);
  
  	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

//---------------------------------------------------------------------------------
void gfx_drawtile(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha, f32 frame,f32 maxframe )
//---------------------------------------------------------------------------------
{
	GXTexObj texObj;
	f32 s1= frame/maxframe;
	f32 s2= (frame+1)/maxframe;
	f32 t1=0;
	f32 t2=1;
	
	GX_InitTexObj(&texObj, data, width*maxframe,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width *=.5;
	height*=.5;
	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis =(guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);
	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
  	GX_Position3f32(-width, -height,  0);
  	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s1, t1);
  
  	GX_Position3f32(width, -height,  0);
 	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s2, t1);
  
  	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s2, t2);
  
  	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s1, t2);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}


//---------------------------------------------------------------------------------
void gfx_init()
//---------------------------------------------------------------------------------
{
	f32 yscale;
	u32 xfbHeight;
	Mtx perspective;
	
	rmode = VIDEO_GetPreferredMode(NULL);

	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
	{
		rmode->viWidth = 678;
		rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678)/2;
	}

	VIDEO_Configure (rmode);

	xfb[0] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	xfb[1] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_ClearFrameBuffer (rmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (rmode, xfb[1], COLOR_BLACK);

	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

	GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);

	GXColor background = { 21, 35, 40, 0xff };
	GX_SetCopyClear (background, 0x00ffffff);

	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa){
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	}
	else{
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	}

	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_ClearVtxDesc();
	GX_InvVtxCache ();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(perspective,0,479,0,639,0,300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	
	GX_SetCullMode(GX_CULL_NONE);
}

//---------------------------------------------------------------------------------
unsigned char* gfx_load_texture(const unsigned char my_png[])
//---------------------------------------------------------------------------------
{
   PNGUPROP imgProp;
   IMGCTX ctx;
   void *my_texture;

   	ctx = PNGU_SelectImageFromBuffer(my_png);
    PNGU_GetImageProperties (ctx, &imgProp);
    my_texture = memalign (32, imgProp.imgWidth * imgProp.imgHeight * 4);
    PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, my_texture, 255);
    PNGU_ReleaseImageContext (ctx);
    DCFlushRange (my_texture, imgProp.imgWidth * imgProp.imgHeight * 4);
	return my_texture;
}


//---------------------------------------------------------------------------------
void gfx_load_gfx()
//---------------------------------------------------------------------------------
{
	//tex_logo1 = gfx_load_texture(gosscreen1);
	tex_font1 = gfx_load_texture(dejfont);
	bubblegfx = gfx_load_texture(bubble);
}

//---------------------------------------------------------------------------------
void gfx_load_gfx1()
//---------------------------------------------------------------------------------
{
	char filepath[MAX_FILEPATH_LEN];
	u8 *filebuff = NULL;
	u32 filesize;
	int ret;
	
	// If no SD or config file has been changed just load defaults
	if(sd_found == 0){
		goto loadgfx;
	}

	if(widescreen) {
		sprintf (filepath, "sd:/data/gecko/background-widea.png");
	} else {
		sprintf (filepath, "sd:/data/gecko/background-fulla.png");
	}
	alphapng = 1;
	FILE *fp = fopen(filepath, "rb");
	
	if(!fp) {
		if(widescreen) {
			sprintf (filepath, "sd:/background-widea.png");
		} else {
			sprintf (filepath, "sd:/background-fulla.png");
		}
		alphapng = 1;
		fp = fopen(filepath, "rb");
	}
	
	if(!fp) {
		if(widescreen) {
			sprintf (filepath, "sd:/data/gecko/background-wide.png");
		} else {
			sprintf (filepath, "sd:/data/gecko/background-full.png");
		}		
		alphapng = 0;
		fp = fopen(filepath, "rb");
	}

	if(!fp) {
		if(widescreen) {
			sprintf (filepath, "sd:/background-wide.png");
		} else {
			sprintf (filepath, "sd:/background-full.png");
		}		
		alphapng = 0;
		fp = fopen(filepath, "rb");			
	}


	if (!fp) { 
		sprintf (filepath, "sd:/data/gecko/backgrounda.png");
		alphapng=1;
		fp = fopen(filepath, "rb");
	}
	
	if (!fp) {
		sprintf (filepath, "sd:/backgrounda.png");
		alphapng=1;
		fp = fopen(filepath, "rb");
	}
	
	if (!fp) {
		sprintf (filepath, "sd:/data/gecko/background.png");
		alphapng=0;
		fp = fopen(filepath, "rb");
	}
	
	if (!fp){
		sprintf (filepath, "sd:/background.png");
		alphapng=0;
		fp = fopen(filepath, "rb");
		if (!fp){
			loadedpng = 0;
			goto loadgfx;
		}
	}
	
	fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    filebuff = (u8*) memalign(32,filesize);
    if(!filebuff){
		fclose(fp);
		loadedpng = 0;
		goto loadgfx;
    }

	ret = fread(filebuff, 1, filesize, fp);
	if(ret != filesize){	
		free(filebuff);
		fclose(fp);
		loadedpng = 0;
		goto loadgfx;
	}
	
	DCFlushRange(filebuff, filesize);
	loadedpng = 1;

	fflush(fp);
	fclose(fp);

loadgfx:
	if(loadedpng){
		tex_logo1 = gfx_load_texture(filebuff);
		free(filebuff);
	}
	else {
		tex_logo1 = gfx_load_texture(gosscreen1);
	}
}

//---------------------------------------------------------------------------------
void gfx_fade_logo()
//---------------------------------------------------------------------------------
{
	int i;
	float fadevalue = 0;

	for (i=0;i<255;)
	{
		fadevalue = i;
		gfx_draw_image(0, 0, 640, 480, tex_logo1, 0, 1, 1, fadevalue);
		gfx_render_direct();
		i += fadespeed;
	}
}

//---------------------------------------------------------------------------------
void gfx_int_stars()
//---------------------------------------------------------------------------------
{
	int i;
	
	for(i = 0; i < number_of_stars; i++) {
		stars[i].x = rand() % (640 - 32 ) << 8;
		stars[i].y = rand() % (480 - 32 ) << 8 ;
		stars[i].yspeed = (rand() & 0xFF) + 0x100;
		stars[i].size = ((float) rand()) / RAND_MAX;
	}
}

//---------------------------------------------------------------------------------
static void gfx_draw_new_star(int i)
//---------------------------------------------------------------------------------
{
		stars[i].x = rand() % (640 - 32 ) << 8 ;
		stars[i].y = (480+32) << 8;
		stars[i].yspeed = (rand() & 0xFF) + 0x100;
		stars[i].size = ((float) rand()) / RAND_MAX;
}

//---------------------------------------------------------------------------------
void gfx_draw_stars()
//---------------------------------------------------------------------------------
{
	int i;
	
	if(config_bytes[6] == 0x01)
	{
	
	
	for(i = 0; i < number_of_stars; i++) {

		if(stars[i].size <= 0.3){
			stars[i].yspeed += 10;
			goto addspeed;
		}

		if(stars[i].size > 0.3 && stars[i].size < 0.5){
			stars[i].yspeed += 5;
			goto addspeed;
		}

		if(stars[i].size > 0.5 && stars[i].size < 0.7){
			stars[i].yspeed += 2;
		}

addspeed:
		
		stars[i].y -= stars[i].yspeed;

		if(stars[i].y < ((0-48) << 8))
		{
			gfx_draw_new_star(i);
		}
		// add some sin to make it more realistic
		stars[i].x += SinTab1[i];

		gfx_draw_image(stars[i].x >> 8, stars[i].y >> 8, 48, 48, bubblegfx, 0, stars[i].size, stars[i].size,255);
	}
	}
}

