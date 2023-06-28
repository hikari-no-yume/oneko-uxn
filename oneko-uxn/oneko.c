/*
 *	oneko  -  X11 猫
 */

#ifndef	lint
static char rcsid[] = "$Header: /home/sun/unix/kato/xsam/oneko/oneko.c,v 1.5 90/10/19 21:25:16 kato Exp $";
#endif

#include "oneko.h"
#include "patchlevel.h"

// Replacements for some X types and functions.
// These are not exactly the same, they just fulfill a similar function.

typedef unsigned char *Pixmap; // 1bpp XBM, BITMAP_WIDTH × BITMAP_HEIGHT pixels

typedef struct _Cursor {
    unsigned char *bitmap; // 1bpp XBM
    unsigned char *mask; // 1bpp XBM
    unsigned char width;
    unsigned char height;
    signed char x_hot;
    signed char y_hot;
} Cursor;

void
DrawXBM(
  unsigned char *bitmap,
  unsigned width,
  unsigned height,
  unsigned x,
  unsigned y,
  unsigned char varvara_pixel_byte
)
{
  // Draw XBM bitmap by breaking it into several 8×8 1-bit Varvara sprites.
  // Conveniently XBM pads the width to a multiple of 8, so we only need to
  // do extra work for the height.

  unsigned width_spr = (width + 7) / 8u;
  unsigned height_spr = (height + 7) / 8u;

  for (unsigned y_spr = 0; y_spr < height_spr; y_spr++) {
    for (unsigned x_spr = 0; x_spr < width_spr; x_spr++) {
      unsigned char sprite[8];
      unsigned sprite_base = width_spr * (y_spr * 8u) + x_spr;
      for (unsigned char y_row = 0; y_row < 8; y_row++) {
        sprite[y_row] = (y_spr * 8u + y_row > height)
                      ? 0
                      : bitmap[sprite_base + width_spr * y_row];
      }
      set_screen_xy(x + x_spr * 8, y + y_spr * 8);
      set_screen_addr(sprite);
      // 0x10 = x-flip. XBM bit order is horizontally flipped versus Varvara.
      draw_sprite(varvara_pixel_byte | 0x10);
    }
  }
}

/*
 *	グローバル変数
 */

char	*ClassName = "Oneko";		/* コマンド名称 */
char	*ProgramName;			/* コマンド名称 */

//Display	*theDisplay;			/* ディスプレイ構造体 */
int	theScreen;			/* スクリーン番号 */
//Window	theRoot;			/* ルートウィンドウのＩＤ */
//Window	theWindow;			/* 猫ウィンドウのＩＤ */
char    *WindowName = NULL;		/* 猫ウィンドウの名前 */
Cursor	theCursor;			/* ねずみカーソル */

unsigned int	WindowWidth;		/* ルートウィンドウの幅 */
unsigned int	WindowHeight;		/* ルートウィンドウの高さ */

//XColor	theForegroundColor;		/* 色 (フォアグラウンド) */
//XColor	theBackgroundColor;		/* 色 (バックグラウンド) */

int Synchronous = False;
/* Types of animals */
#define BITMAPTYPES 6
typedef struct _AnimalDefaults {
  char *name;
  int speed, idle, bitmap_width, bitmap_height;
  int time;
  int off_x, off_y;
  char *cursor,*mask;
  int cursor_width,cursor_height,cursor_x_hot,cursor_y_hot;
} AnimalDefaultsData;

AnimalDefaultsData AnimalDefaultsDataTable[] = 
{
  { "neko", 13, 6, 32, 32, 125, 0, 0, mouse_cursor_bits,mouse_cursor_mask_bits,
      mouse_cursor_width,mouse_cursor_height, mouse_cursor_x_hot,mouse_cursor_y_hot },
  { "tora", 16, 6, 32, 32, 125, 0, 0, mouse_cursor_bits,mouse_cursor_mask_bits,
      mouse_cursor_width,mouse_cursor_height, mouse_cursor_x_hot,mouse_cursor_y_hot },
  { "dog" , 10, 6, 32, 32, 125, 0, 0, bone_cursor_bits,bone_cursor_mask_bits,
      bone_cursor_width,bone_cursor_height, bone_cursor_x_hot,bone_cursor_y_hot },
  { "bsd_daemon" , 16, 6, 32, 32, 300, 22, 20, bsd_cursor_bits,bsd_cursor_mask_bits,
      bsd_cursor_width,bsd_cursor_height, bsd_cursor_x_hot,bsd_cursor_y_hot },
  { "sakura" , 13, 6, 32, 32, 125, 0, 0, card_cursor_bits,card_cursor_mask_bits,
      card_cursor_width,card_cursor_height, card_cursor_x_hot,card_cursor_y_hot },
  { "tomoyo" , 10, 6, 32, 32, 125, 32, 32, petal_cursor_bits,petal_cursor_mask_bits,
      petal_cursor_width,petal_cursor_height, petal_cursor_x_hot,petal_cursor_y_hot },
};

/*
 *	いろいろな初期設定 (オプション、リソースで変えられるよ)
 */

					/* Resource:	*/
char	*Foreground = NULL;		/*   foreground	*/
char	*Background = NULL;		/*   background	*/
int		IntervalTime = 0;		/*   time (milliseconds)	*/
int	NekoSpeed = 0;			/*   speed	*/
int	IdleSpace = 0;			/*   idle	*/
int	NekoMoyou = NOTDEFINED;		/*   tora	*/
int	NoShape = NOTDEFINED;		/*   noshape	*/
int	ReverseVideo = NOTDEFINED;	/*   reverse	*/
int     XOffset=0,YOffset=0;            /* X and Y offsets for cat from mouse
					   pointer. */
/*
 *	いろいろな状態変数
 */

unsigned FramesSinceLastInterval; // timer (frame = 1/60 seconds)

Bool	DontMapped = True;

int	NekoTickCount;		/* 猫動作カウンタ */
int	NekoStateCount;		/* 猫同一状態カウンタ */
int	NekoState;		/* 猫の状態 */

int	MouseX;			/* マウスＸ座標 */
int	MouseY;			/* マウスＹ座標 */

int	PrevMouseX = 0;		/* 直前のマウスＸ座標 */
int	PrevMouseY = 0;		/* 直前のマウスＹ座標 */

int	PrevCursorX = -1;	// last X position of cursor (updated every frame)
int	PrevCursorY = -1;	// last Y position of cursor (updated every frame)

int	NekoX;			/* 猫Ｘ座標 */
int	NekoY;			/* 猫Ｙ座標 */

int	NekoMoveDx;		/* 猫移動距離Ｘ */
int	NekoMoveDy;		/* 猫移動距離Ｙ */

int	NekoLastX;		/* 猫最終描画Ｘ座標 */
int	NekoLastY;		/* 猫最終描画Ｙ座標 */
Pixmap	NekoLastBitmap;	/* 猫最終描画ビットマップ */

/*
 *	その他
 */

Pixmap	Mati2Xbm, Jare2Xbm, Kaki1Xbm, Kaki2Xbm, Mati3Xbm, Sleep1Xbm, Sleep2Xbm;
Pixmap	Mati2Msk, Jare2Msk, Kaki1Msk, Kaki2Msk, Mati3Msk, Sleep1Msk, Sleep2Msk;

Pixmap	AwakeXbm, AwakeMsk;

Pixmap	Up1Xbm, Up2Xbm, Down1Xbm, Down2Xbm, Left1Xbm, Left2Xbm;
Pixmap	Up1Msk, Up2Msk, Down1Msk, Down2Msk, Left1Msk, Left2Msk;
Pixmap	Right1Xbm, Right2Xbm, UpLeft1Xbm, UpLeft2Xbm, UpRight1Xbm;
Pixmap	Right1Msk, Right2Msk, UpLeft1Msk, UpLeft2Msk, UpRight1Msk;
Pixmap	UpRight2Xbm, DownLeft1Xbm, DownLeft2Xbm, DownRight1Xbm, DownRight2Xbm;
Pixmap	UpRight2Msk, DownLeft1Msk, DownLeft2Msk, DownRight1Msk, DownRight2Msk;

Pixmap	UpTogi1Xbm, UpTogi2Xbm, DownTogi1Xbm, DownTogi2Xbm, LeftTogi1Xbm;
Pixmap	UpTogi1Msk, UpTogi2Msk, DownTogi1Msk, DownTogi2Msk, LeftTogi1Msk;
Pixmap	LeftTogi2Xbm, RightTogi1Xbm, RightTogi2Xbm;
Pixmap	LeftTogi2Msk, RightTogi1Msk, RightTogi2Msk;



typedef struct {
    Pixmap	*BitmapCreatePtr;
    char	*PixelPattern[BITMAPTYPES];
    Pixmap	*BitmapMasksPtr;
    char	*MaskPattern[BITMAPTYPES];
} BitmapData;

BitmapData	BitmapDataTable[] =
{
    { &Mati2Xbm,  mati2_bits, mati2_tora_bits, mati2_dog_bits, mati2_bsd_bits, mati2_sakura_bits, mati2_tomoyo_bits,
      &Mati2Msk, mati2_mask_bits, mati2_mask_bits, mati2_dog_mask_bits, mati2_bsd_mask_bits, mati2_sakura_mask_bits, mati2_tomoyo_mask_bits },
    { &Jare2Xbm,  jare2_bits, jare2_tora_bits, jare2_dog_bits, jare2_bsd_bits, jare2_sakura_bits, jare2_tomoyo_bits,
      &Jare2Msk, jare2_mask_bits, jare2_mask_bits, jare2_dog_mask_bits, jare2_bsd_mask_bits, jare2_sakura_mask_bits, jare2_tomoyo_mask_bits },
    { &Kaki1Xbm,  kaki1_bits, kaki1_tora_bits, kaki1_dog_bits, kaki1_bsd_bits, kaki1_sakura_bits, kaki1_tomoyo_bits,
      &Kaki1Msk, kaki1_mask_bits, kaki1_mask_bits, kaki1_dog_mask_bits, kaki1_bsd_mask_bits, kaki1_sakura_mask_bits, kaki1_tomoyo_mask_bits },
    { &Kaki2Xbm,  kaki2_bits, kaki2_tora_bits, kaki2_dog_bits, kaki2_bsd_bits, kaki2_sakura_bits, kaki2_tomoyo_bits,
      &Kaki2Msk, kaki2_mask_bits, kaki2_mask_bits, kaki2_dog_mask_bits, kaki2_bsd_mask_bits, kaki2_sakura_mask_bits, kaki2_tomoyo_mask_bits },
    { &Mati3Xbm,  mati3_bits, mati3_tora_bits, mati3_dog_bits, mati3_bsd_bits, mati3_sakura_bits, mati3_tomoyo_bits,
      &Mati3Msk, mati3_mask_bits, mati3_mask_bits, mati3_dog_mask_bits, mati3_bsd_mask_bits, mati3_sakura_mask_bits, mati3_tomoyo_mask_bits },
    { &Sleep1Xbm,  sleep1_bits, sleep1_tora_bits, sleep1_dog_bits, sleep1_bsd_bits, sleep1_sakura_bits, sleep1_tomoyo_bits,
      &Sleep1Msk, sleep1_mask_bits, sleep1_mask_bits, sleep1_dog_mask_bits, sleep1_bsd_mask_bits, sleep1_sakura_mask_bits, sleep1_tomoyo_mask_bits },
    { &Sleep2Xbm,  sleep2_bits, sleep2_tora_bits, sleep2_dog_bits, sleep2_bsd_bits, sleep2_sakura_bits, sleep2_tomoyo_bits,
      &Sleep2Msk, sleep2_mask_bits, sleep2_mask_bits, sleep2_dog_mask_bits, sleep2_bsd_mask_bits, sleep2_sakura_mask_bits, sleep2_tomoyo_mask_bits },
    { &AwakeXbm,  awake_bits, awake_tora_bits, awake_dog_bits, awake_bsd_bits, awake_sakura_bits, awake_tomoyo_bits,
      &AwakeMsk, awake_mask_bits, awake_mask_bits, awake_dog_mask_bits, awake_bsd_mask_bits, awake_sakura_mask_bits, awake_tomoyo_mask_bits },
    { &Up1Xbm,  up1_bits, up1_tora_bits, up1_dog_bits, up1_bsd_bits, up1_sakura_bits, up1_tomoyo_bits,
      &Up1Msk, up1_mask_bits, up1_mask_bits, up1_dog_mask_bits, up1_bsd_mask_bits, up1_sakura_mask_bits, up1_tomoyo_mask_bits },
    { &Up2Xbm,  up2_bits, up2_tora_bits, up2_dog_bits, up2_bsd_bits, up2_sakura_bits, up2_tomoyo_bits,
      &Up2Msk, up2_mask_bits, up2_mask_bits, up2_dog_mask_bits, up2_bsd_mask_bits, up2_sakura_mask_bits, up2_tomoyo_mask_bits },
    { &Down1Xbm,  down1_bits, down1_tora_bits, down1_dog_bits, down1_bsd_bits, down1_sakura_bits, down1_tomoyo_bits,
      &Down1Msk, down1_mask_bits, down1_mask_bits, down1_dog_mask_bits, down1_bsd_mask_bits, down1_sakura_mask_bits, down1_tomoyo_mask_bits },
    { &Down2Xbm,  down2_bits, down2_tora_bits, down2_dog_bits, down2_bsd_bits, down2_sakura_bits, down2_tomoyo_bits,
      &Down2Msk, down2_mask_bits, down2_mask_bits, down2_dog_mask_bits, down2_bsd_mask_bits, down2_sakura_mask_bits, down2_tomoyo_mask_bits },
    { &Left1Xbm,  left1_bits, left1_tora_bits, left1_dog_bits, left1_bsd_bits, left1_sakura_bits, left1_tomoyo_bits,
      &Left1Msk, left1_mask_bits, left1_mask_bits, left1_dog_mask_bits, left1_bsd_mask_bits, left1_sakura_mask_bits, left1_tomoyo_mask_bits },
    { &Left2Xbm,  left2_bits, left2_tora_bits, left2_dog_bits, left2_bsd_bits, left2_sakura_bits, left2_tomoyo_bits,
      &Left2Msk, left2_mask_bits, left2_mask_bits, left2_dog_mask_bits, left2_bsd_mask_bits, left2_sakura_mask_bits, left2_tomoyo_mask_bits },
    { &Right1Xbm,  right1_bits, right1_tora_bits, right1_dog_bits, right1_bsd_bits, right1_sakura_bits, right1_tomoyo_bits,
      &Right1Msk, right1_mask_bits, right1_mask_bits,right1_dog_mask_bits, right1_bsd_mask_bits, right1_sakura_mask_bits, right1_tomoyo_mask_bits },
    { &Right2Xbm,  right2_bits, right2_tora_bits, right2_dog_bits, right2_bsd_bits, right2_sakura_bits, right2_tomoyo_bits,
      &Right2Msk, right2_mask_bits, right2_mask_bits, right2_dog_mask_bits, right2_bsd_mask_bits, right2_sakura_mask_bits, right2_tomoyo_mask_bits },
    { &UpLeft1Xbm,  upleft1_bits, upleft1_tora_bits, upleft1_dog_bits, upleft1_bsd_bits, upleft1_sakura_bits, upleft1_tomoyo_bits,
      &UpLeft1Msk, upleft1_mask_bits, upleft1_mask_bits, upleft1_dog_mask_bits, upleft1_bsd_mask_bits, upleft1_sakura_mask_bits, upleft1_tomoyo_mask_bits },
    { &UpLeft2Xbm,  upleft2_bits, upleft2_tora_bits, upleft2_dog_bits, upleft2_bsd_bits, upleft2_sakura_bits, upleft2_tomoyo_bits,
      &UpLeft2Msk, upleft2_mask_bits, upleft2_mask_bits,upleft2_dog_mask_bits, upleft2_bsd_mask_bits, upleft2_sakura_mask_bits, upleft2_tomoyo_mask_bits },
    { &UpRight1Xbm,  upright1_bits, upright1_tora_bits, upright1_dog_bits, upright1_bsd_bits, upright1_sakura_bits, upright1_tomoyo_bits,
      &UpRight1Msk, upright1_mask_bits, upright1_mask_bits,upright1_dog_mask_bits, upright1_bsd_mask_bits, upright1_sakura_mask_bits, upright1_tomoyo_mask_bits },
    { &UpRight2Xbm,  upright2_bits, upright2_tora_bits, upright2_dog_bits, upright2_bsd_bits, upright2_sakura_bits, upright2_tomoyo_bits,
      &UpRight2Msk, upright2_mask_bits, upright2_mask_bits,upright2_dog_mask_bits, upright2_bsd_mask_bits, upright2_sakura_mask_bits, upright2_tomoyo_mask_bits },
    { &DownLeft1Xbm,  dwleft1_bits, dwleft1_tora_bits, dwleft1_dog_bits, dwleft1_bsd_bits, dwleft1_sakura_bits, dwleft1_tomoyo_bits,
      &DownLeft1Msk, dwleft1_mask_bits, dwleft1_mask_bits, dwleft1_dog_mask_bits, dwleft1_bsd_mask_bits, dwleft1_sakura_mask_bits, dwleft1_tomoyo_mask_bits },
    { &DownLeft2Xbm,  dwleft2_bits, dwleft2_tora_bits, dwleft2_dog_bits, dwleft2_bsd_bits, dwleft2_sakura_bits, dwleft2_tomoyo_bits,
      &DownLeft2Msk, dwleft2_mask_bits, dwleft2_mask_bits, dwleft2_dog_mask_bits, dwleft2_bsd_mask_bits, dwleft2_sakura_mask_bits, dwleft2_tomoyo_mask_bits },
    { &DownRight1Xbm,  dwright1_bits, dwright1_tora_bits, dwright1_dog_bits, dwright1_bsd_bits, dwright1_sakura_bits, dwright1_tomoyo_bits,
      &DownRight1Msk, dwright1_mask_bits, dwright1_mask_bits, dwright1_dog_mask_bits, dwright1_bsd_mask_bits, dwright1_sakura_mask_bits, dwright1_tomoyo_mask_bits },
    { &DownRight2Xbm,  dwright2_bits, dwright2_tora_bits, dwright2_dog_bits, dwright2_bsd_bits, dwright2_sakura_bits, dwright2_tomoyo_bits,
      &DownRight2Msk, dwright2_mask_bits, dwright2_mask_bits, dwright2_dog_mask_bits, dwright2_bsd_mask_bits, dwright2_sakura_mask_bits, dwright2_tomoyo_mask_bits },
    { &UpTogi1Xbm,  utogi1_bits, utogi1_tora_bits, utogi1_dog_bits, utogi1_bsd_bits, utogi1_sakura_bits, utogi1_tomoyo_bits,
      &UpTogi1Msk, utogi1_mask_bits, utogi1_mask_bits, utogi1_dog_mask_bits, utogi1_bsd_mask_bits, utogi1_sakura_mask_bits, utogi1_tomoyo_mask_bits },
    { &UpTogi2Xbm,  utogi2_bits, utogi2_tora_bits, utogi2_dog_bits, utogi2_bsd_bits, utogi2_sakura_bits, utogi2_tomoyo_bits,
      &UpTogi2Msk, utogi2_mask_bits, utogi2_mask_bits, utogi2_dog_mask_bits, utogi2_bsd_mask_bits, utogi2_sakura_mask_bits, utogi2_tomoyo_mask_bits },
    { &DownTogi1Xbm,  dtogi1_bits, dtogi1_tora_bits, dtogi1_dog_bits, dtogi1_bsd_bits, dtogi1_sakura_bits, dtogi1_tomoyo_bits,
      &DownTogi1Msk, dtogi1_mask_bits, dtogi1_mask_bits, dtogi1_dog_mask_bits, dtogi1_bsd_mask_bits, dtogi1_sakura_mask_bits, dtogi1_tomoyo_mask_bits },
    { &DownTogi2Xbm,  dtogi2_bits, dtogi2_tora_bits, dtogi2_dog_bits, dtogi2_bsd_bits, dtogi2_sakura_bits, dtogi2_tomoyo_bits,
      &DownTogi2Msk, dtogi2_mask_bits, dtogi2_mask_bits, dtogi2_dog_mask_bits, dtogi2_bsd_mask_bits, dtogi2_sakura_mask_bits, dtogi2_tomoyo_mask_bits },
    { &LeftTogi1Xbm,  ltogi1_bits, ltogi1_tora_bits, ltogi1_dog_bits, ltogi1_bsd_bits, ltogi1_sakura_bits, ltogi1_tomoyo_bits,
      &LeftTogi1Msk, ltogi1_mask_bits, ltogi1_mask_bits,ltogi1_dog_mask_bits, ltogi1_bsd_mask_bits, ltogi1_sakura_mask_bits, ltogi1_tomoyo_mask_bits },
    { &LeftTogi2Xbm,  ltogi2_bits, ltogi2_tora_bits, ltogi2_dog_bits, ltogi2_bsd_bits, ltogi2_sakura_bits, ltogi2_tomoyo_bits,
      &LeftTogi2Msk, ltogi2_mask_bits, ltogi2_mask_bits,ltogi2_dog_mask_bits, ltogi2_bsd_mask_bits, ltogi2_sakura_mask_bits, ltogi2_tomoyo_mask_bits },
    { &RightTogi1Xbm,  rtogi1_bits, rtogi1_tora_bits, rtogi1_dog_bits, rtogi1_bsd_bits, rtogi1_sakura_bits, rtogi1_tomoyo_bits,
      &RightTogi1Msk, rtogi1_mask_bits, rtogi1_mask_bits,rtogi1_dog_mask_bits, rtogi1_bsd_mask_bits, rtogi1_sakura_mask_bits, rtogi1_tomoyo_mask_bits },
    { &RightTogi2Xbm,  rtogi2_bits, rtogi2_tora_bits, rtogi2_dog_bits, rtogi2_bsd_bits, rtogi2_sakura_bits, rtogi2_tomoyo_bits,
      &RightTogi2Msk, rtogi2_mask_bits, rtogi2_mask_bits,rtogi2_dog_mask_bits, rtogi2_bsd_mask_bits, rtogi2_sakura_mask_bits, rtogi2_tomoyo_mask_bits },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

typedef struct {
    Pixmap	*TickBitmapPtr;
    Pixmap	*TickMaskPtr;
} Animation;

Animation	AnimationPattern[][2] =
{
  { { &Mati2Xbm, &Mati2Msk },
    { &Mati2Xbm, &Mati2Msk } },		/* NekoState == NEKO_STOP */
  { { &Jare2Xbm, &Jare2Msk },
    { &Mati2Xbm, &Mati2Msk } },		/* NekoState == NEKO_JARE */
  { { &Kaki1Xbm, &Kaki1Msk },
    { &Kaki2Xbm, &Kaki2Msk } },		/* NekoState == NEKO_KAKI */
  { { &Mati3Xbm, &Mati3Msk },
    { &Mati3Xbm, &Mati3Msk } },		/* NekoState == NEKO_AKUBI */
  { { &Sleep1Xbm, &Sleep1Msk },
    { &Sleep2Xbm, &Sleep2Msk } },		/* NekoState == NEKO_SLEEP */
  { { &AwakeXbm, &AwakeMsk },
    { &AwakeXbm, &AwakeMsk } },		/* NekoState == NEKO_AWAKE */
  { { &Up1Xbm, &Up1Msk },
    { &Up2Xbm, &Up2Msk } },		/* NekoState == NEKO_U_MOVE */
  { { &Down1Xbm, &Down1Msk },
    { &Down2Xbm, &Down2Msk } },		/* NekoState == NEKO_D_MOVE */
  { { &Left1Xbm, &Left1Msk },
    { &Left2Xbm, &Left2Msk } },		/* NekoState == NEKO_L_MOVE */
  { { &Right1Xbm, &Right1Msk },
    { &Right2Xbm, &Right2Msk } },		/* NekoState == NEKO_R_MOVE */
  { { &UpLeft1Xbm, &UpLeft1Msk },
    { &UpLeft2Xbm, &UpLeft2Msk } },	/* NekoState == NEKO_UL_MOVE */
  { { &UpRight1Xbm, &UpRight1Msk },
    { &UpRight2Xbm, &UpRight2Msk } },	/* NekoState == NEKO_UR_MOVE */
  { { &DownLeft1Xbm, &DownLeft1Msk },
    { &DownLeft2Xbm, &DownLeft2Msk } },	/* NekoState == NEKO_DL_MOVE */
  { { &DownRight1Xbm, &DownRight1Msk },
    { &DownRight2Xbm, &DownRight2Msk } },	/* NekoState == NEKO_DR_MOVE */
  { { &UpTogi1Xbm, &UpTogi1Msk },
    { &UpTogi2Xbm, &UpTogi2Msk } },	/* NekoState == NEKO_U_TOGI */
  { { &DownTogi1Xbm, &DownTogi1Msk },
    { &DownTogi2Xbm, &DownTogi2Msk } },	/* NekoState == NEKO_D_TOGI */
  { { &LeftTogi1Xbm, &LeftTogi1Msk },
    { &LeftTogi2Xbm, &LeftTogi2Msk } },	/* NekoState == NEKO_L_TOGI */
  { { &RightTogi1Xbm, &RightTogi1Msk },
    { &RightTogi2Xbm, &RightTogi2Msk } },	/* NekoState == NEKO_R_TOGI */
};

/*
 *	ビットマップデータ初期化
 */

void
InitBitmaps(void)
{
    BitmapData	*BitmapDataTablePtr;

    for (BitmapDataTablePtr = BitmapDataTable;
	 BitmapDataTablePtr->BitmapCreatePtr != NULL;
	 BitmapDataTablePtr++) {

	*(BitmapDataTablePtr->BitmapCreatePtr)
	    = BitmapDataTablePtr->PixelPattern[NekoMoyou];

	*(BitmapDataTablePtr->BitmapMasksPtr)
	    = BitmapDataTablePtr->MaskPattern[NekoMoyou];
    }
}

/*
 *	オプションを設定
 */

void
GetResources(void)
{
  if (Foreground == NULL) {
    Foreground = DEFAULT_FOREGROUND;
  }
  if (Background == NULL) {
    Background = DEFAULT_BACKGROUND;
  }
  if (NekoMoyou == NOTDEFINED) {
    NekoMoyou = 0;
  }
  if (IntervalTime == 0) {
    IntervalTime = AnimalDefaultsDataTable[NekoMoyou].time;
  }
  if (NekoSpeed == 0) {
    NekoSpeed = AnimalDefaultsDataTable[NekoMoyou].speed;
  }
  if (IdleSpace == 0) {
    IdleSpace = AnimalDefaultsDataTable[NekoMoyou].idle;
  }
  XOffset = XOffset + AnimalDefaultsDataTable[NekoMoyou].off_x;
  YOffset = YOffset + AnimalDefaultsDataTable[NekoMoyou].off_y;
  if (NoShape == NOTDEFINED) {
    NoShape = False;
  }
  if (ReverseVideo == NOTDEFINED) {
    ReverseVideo = False;
  }
}

/*
 *	ねずみ型カーソルを作る
 */

void
MakeMouseCursor(void)
{
    theCursor.bitmap = AnimalDefaultsDataTable[NekoMoyou].cursor;
    theCursor.mask = AnimalDefaultsDataTable[NekoMoyou].mask;
    theCursor.width = AnimalDefaultsDataTable[NekoMoyou].cursor_width;
    theCursor.height = AnimalDefaultsDataTable[NekoMoyou].cursor_height;
    theCursor.x_hot = AnimalDefaultsDataTable[NekoMoyou].cursor_x_hot,
    theCursor.y_hot = AnimalDefaultsDataTable[NekoMoyou].cursor_y_hot;
}

/*
 *	色を初期設定する
 */

void
SetupColors(void)
{
    // 0 = background = white (0xfff)
    // 1 = foreground = black (0x000)
    // 2 = second copy of background color for masking when drawing cursor
    // 3 = unused
    set_palette(0xf0f0, 0xf0f0, 0xf0f0);

    // TODO: support custom colors:
    /*XColor	theExactColor;
    Colormap	theColormap;

    theColormap = DefaultColormap(theDisplay, theScreen);

    if (ReverseVideo == True) {
	char	*tmp;

	tmp = Foreground;
	Foreground = Background;
	Background = tmp;
    }

    if (!XAllocNamedColor(theDisplay, theColormap,
		Foreground, &theForegroundColor, &theExactColor)) {
	fprintf(stderr, "%s: Can't XAllocNamedColor(\"%s\").\n",
		ProgramName, Foreground);
	exit(1);
    }

    if (!XAllocNamedColor(theDisplay, theColormap,
		Background, &theBackgroundColor, &theExactColor)) {
	fprintf(stderr, "%s: Can't XAllocNamedColor(\"%s\").\n",
		ProgramName, Background);
	exit(1);
    }*/
}

/*
 *	スクリーン環境初期化
 */

void
InitScreen(
    char	*DisplayName
)
{

  GetResources();

  WindowWidth = 255;
  WindowHeight = 255;
  set_screen_size(WindowWidth, WindowHeight);

  SetupColors();
  MakeMouseCursor();

  InitBitmaps();

  /*XSelectInput(theDisplay, theWindow, 
	       ExposureMask|VisibilityChangeMask|KeyPressMask);

  XFlush(theDisplay);*/
}


/*
 *	ティックカウント処理
 */

void
TickCount(void)
{
    if (++NekoTickCount >= MAX_TICK) {
	NekoTickCount = 0;
    }

    if (NekoTickCount % 2 == 0) {
	if (NekoStateCount < MAX_TICK) {
	    NekoStateCount++;
	}
    }
}


/*
 *	猫状態設定
 */

void
SetNekoState(
    int		SetValue
)
{
    NekoTickCount = 0;
    NekoStateCount = 0;

    NekoState = SetValue;
}


/*
 *	猫描画処理
 */

void
DrawNeko(
    int		x,
    int		y,
    Animation	DrawAnime
)
{
/*@@@@@@*/
    /*register*/ Pixmap	DrawBitmap = *(DrawAnime.TickBitmapPtr);
    /*register*/ Pixmap	DrawMask = *(DrawAnime.TickMaskPtr);

    if ((x != NekoLastX) || (y != NekoLastY)
		|| (DrawBitmap != NekoLastBitmap)) {

      // clear background screen layer
      set_screen_xy(0, 0);
      draw_pixel(BgFillBR | 0);

      DrawXBM(DrawBitmap, BITMAP_WIDTH, BITMAP_HEIGHT, x, y, Bg1 | 0x1);
      // TODO: masking (SHAPE)
    }

    NekoLastX = x;
    NekoLastY = y;

    NekoLastBitmap = DrawBitmap;
}


/*
 *	猫移動方法決定
 *
 *      This sets the direction that the neko is moving.
 *
 */

void
NekoDirection(void)
{
    int			NewState;

    if (NekoMoveDx == 0 && NekoMoveDy == 0) {
	NewState = NEKO_STOP;
    } else {
	unsigned AbsDx = (NekoMoveDx >= 0) ? NekoMoveDx : -NekoMoveDx;
	unsigned AbsDy = (NekoMoveDy >= 0) ? NekoMoveDy : -NekoMoveDy;
	Bool XBig = (AbsDx * 2 > AbsDy);
	Bool YBig = (AbsDy * 2 > AbsDx);
	if (NekoMoveDx > 0) {
	    if (NekoMoveDy < 0) {
			if (YBig && !XBig) {
				NewState = NEKO_U_MOVE;
			} else if (XBig && !YBig) {
				NewState = NEKO_R_MOVE;
			} else {
				NewState = NEKO_UR_MOVE;
			}
	    } else {
			if (YBig && !XBig) {
				NewState = NEKO_D_MOVE;
			} else if (XBig && !YBig) {
				NewState = NEKO_R_MOVE;
			} else {
				NewState = NEKO_DR_MOVE;
			}
	    }
	} else {
	    if (NekoMoveDy < 0) {
			if (YBig && !XBig) {
				NewState = NEKO_U_MOVE;
			} else if (XBig && !YBig) {
				NewState = NEKO_L_MOVE;
			} else {
				NewState = NEKO_UL_MOVE;
			}
	    } else {
			if (YBig && !XBig) {
				NewState = NEKO_D_MOVE;
			} else if (XBig && !YBig) {
				NewState = NEKO_L_MOVE;
			} else {
				NewState = NEKO_DL_MOVE;
			}
	    }
	}
    }

    if (NekoState != NewState) {
	SetNekoState(NewState);
    }
}


/*
 *	猫壁ぶつかり判定
 */

Bool
IsWindowOver(void)
{
    Bool	ReturnValue = False;

    if (NekoY <= 0) {
	NekoY = 0;
	ReturnValue = True;
    } else if (NekoY >= WindowHeight - BITMAP_HEIGHT) {
	NekoY = WindowHeight - BITMAP_HEIGHT;
	ReturnValue = True;
    }
    if (NekoX <= 0) {
	NekoX = 0;
	ReturnValue = True;
    } else if (NekoX >= WindowWidth - BITMAP_WIDTH) {
	NekoX = WindowWidth - BITMAP_WIDTH;
	ReturnValue = True;
    }

    return(ReturnValue);
}


/*
 *	猫移動状況判定
 */

Bool
IsNekoDontMove(void)
{
    if (NekoX == NekoLastX && NekoY == NekoLastY) {
	return(True);
    } else {
	return(False);
    }
}


/*
 *	猫移動開始判定
 */

Bool
IsNekoMoveStart(void)
{
    if ((PrevMouseX >= MouseX - IdleSpace
	 && PrevMouseX <= MouseX + IdleSpace) &&
	 (PrevMouseY >= MouseY - IdleSpace 
	 && PrevMouseY <= MouseY + IdleSpace)) {
	return(False);
    } else {
	return(True);
    }
}

// https://en.wikipedia.org/wiki/Integer_square_root#Example_implementation_in_C
unsigned usqrt(unsigned s)
{
    if (s <= 1) return s;
    unsigned x0 = s / 2;
    unsigned x1 = (x0 + s / x0) / 2;
    while (x1 < x0)
    {
         x0 = x1;
         x1 = (x0 + s / x0) / 2;
    }
    return x0;
}

/*
 *	猫移動 dx, dy 計算
 */

void
CalcDxDy(void)
{
    // These are used for Euclidean distance calculation. They must be big
    // enough to hold a value as large as WindowWidth² or WindowHeight².
    // In oneko-sakura these were `double`, but uxn does not have floats.
    // Solution: limit window size to 255×255 (`unsigned char` limit) so that
    // it will be ≤ 65535 (`unsigned int` limit).
    int		LargeX, LargeY;
    unsigned		SquaredLength, Length;

    PrevMouseX = MouseX;
    PrevMouseY = MouseY;

    MouseX = mouse_x();
    MouseY = mouse_y();

    LargeX = MouseX - NekoX - BITMAP_WIDTH / 2;
    LargeY = MouseY - NekoY - BITMAP_HEIGHT;

    SquaredLength = LargeX * LargeX + LargeY * LargeY;

    if (SquaredLength != 0) {
	Length = usqrt(SquaredLength);
	if (Length <= NekoSpeed) {
	    NekoMoveDx = LargeX;
	    NekoMoveDy = LargeY;
	} else {
	    NekoMoveDx = ((NekoSpeed * LargeX) / (signed)Length);
	    NekoMoveDy = ((NekoSpeed * LargeY) / (signed)Length);
	}
    } else {
	NekoMoveDx = NekoMoveDy = 0;
    }
}


/*
 *	動作解析猫描画処理
 */

void
NekoThinkDraw(void)
{
    CalcDxDy();

    if (NekoState != NEKO_SLEEP) {
	DrawNeko(NekoX, NekoY,
		AnimationPattern[NekoState][NekoTickCount & 0x1]);
    } else {
	DrawNeko(NekoX, NekoY,
		AnimationPattern[NekoState][(NekoTickCount >> 2) & 0x1]);
    }

    TickCount();

    switch (NekoState) {
    case NEKO_STOP:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	if (NekoStateCount < NEKO_STOP_TIME) {
	    break;
	}
	if (NekoMoveDx < 0 && NekoX <= 0) {
	    SetNekoState(NEKO_L_TOGI);
	} else if (NekoMoveDx > 0 && NekoX >= WindowWidth - BITMAP_WIDTH) {
	    SetNekoState(NEKO_R_TOGI);
	} else if ((NekoMoveDy < 0 && NekoY <= 0)) {
	    SetNekoState(NEKO_U_TOGI);
	} else if ((NekoMoveDy > 0 && NekoY >= WindowHeight - BITMAP_HEIGHT)) {
	    SetNekoState(NEKO_D_TOGI);
	} else {
	    SetNekoState(NEKO_JARE);
	}
	break;
    case NEKO_JARE:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	if (NekoStateCount < NEKO_JARE_TIME) {
	    break;
	}
	SetNekoState(NEKO_KAKI);
	break;
    case NEKO_KAKI:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	if (NekoStateCount < NEKO_KAKI_TIME) {
	    break;
	}
	SetNekoState(NEKO_AKUBI);
	break;
    case NEKO_AKUBI:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	if (NekoStateCount < NEKO_AKUBI_TIME) {
	    break;
	}
	SetNekoState(NEKO_SLEEP);
	break;
    case NEKO_SLEEP:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	break;
    case NEKO_AWAKE:
	if (NekoStateCount < NEKO_AWAKE_TIME) {
	    break;
	}
	NekoDirection();	/* 猫が動く向きを求める */
	break;
    case NEKO_U_MOVE:
    case NEKO_D_MOVE:
    case NEKO_L_MOVE:
    case NEKO_R_MOVE:
    case NEKO_UL_MOVE:
    case NEKO_UR_MOVE:
    case NEKO_DL_MOVE:
    case NEKO_DR_MOVE:
	NekoX += NekoMoveDx;
	NekoY += NekoMoveDy;
	NekoDirection();
	if (IsWindowOver()) {
	    if (IsNekoDontMove()) {
		SetNekoState(NEKO_STOP);
	    }
	}
	break;
    case NEKO_U_TOGI:
    case NEKO_D_TOGI:
    case NEKO_L_TOGI:
    case NEKO_R_TOGI:
	if (IsNekoMoveStart()) {
	    SetNekoState(NEKO_AWAKE);
	    break;
	}
	if (NekoStateCount < NEKO_TOGI_TIME) {
	    break;
	}
	SetNekoState(NEKO_KAKI);
	break;
    default:
	/* Internal Error */
	SetNekoState(NEKO_STOP);
	break;
    }
}


/*
 *	キーイベント処理
 */

//Bool
//ProcessKeyPress(theKeyEvent)
//    XKeyEvent	*theKeyEvent;
//{
//  int			Length;
//  int			theKeyBufferMaxLen = AVAIL_KEYBUF;
//  char		theKeyBuffer[AVAIL_KEYBUF + 1];
//  KeySym		theKeySym;
//  XComposeStatus	theComposeStatus;
//  Bool		ReturnState;
//
//  ReturnState = True;
//
//  Length = XLookupString(theKeyEvent,
//			 theKeyBuffer, theKeyBufferMaxLen,
//			 &theKeySym, &theComposeStatus);
//
//  if (Length > 0) {
//    switch (theKeyBuffer[0]) {
//    case 'q':
//    case 'Q':
//      if (theKeyEvent->state & Mod1Mask) { /* META (Alt) キー */
//	ReturnState = False;
//      }
//      break;
//    default:
//      break;
//    }
//  }
//
//  return(ReturnState);
//}


/*
 *	イベント処理
 */

//Bool
//ProcessEvent(void)
//{
//    XEvent	theEvent;
//    Bool	ContinueState = True;
//
//    while (XPending(theDisplay)) {
//        XNextEvent(theDisplay,&theEvent);
//	switch (theEvent.type) {
//	case KeyPress:
//	    ContinueState = ProcessKeyPress(&theEvent);
//	    if (!ContinueState) {
//		    return(ContinueState);
//	    }
//	    break;
//	default:
//	    /* Unknown Event */
//	    break;
//	}
//    }
//
//    return(ContinueState);
//}


/*
 *	猫処理
 */

void
ProcessNekoInit(void)
{
  /* 猫の初期化 */

  NekoX = (WindowWidth - BITMAP_WIDTH / 2) / 2;
  NekoY = (WindowHeight - BITMAP_HEIGHT / 2) / 2;

  NekoLastX = NekoX;
  NekoLastY = NekoY;

  SetNekoState(NEKO_STOP);
}

void
ProcessNekoMain(void)
{
  // draw cursor smoothly (every frame)

  int CursorX = mouse_x();
  int CursorY = mouse_y();
  if (CursorX != PrevCursorX || CursorY != PrevCursorY) {
    // update and redraw cursor
    PrevCursorX = CursorX;
    PrevCursorY = CursorY;

    // clear foreground screen layer
    set_screen_xy(0, 0);
    draw_pixel(FgFillBR | 0);

    // draw mask to foreground screen layer
    DrawXBM(
        theCursor.mask,
        theCursor.width,
        theCursor.height,
        CursorX - theCursor.x_hot,
        CursorY - theCursor.y_hot,
        Fg1 | 0xa // blend mode makes 1 bits into color 2 (mask color)
    );

    // draw main bitmap to foreground screen layer
    DrawXBM(
        theCursor.bitmap,
        theCursor.width,
        theCursor.height,
        CursorX - theCursor.x_hot,
        CursorY - theCursor.y_hot,
        Fg1 | 0x5 // blend mode makes 0 bits transparent (mask shows through)
    );
  }

  /* タイマー設定 */

  FramesSinceLastInterval++;
  if ((FramesSinceLastInterval * 1000u) / 60u > IntervalTime) {
    FramesSinceLastInterval = 0;
  } else {
    return;
  }

  /* メイン処理 */

  // TODO: process events
  //do {
  NekoThinkDraw();
  //} while (ProcessEvent());
}


/*
 *	エラー処理
 */

/*int
NekoErrorHandler(dpy, err)
     Display		*dpy;
     XErrorEvent	*err;
{
  char msg[80];
  XGetErrorText(dpy, err->error_code, msg, 80);
  fprintf(stderr, "%s: Error and exit.\n%s\n", ProgramName, msg);
  exit(1);
}*/


/*
 *	Usage
 */

char	*message[] = {
"",
"Options are:",
"-display <display>	: Neko appears on specified display.",
"-fg <color>		: Foreground color",
"-bg <color>		: Background color",
"-speed <dots>",
"-time <microseconds>",
"-idle <dots>",
"-name <name>		: set window name of neko.",
"-rv			: Reverse video. (effects monochrome display only)",
"-position <geometry>   : adjust position relative to mouse pointer.",
"-debug                 : puts you in synchronous mode.",
"-patchlevel            : print out your current patchlevel.",
NULL };

/*void
Usage(void)
{
  char	**mptr;
  int loop;

  mptr = message;
  fprintf(stderr, "Usage: %s [<options>]\n", ProgramName);
  while (*mptr) {
    fprintf(stderr,"%s\n", *mptr);
    mptr++;
  }
  for (loop=0;loop<BITMAPTYPES;loop++)
    fprintf(stderr,"-%s Use %s bitmaps\n",AnimalDefaultsDataTable[loop].name,AnimalDefaultsDataTable[loop].name);
}*/


/*
 *	オプションの理解
 */

/*Bool
GetArguments(
    int		argc,
    char	*argv[],
    char	*theDisplayName
)
{
  int		ArgCounter;
  int    result,foo,bar;
  extern int XOffset,YOffset;
  int loop,found=0;

  theDisplayName[0] = '\0';

  for (ArgCounter = 0; ArgCounter < argc; ArgCounter++) {

    if (strncmp(argv[ArgCounter], "-h", 2) == 0) {
      Usage();
      exit(0);
    }
    if (strcmp(argv[ArgCounter], "-display") == 0) {
      ArgCounter++;
      if (ArgCounter < argc) {
	strcpy(theDisplayName, argv[ArgCounter]);
      } else {
	fprintf(stderr, "%s: -display option error.\n", ProgramName);
	exit(1);
      }
    }
    else if (strcmp(argv[ArgCounter], "-speed") == 0) {
      ArgCounter++;
      if (ArgCounter < argc) {
	NekoSpeed = atof(argv[ArgCounter]);
      } else {
	fprintf(stderr, "%s: -speed option error.\n", ProgramName);
	exit(1);
      }
    }
    else if (strcmp(argv[ArgCounter], "-time") == 0) {
      ArgCounter++;
      if (ArgCounter < argc) {
	IntervalTime = atol(argv[ArgCounter]);
      } else {
	fprintf(stderr, "%s: -time option error.\n", ProgramName);
	exit(1);
      }
    }
    else if (strcmp(argv[ArgCounter], "-idle") == 0) {
      ArgCounter++;
      if (ArgCounter < argc) {
	IdleSpace = atol(argv[ArgCounter]);
      } else {
	fprintf(stderr, "%s: -idle option error.\n", ProgramName);
	exit(1);
      }
    }
    else if (strcmp(argv[ArgCounter], "-name") == 0) {
      ArgCounter++;
      if (ArgCounter < argc) {
	WindowName = argv[ArgCounter];
      } else {
	fprintf(stderr, "%s: -name option error.\n", ProgramName);
	exit(1);
      }
    }
    else if ((strcmp(argv[ArgCounter], "-fg") == 0) ||
	     (strcmp(argv[ArgCounter], "-foreground") == 0)) {
      ArgCounter++;
      Foreground = argv[ArgCounter];
	     }
    else if ((strcmp(argv[ArgCounter], "-bg") == 0) ||
	     (strcmp(argv[ArgCounter], "-background") == 0)) {
      ArgCounter++;
      Background = argv[ArgCounter];
	     }
    else if (strcmp(argv[ArgCounter], "-rv") == 0) {
      ReverseVideo = True;
    }
    else if (strcmp(argv[ArgCounter], "-noshape") == 0) {
      NoShape = True;
    }
    else if (strcmp(argv[ArgCounter], "-position") == 0) {
      ArgCounter++;
      result=XParseGeometry(argv[ArgCounter],&XOffset,&YOffset,&foo,&bar);
    }
    else if (strcmp(argv[ArgCounter], "-debug") ==0) {
      Synchronous = True;
    }
    else if (strcmp(argv[ArgCounter], "-patchlevel") == 0) {
      fprintf(stderr,"Patchlevel :%s\n",PATCHLEVEL);
    }
    else {
      char *av = argv[ArgCounter] + 1;
      if (strcmp(av, "bsd") == 0)
	av = "bsd_daemon";
      for (loop=0;loop<BITMAPTYPES;loop++) {
	if (strcmp(av,AnimalDefaultsDataTable[loop].name)==0)
	  {NekoMoyou = loop;found=1;}
      }
      if (!found) {
	fprintf(stderr,
		"%s: Unknown option \"%s\".\n", ProgramName,
		argv[ArgCounter]);
	Usage();
	exit(1);
      }
    }
  }

  if (strlen(theDisplayName) < 1) {
    theDisplayName = NULL;
  }
}*/


/*
 *	メイン関数
 */

int
main(
//    int		argc,
//    char	*argv[]
)
{
  char	theDisplayName[MAXDISPLAYNAME];

  //ProgramName = argv[0];

  //argc--;
  //argv++;

  //GetArguments(argc, argv, theDisplayName);

  //XSetErrorHandler(NekoErrorHandler);

  InitScreen(theDisplayName);

  ProcessNekoInit();
}

void on_screen(void)
{
  ProcessNekoMain();
}
