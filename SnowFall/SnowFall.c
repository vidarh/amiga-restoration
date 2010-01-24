#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <exec/memory.h>
#include <stdio.h>
#include <ctype.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <string.h>
#include <stdlib.h>

#define MAXSNOWMASS 200

struct Screen *s;
struct Window *w;
struct BitMap *save;
struct NewScreen ns={0,0,320,256,5,1,2,0,CUSTOMSCREEN,0,0,0,0};
struct NewWindow nw={0,0,320,256,-1,-1,IDCMP_CLOSEWINDOW,WFLG_CLOSEGADGET|WFLG_BORDERLESS|
					 WFLG_SMART_REFRESH|WFLG_NOCAREREFRESH|WFLG_ACTIVATE,0,0,0,0,0,0,0,0,0,CUSTOMSCREEN};

void __stack_chk_fail() {} /* Workaround for AROS SDK issue w/certain versions of gcc */

struct BitMap *copy_scaled_bitmap (struct BitMap *oldbm, int f) {
  struct BitMap *newbm;
  long oldw = GetBitMapAttr(oldbm,BMA_WIDTH);
  long oldh = GetBitMapAttr(oldbm,BMA_HEIGHT);

  if ((newbm = AllocBitMap (oldw*f,oldh*f,
						   GetBitMapAttr(oldbm,BMA_DEPTH),
						   GetBitMapAttr(oldbm,BMA_FLAGS),
							NULL)))
	{
	  struct BitScaleArgs scale;
	  scale.bsa_SrcX = 0;
	  scale.bsa_SrcY = 0;
	  scale.bsa_SrcWidth = oldw;
	  scale.bsa_SrcHeight = oldh;
	  scale.bsa_DestX = 0;
	  scale.bsa_DestY = 0;
	  scale.bsa_DestWidth =  oldw*f;
	  scale.bsa_DestHeight = oldh*f;
	  scale.bsa_XSrcFactor = oldw;
	  scale.bsa_XDestFactor = oldw*f;
	  scale.bsa_YSrcFactor = oldh;
	  scale.bsa_YDestFactor = oldh*f;
	  scale.bsa_SrcBitMap = oldbm;
	  scale.bsa_DestBitMap = newbm;
	  scale.bsa_Flags = 0;
	  BitMapScale(&scale);
	}
  
  return (newbm);
}


void xabort(char * errortext)
{
	if (errortext!=0) puts(errortext);
	if (w) CloseWindow(w);
	FreeBitMap((void *)save);
	if (s) CloseScreen(s);
   	exit(0);
}

int LoadPicture(char * name) {
  Object *o;
  struct BitMapHeader *bmhd;
  struct BitMap *dtbm;
  ULONG ncols;
  ULONG *cregs;
  long i;

  o = NewDTObject (name,DTA_GroupID,GID_PICTURE,PDTA_Remap,FALSE,TAG_END);
  if (!o) xabort("Unable to open image");

  i = DoMethod (o,DTM_PROCLAYOUT,NULL,1);
  GetDTAttrs (o,PDTA_BitMapHeader,&bmhd,PDTA_BitMap,&dtbm,PDTA_NumColors,&ncols,PDTA_CRegs,&cregs,TAG_END);
  
  ns.LeftEdge = bmhd->bmh_Left;
  ns.TopEdge  = bmhd->bmh_Top;
  ns.Width    = bmhd->bmh_Width;
  ns.Height   = bmhd->bmh_Height;
  ns.Depth    = GetBitMapAttr(dtbm,BMA_DEPTH);
  ns.ViewModes= 0;
  ns.Type     = CUSTOMSCREEN;

  int scale = 1;

  // FIXME: Scaling doesn't work properly. Anyway, should pick a screen mode based on the
  // Workbench instead of just assuming.
  if (ns.Width < 600) {
	//	ns.Width  *= 2;
	//ns.Height *= 2;
	//scale = 2;
  } 
  save = copy_scaled_bitmap(dtbm, scale);

  if ((s=OpenScreen(&ns))==0) xabort("No memory for screen");
  nw.Screen=s;
  nw.Width=ns.Width;
  nw.Height=ns.Height;

  if ((w=(struct Window *)OpenWindow(&nw))==0) xabort("No window here");

  int ci;
  for (ci = 0; ci < ncols; ci++) {
	SetRGB32(&s->ViewPort,ci,cregs[ci*3],cregs[ci*3 + 1], cregs[ci*3 + 2]);
  }

  BltBitMapRastPort(save,0L,0L,w->RPort,0L,0L,ns.Width,ns.Height,0xC0L);
  DisposeDTObject (o);

  return(1);
}

void DoSnow(long mass,long refresh,long wind) {
  struct IntuiMessage *msg;
  register long cx,cy,i,end,oldx,*x,*y,timer;
	
  timer=refresh;
  x=AllocMem(800L,0L);
  y=AllocMem(800L,0L);
  if (mass>MAXSNOWMASS) mass=MAXSNOWMASS;
  for (i=0;i<mass;i++) y[i]=-(rand()%w->Height);
  
  ULONG bg = ReadPixel(w->RPort, 0, 0);
  while ((msg=(struct IntuiMessage *)GetMsg(w->UserPort))==0) {
	if (refresh!=-1) {
	  if (--refresh==0) {
		for (i=0;i<mass;i++)
		  *(save->Planes[0]+(s->Width/8)*y[i]+x[i]/8) |= (128>>(x[i]%8));
		BltBitMap(save,0L,0L,&s->BitMap,0L,0L,(long)s->Width,(long)s->Height,0xC0L,0xFFL,0x00L);
		for (i=0;i<mass;i++)
		  *(save->Planes[0]+(s->Width/8)*y[i]+x[i]/8) &= ~(128>>(x[i]%8));
		refresh=timer;
	  }
	}
	for (i=0;i<mass;i++) {
	  if (y[i]<-1) y[i]++;
	  else {
		cx=x[i];
		cy=y[i];
		if (cy==-1) {cy=0;cx=rand()%w->Width;}
		else {
		  oldx=cx;
		  cx+=(rand()%31-wind-15)/8;
		  if (cx>w->Width-1) cx-=w->Width;
		  if (cx<0) cx+=s->Width;
		  if (ReadPixel(w->RPort,cx,cy+1)== bg) {
			SetAPen(w->RPort, 0);
			WritePixel(w->RPort, oldx,cy);
			cy ++;
			SetAPen(w->RPort, 1);
			WritePixel(w->RPort, cx,cy);
		  } else {
			end=1;
			if ((ReadPixel(w->RPort,cx+1,cy+1)==bg)&&
				(ReadPixel(w->RPort,cx-1,cy+1)==bg)) {
			  if ((cx>0)&&(rand()%2==1)) cx--;
			  else if (cx<w->Width-1) cx++;
			  else cx--;
			  end=0;
			}
			if ((ReadPixel(w->RPort,cx+1,cy+1)==bg)&&(end==1)/* &&(rand()%6!=1) */)
			  {cx++; end=0;}
			if ((ReadPixel(w->RPort,cx-1,cy+1)==bg)&&(end==1)/* &&(rand()%6!=1) */)
			  {cx--; end=0;}
			if (end==0)
			  {
				SetAPen(w->RPort, 0);
				WritePixel(w->RPort, oldx,cy);
				cy++;
			  }
			SetAPen(w->RPort, 1);
			WritePixel(w->RPort, cx,cy);
			if (end==1) cy=-1;
		  }
		}
		x[i]=cx;
		y[i]=cy;
	  }
	}
  }
  ReplyMsg((struct Message *)msg);
  FreeMem(x,800L);
  FreeMem(y,800L);
}

void DisplayUsage() {
  puts("Snowfall - made December 1989 by Lars R. Clausen");
  puts("[43mUsage:[0m SnowFall [-ppicture] [-n#snowflakes] [-trefreshtime] [-wwindforce]");
  exit(0);
}

void OpenAll(char * filename,int backup) {
  ULONG sec,mic;
  CurrentTime(&sec,&mic);
  srand((int)mic);
  if ((LoadPicture(filename))==0) xabort("Can't load picture");
}

int main(int argc,char ** argv) {
  char file[100];
  int number=40,refresh=10000,i,wind=0;
	
  if ((argc==2)&&(strcmp(argv[1],"?")==0)) DisplayUsage();
  
  strcpy(file,"PROGDIR:SnowPic");
  for (i=1;i<argc;i++) {
	if (argv[i][0]=='-')
	  switch (argv[i][1]) {
	  case 'p': strcpy(file,&argv[i][2]); break;
	  case 'n': number=atoi(&argv[i][2]); break;
	  case 't': refresh=atoi(&argv[i][2]); break;
	  case 'w': wind=atoi(&argv[i][2]); break;
	  default: DisplayUsage();
	  }
	else DisplayUsage();
  }
  if (refresh>-1) OpenAll(file,1);
  else OpenAll(file,0);
  DoSnow((long)number,(long)refresh,(long)wind);
  xabort(0);
  return 0; // Won't get here.
}
