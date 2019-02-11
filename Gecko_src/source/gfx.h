#ifndef _gfx_h_
#define _gfx_h_
//---------------------------------------------------------------------------------
void gfx_init();
void gfx_load_gfx();
void gfx_fade_logo();
void gfx_print(u8 *font,s32 x,s32 y,u8 alpha, char *fmt, ...);
void gfx_printf(u8 *font,s32 x,s32 y,u8 alpha, char *fmt, ...);
void gfx_draw_image(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha);
void gfx_drawtile(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha, f32 frame,f32 maxframe);
void gfx_draw_stars();
void gfx_init_stars();
void gfx_load_gfx1();
//---------------------------------------------------------------------------------
extern void *xfb[2];
extern int whichfb;
extern void *gp_fifo;
extern int loadedpng;
extern Mtx GXmodelView2D;
extern GXRModeObj *rmode;
extern u8 *tex_logo1;
extern u8 *tex_font1;
extern u8 *bubblegfx;

extern u8 widescreen;
extern u8 alphapng;

//---------------------------------------------------------------------------------
#endif 
//---------------------------------------------------------------------------------
