/*

  Changes 3.8.2015 (M. Methfessel):

  - Merged in PS output fix by Alessandro Mottura
  - Merged in "tell" command from xbsa by Jan Labanowski
  - Added animation, adapted from xbsa
  - Animate as rotation with Ctrl-<cursor>
  - Write PS files during animation with Ctrl-O
  - Quick print with Ctrl-P
  - Handle crazy perspective cases, eg. viewpoint inside a ball


 *  xbs: an X-Window ball&sticks plotting program.
 *  Copyright (C) 1995  Michael Methfessel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  The author can be contacted as follows:
 *
 *  Michael Methfessel
 *  methfessel@ihp-ffo.de
 *  Institute for Semiconductor Physics, PO Box 409,
 *  D-15204 Frankfurt (Oder), Germany
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define VERSION "2.0"
#define VDATE   "August 6, 2015"


#define  W_HEIGHT     450      /* startup size */
#define  W_WIDTH      550
#define  NAMAX       16384
#define  NBMAX       262144
#define  NBTMAX       200
#define  NSPMAX        50
#define  NLNMAX        50
#define  PI           3.1415926
#define  MAXRAD     100.0
#define  GRAY0      0.5
#define  PSFAC      1.42
#define  NPOINTS    5
#define  NCOL       31   /* max number of colors to be allocated */
#define  FONT       "8x13"
#define  LABFONT    "6x10"
#define  SHADOW     5
#define  BELL_LEVEL 30
#define  SVINP      10

#define  FBMAX    1000000   /* max for atoms times frames */
#define  NFRMAX   2000000   /* max num of frames */

/* return codes for interpret_keypress routine */
#define K_NOP        0
#define K_QUIT       1
#define K_REPLOT     2
#define K_RESETUP    3
#define K_UPDATE     4
#define K_READ_MORE  5
#define K_READ_DONE  6

/* codes for gray modes */
#define G_STD    0
#define G_RAMP   1
#define G_LIGHT  2

struct ballstr {
  float pos[3];
  float rad;
  float gray;
  float r,g,b;
  int col;
  int special;
  char lab[21];
};


struct stickstr {
  int start;
  int end;
  float rad;
  float gray;
  float r, g, b;
  int col;
};

/* --- global variables --- */

struct {
  char  lab[21];
  float rad;
  float r,g,b;
  char  cname[81];
  int   col;
  float gray;
} spec[NSPMAX];

struct {
  char lab[21];
  float pos[3];
  float pol[3];
} atom[NAMAX];

struct {
  char  lab1[21];
  char  lab2[21];
  float min;
  float max;
  float rad;
  float r,g,b;
  char  cname[81];
  int   col;
  float gray;
} bonds [NBTMAX];

struct {
  float a[3],b[3];
} xline [NLNMAX];    /* for extra lines */

int natom,nbond;
struct ballstr  ball[NAMAX];
struct stickstr stick[NBMAX];

float arc[NPOINTS][2],xbot,xtop,ybot,ytop;

XtAppContext   app_context;
Widget         app_shell;
Display        *dpy;
Window         win;
Drawable       drw;
Pixmap         pixmap, bgrmap;
GC             gc, gcbg, graygc[NCOL], shadowgc, labelgc, labbggc, lngc;
unsigned long  fground, bground, gpx[NCOL];
int            screen, depth, ncol;
Screen        *screenptr;
Colormap       cmap;
Pixmap         stipl;

float p[NAMAX][3];
int nspec, nbas, nbonds, ngray, nxline;
int count;
int ipr=10;
int igeo, igx, igy, igw, igh;
float igs;
int midx, midy;
float alat, dist, dist0, amp, dalfa, scale, tmat[3][3], radfac, bndfac;
float taux, tauy, dtaux, dtauy, taux0, tauy0, bg;
float gslope, gz0, light[3];
float center[3];

/* for animation */
int animate, fps, msecs, framephase;
int animation_print = 0;
int anim_nwrite, anim_nskip, anim_rot_count;
XtIntervalId animation_timer = 0;
char anim_msg[64];

float   frame[3][FBMAX];
char    frstr[NFRMAX][81];
int     nframe,iframe,saveframe;

FILE *outfp;   /*  for PS output file */
int hardcopy, usepixmap, numbers, grayvalues, wrinfo, fstep, wrhelp;
int bline, wire, withbonds, recenter, pmode, gmode, shadow, bondnums;
int color, autocolor, reverse, coords, stippled;
int showaxes, showlines;
int replot, resetup, startup, chginfo;
int num_print;
float pr_xoff, pr_yoff;
int lnum, xln;

char inf[81]="in.bs",  outf[81]="Save.bs",  prf[81]="Bs.ps";
char inmv[81], prfsave[81], wname[81], curf[81];
char gmsg[101],  emsg[101];
char svinput[SVINP][257];
int  svline, nsvline=1;

/* forward declarations */
void WriteStatus (Drawable draw);
void WriteInfo (Drawable draw);
void WriteHelp ();
void draw_plot ();
int match (char str[], char pat[]);

#include "intc.c"
#include "grsubs.h"
#include "hardcopy.h"
#include "subs.h"

#define stipple_width 8
#define stipple_height 8
static unsigned char stipple_bits[] = {
   0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00};


/* --- do_ConfigureNotify --- */
int do_ConfigureNotify (eventp)
XEvent *eventp;
{
  XConfigureEvent *e = (XConfigureEvent *) eventp;
  int  x1, x2, y1, y2, mmx, mmy, h1, w1, xx;

/*  x1 = midx + PSFAC*(taux+xbot);
  x2 = midx + PSFAC*(taux+xtop);
  y1 = midy + PSFAC*(tauy+ybot);
  y2 = midy + PSFAC*(tauy+ytop);
  printf ("old area: X %d %d  Y %d %d\n",  x1, x2, y1, y2);
*/
  mmx = midx;
  mmy = midy;
  w1 = igw;
  h1 = igh;

  if ((e->height != igh) || (e->width != igw)) {
    igw = e->width;
    igh = e->height;
    midx = igw/2;
    midy = igh/2-20;

    /* make new pixmaps but save contents of bgrmap */
    XCopyArea (dpy, bgrmap, pixmap, gc, 0, 0, w1, h1, 0, 0);
    XFreePixmap (dpy, bgrmap);
    bgrmap = XCreatePixmap (dpy, win, igw, igh, depth);
    XFillRectangle (dpy, bgrmap, gcbg, 0, 0, igw, igh);
    XCopyArea (dpy, pixmap, bgrmap, gc, 0, 0, w1, h1, 0, 0);
    XFreePixmap (dpy, pixmap);
    pixmap = XCreatePixmap (dpy, win, igw, igh, depth);

    if (num_print == 0) {   /* normally put back to middle on resize */
      taux = taux0 = 0;
      tauy = tauy0 = 0;
    }
    else {          /* but don't move plot about if building a print */
      taux = taux - (midx-mmx)/PSFAC;
      taux0 = taux0 - (midx-mmx)/PSFAC;
      tauy = tauy - (midy-mmy-igh+h1)/PSFAC;
      tauy0 = tauy0 - (midy-mmy-igh+h1)/PSFAC;
    }

    return 1;
  }
  else
    return 0;
}


/* --- close_print --- */
void close_print(int helpme) {

  if (helpme) {
    sprintf (gmsg,"Usage: close  - close current print file");
    return;
  }

  if (num_print == 0) {
    sprintf (emsg, "close: no print file is open");
    return;
  }

  hardcopy_close();
  XFillRectangle(dpy, bgrmap, gcbg, 0, 0, igw, igh);
  num_print = 0;
}


/* --- handle_print --- */
int handle_print (char *msg1, char *msg2, int helpme)
{
  char xx[81];
  float x,y,x1,y1,z1,x2,y2,z2;
  int ix,iy,dd;
  Drawable drw1;

  if (helpme) {
    sprintf (gmsg,"Usage: print [-T] [-t title] [file]  - print to PS file");
    return 0;
  }

  sprintf (gmsg,"Print:");
  if (num_print > 0 && strcmp(prf,prfsave)) {
    close_print(0);
    sprintf (xx, " %s closed,", prfsave);
    strcat (gmsg, xx);
    replot = 1;
  }

  if (num_print == 0) {
    hardcopy_init (prf);
    sprintf (xx, " %s opened,", prf);
    strcat (gmsg, xx);
    pr_xoff = 0;
    pr_yoff = ybot-15;
  } else {
    hardcopy_redefine();
  }

  num_print++;
  strcpy (prfsave, prf);
  hardcopy = 1;
  bs_transform (natom, ball);
  bs_kernel (natom, ball, nbond, stick);
  hardcopy = 0;
  drw = bgrmap;
  bs_kernel (natom, ball, nbond, stick);

  if (strlen(msg1) > 0) {
    x = pr_xoff + taux;
    y = pr_yoff + tauy;
    hardcopy_label (x, y,  msg1);
    ix = midx + PSFAC*x;
    iy = midy - PSFAC*y;
    dd = strlen(msg1)*6;
    XDrawString (dpy, win, labelgc, ix-dd/2, iy, msg1, strlen(msg1));
    XDrawString (dpy, bgrmap, labelgc, ix-dd/2, iy, msg1, strlen(msg1));
  }

  if (strlen(msg2) > 0) {
    x = pr_xoff + taux;
    y = pr_yoff + tauy - 10;
    hardcopy_label (x, y,  msg2);
    ix = midx + PSFAC*x;
    iy = midy - PSFAC*y;
    dd = strlen(msg2)*6;
    XDrawString (dpy, win, labelgc, ix-dd/2, iy, msg2, strlen(msg2));
    XDrawString (dpy, bgrmap, labelgc, ix-dd/2, iy, msg2, strlen(msg2));
  }

  fflush (outfp);
  sprintf(xx," write(%d) to %s", num_print, prf);
  strcat (gmsg, xx);
}



/* --- update_from_file --- */
int update_from_file ()
{
  char msg1[81],msg2[81],str[81],pat[81],w[8][41];
  char *p;
  float sh[3][6],cut[3],cut1,cut2;
  int i,j,n,rc,nw,svstep,svrgb,helpme;

  nbas=0; nbonds=0; nspec=0;
  nframe=1;
  if (!readclusterdata(inf)) {
    sprintf (emsg, "Cannot update from file %s", inf);
    return 0;
  }

  sprintf (curf, "%s ", inf);
  clearline (win, 10, 8);
  showline (win, 10, 8, "Reading ", inmv, " .. please wait");
  XFlush(dpy);
  sprintf(gmsg,"Updated from %s ", inf);
  if(readclusterdata(inmv)) {
    sprintf(gmsg,"Updated from %s and %s", inf, inmv);
    sprintf(frstr[0], "%s", "start ..\0");
    strcat (curf, inmv);
  }

  for (i=0;i<ncol;i++) FreeColorGC (i);
  if (autocolor) set_auto_colors();
  parse_all_colors ();
  if (color)
    SetColors ();
  else {
    if (stippled) SetStippled4x4 ();
    else          SetSmoothGrays ();
  }

  natom = ball_list(ball, 0);
  nbond = stick_list (ball, stick);
  putframe (ball, 0);
  getframe (ball, iframe);
  replot=1;
  resetup=1;
  return 1;
}

/* --- interpret_input --- */
void interpret_input (char inp[]) {
  char msg1[81],msg2[81],str[81],pat[81],w[8][41];
  char *p;
  float sh[3][6],cut[3],cut1,cut2;
  int i,j,n,rc,nw,svstep,svrgb,helpme;

  nw = parse_args(inp, w);
  if (nw==0) {
    sprintf (gmsg, "Cancel input mode");
    return;
  }

  helpme=0;
  if (!strcmp(w[1],"?")) helpme=1;
  if (!strcmp(w[1],"-h")) helpme=1;

  if (abbrev(w[0],"help",4)) {
    if (nw==1) {
      sprintf (gmsg, "Try \'command ?\' or press key h");
      return;
    }
    else {
      helpme=1;
      sprintf (inp, "%s", w[1]);
      nw = parse_args(inp, w);
    }
  }

  if (abbrev(w[0],"update",2)) {
    if (helpme)
      sprintf (gmsg,"Usage: update [-color] [-rv] [+rv] [-bw] [-st] [-auto] "
               "[file]  - update from file");
    else {
      i=1;
      while (i<nw) {
        if      (abbrev(w[i],"-st",3))    { stippled=1; color=0; }
        else if (abbrev(w[i],"-bw",3))    { stippled=0; color=0; }
        else if (abbrev(w[i],"-color",4))   color=1;
        else if (abbrev(w[i],"-auto",5))  { autocolor=1; color=1; }
        else if (abbrev(w[i],"-rv",3))      reverse=1;
        else if (abbrev(w[i],"+rv",3))      reverse=0;
        else if (w[i][0]=='-') {
          sprintf (emsg, "update: unknown flag %s", w[i]);
          return;
        }
        else {
          strext (inf, w[i], "bs", 0);
          strext (inmv, inf, "mv", 1);
        }
        i++;
      }
      update_from_file();
    }
  }

  else if(abbrev(w[0],"save",3)) {
    if (helpme)
      sprintf (gmsg,"Usage: save [-rgb] [-step n] file  - save data");
    else {
      i=1; svstep=1; svrgb=0;
      while (i<nw) {
        if      (abbrev(w[i],"-step",3))   {i++; svstep=atoi(w[i]); }
        else if (abbrev(w[i],"-rgb",4))     svrgb=1;
        else if (w[i][0]=='-') {
          sprintf (emsg, "save: unknown flag %s", w[i]);
          return;
        }
        else strext (outf, w[i], "bs", 0);
        i++;
      }
      showline(win, 10, 8, "Saving .. ", "" ,"");
      XFlush(dpy);
      writeclusterdata(outf, svstep, svrgb);
    }
  }

  else if (abbrev(w[0],"print",2)) {
    if (helpme)
      handle_print (msg1, msg2, helpme);
    else {
      i=1;
      strcpy (msg1, "");
      strcpy (msg2, "");
      while (i<nw) {
        if (!strcmp(w[i],"-t")) {
          i++;
          strcpy (msg1, w[i]);
        }
        else if (!strcmp(w[i],"-T")) {
          sprintf(msg1,"%s frame %d of %d", inf, iframe+1, nframe);
          if (strlen(frstr[iframe])>0) strip(msg2, frstr[iframe]);
        }
        else if (w[i][0]=='-') {
          sprintf (emsg, "print: unknown flag %s", w[i]);
          return;
        }
        else {
          strext (prf, w[i], "ps", 0);
        }
        i++;
      }
      handle_print (msg1, msg2, helpme);
    }
  }

  else if (abbrev(w[0],"close",2)) {
    if (!helpme)
      sprintf(gmsg, "Print: %s closed after %d write%s",
              prf, num_print, num_print==1 ? "" : "s");
    close_print (helpme);
    replot=1;
  }

  else if (abbrev(w[0],"dup",3)) {
    for (i=0;i<3;i++) for (j=0;j<6;j++) sh[i][j]=0;
    sscanf(inp, "%*s %f %f %f %f %f %f %f %f %f %f %f %f",
           &sh[0][0],&sh[1][0],&sh[2][0],&sh[0][1],&sh[1][1],&sh[2][1],
           &sh[0][2],&sh[1][2],&sh[2][2],&sh[0][3],&sh[1][3],&sh[2][3],
           &sh[0][4],&sh[1][4],&sh[2][4],&sh[0][5],&sh[1][5],&sh[2][5]);
    if (duplicate_atoms (sh,helpme)) resetup=2;
  }

  else if (abbrev(w[0],"cut",3)) {
    cut[0]=cut[1]=cut[2]=cut1=cut2=0;
    sscanf(inp, "%*s %f %f %f %f %f",
           &cut[0], &cut[1], &cut[2], &cut1, &cut2);
    if (cut_atoms (cut, cut1, cut2, helpme)) resetup=2;
  }

  else if (abbrev(w[0],"color",3)) {
    strcpy (pat,"");
    sscanf(inp, "%*s %s %n", pat, &n);
    for (i=n;i<strlen(inp)+1;i++) str[i-n]=inp[i];
    if (NewSpecColor (pat,str,helpme)) resetup=1;
  }

  else {
    rc=readclusterline(inp,helpme);
    if (rc==1) replot=1;
    if (rc==2) resetup=1;
  }

}

/* --- print_animation_frame --- */
void print_animation_frame ()
{
  char fname[64];
  struct stat statbuf;
  int rc, nrot;

  if (! animation_print) return;

  /* animation for frames goes to frm_xxxx.ps */
  if (animate < 10)
    sprintf (fname, "frm_%04d.ps", iframe+1);

  /* animation for rotation goes to rot_xxxx.ps */
  else {
    anim_rot_count ++;
    nrot = 360/dalfa;
    sprintf (fname, "rot_%04d.ps", anim_rot_count);

    if (anim_rot_count > nrot) {
      sprintf (anim_msg, " - completed 360 degree turn");
      return;
    }
  }


  rc = stat(fname,&statbuf);
  if (rc == 0) {
    if (animate > 10)
      sprintf (anim_msg, " -  skip %s of %d", fname, nrot);
    else
      sprintf (anim_msg, " -  skip %s", fname);
    anim_nskip++;
    return;
  }


  hardcopy_init (fname);
  pr_xoff = 0;
  pr_yoff = ybot-15;

  hardcopy = 1;
  bs_transform (natom, ball);
  bs_kernel (natom, ball, nbond, stick);
  hardcopy_close();
  hardcopy = 0;
  anim_nwrite ++;

  if (animate > 10)
    sprintf (anim_msg, " - write %s of %d", fname, nrot);
  else
    sprintf (anim_msg, " - write %s", fname);


}


/* --- animation_timer_cb --- */
void animation_timer_cb (Widget w, caddr_t client_data, caddr_t call_data)
{

  /* frames shown as: 1, 2, ... N, 1, 2 .. */
  if (animate == 1) {
    iframe += fstep;
    iframe = (iframe + 2 * nframe) % nframe;
  }

  /* frames shown as: N, N-1, ... 1, N, N-1 ... */
  if (animate == 2) {
    iframe -= fstep;
    iframe = (iframe + 2 * nframe) % nframe;
  }

  /* frames shown as: 1, 2, ... N, N-1, N-2, ... 2, 1, 2, 3... */
  if (animate == 3) {
    iframe += fstep * framephase;
    if ((iframe >= nframe) || (iframe < 0)) {
      framephase *= -1;
      iframe += 2*framephase*fstep;
    }
  }

  if (animate > 10) {
    if      (animate == 11) rotmat (1,  dalfa*PI/180.0);
    else if (animate == 12) rotmat (1, -dalfa*PI/180.0);
    else if (animate == 13) rotmat (2, -dalfa*PI/180.0);
    else if (animate == 14) rotmat (2,  dalfa*PI/180.0);
    else if (animate == 15) rotmat (3,  dalfa*PI/180.0);
    else if (animate == 16) rotmat (3, -dalfa*PI/180.0);

    draw_plot ();
  }

  else {
    getframe (ball, iframe);
    draw_plot ();
  }

  print_animation_frame ();

  animation_timer = XtAppAddTimeOut (app_context, msecs,
                                     (XtTimerCallbackProc)animation_timer_cb, NULL);

}

/* --- control_animation --- */
/*
   mode: 1 for 'A', 2 for 'B', 3 for 'C', 0 to stop
   Note: use ESC or 'r' to stop the animation
*/
void control_animation (int mode)
{
  if (! animate) *anim_msg = anim_nwrite = anim_nskip = anim_rot_count = 0;

  /* stop the animation */
  if (mode == 0) {
    if (animation_timer) XtRemoveTimeOut (animation_timer);
    animation_timer = 0;
    animate = 0;
    if (animation_print)
      sprintf (gmsg, "Stop animation - wrote %d, skipped %d files", anim_nwrite, anim_nskip);
    else
      sprintf (gmsg, "Stop animation");
    return;
  }

  /* start animation as continuous rotation */
  if (mode > 10) {
    animate = mode;
    if (! animation_timer)
      animation_timer = XtAppAddTimeOut (app_context, msecs,
                                         (XtTimerCallbackProc)animation_timer_cb, NULL);
    return;
  }


  /* need at least two frames to animate */
  if (nframe < 2) {
    sprintf (gmsg, "No frames available for animation");
    return;
  }


  /* enable framewise animation in selected mode */

  /* be clever about initial direction in up/down mode */
  if (mode == 3) {
    if (animate == 1) framephase =   1;
    if (animate == 2) framephase =  -1;
    if (animate == 3) framephase *= -1;
  }

  animate = mode;
  print_animation_frame ();

  if (! animation_timer)
    animation_timer = XtAppAddTimeOut (app_context, msecs,
                                       (XtTimerCallbackProc)animation_timer_cb, NULL);

}


/* --- interpret_keypress --- */
int interpret_keypress (XEvent *ev, int *inpmode, char input[],
    int *ixyz, float *alfa)
{
  int count, l, ctrl;
  char buff[8],msg[81];
  KeySym key;

  count = XLookupString (&ev->xkey, buff, 8, &key, 0);

  ctrl  = (ev->xbutton.state & ControlMask);

  if (*inpmode) {
    l = strlen(input);
    if (key == XK_Up) {
      if (svline == 0) strcpy (svinput[0], input);
      if (svline < nsvline-1) svline++;
      strcpy (input, svinput[svline]);
      return K_READ_MORE;
    }
    if (key == XK_Down) {
      if (svline > 0) svline--;
      strcpy (input, svinput[svline]);
      return K_READ_MORE;
    }
    if (key == XK_Down) {
      strcpy (input, "");
      return K_READ_MORE;
    }
    if (key == XK_Return) {
      *inpmode = 0;
      return K_READ_DONE;
    }
    if (key == XK_BackSpace || key == XK_Left) {
      if (l > 0) input[l-1] = '\0';
      return K_READ_MORE;
    }

    if (key == XK_Escape) {
      strcpy (input, "");
      *inpmode = 0;
      return K_READ_DONE;
    }

    if (count > 0) {
      input[l] = buff[0];
      input[l+1] = '\0';
    }
    return K_READ_MORE;
  }


  /* handle keys with Control pressed */
  if (ctrl) {

    /* start animation as continuous rotation */
    if (key == XK_Right)       control_animation (11);
    if (key == XK_Left)        control_animation (12);
    if (key == XK_Up)          control_animation (13);
    if (key == XK_Down)        control_animation (14);
    if (key == XK_comma)       control_animation (15);
    if (key == XK_period)      control_animation (16);

    /* quick print */
    if (key == XK_p) {
      handle_print ("", "", 0);
      close_print (0);
      sprintf (gmsg, "Wrote %s", prf);
    }

    /* animation print on/off */
    if (key == XK_o) {
      if (animation_print) {
        animation_print = 0;
        *anim_msg = 0;
        sprintf (gmsg, "Animation print off");
      }
      else {
        animation_print = 1;
        *anim_msg = 0;
        sprintf (gmsg, "Animation print on");
      }
    }

    return K_NOP;
  }



  /* next few keys control animation */
  if (key == XK_A) { /* start animation 1,2,...N,1,2  */
    control_animation (1);
    return K_RESETUP;
  }

  if (key == XK_B) { /* start animation N,N-1,N-2..,1,N,  */
    control_animation (2);
    return K_RESETUP;
  }

  if (key == XK_C) { /* start animation 1,2, ..., N,N-1,N-2, .. 3,1,2,3...*/
    control_animation (3);
    return K_RESETUP;
  }

  if (key == XK_less) {
    fps -= (fps <= 10 ? 1 : 2);
    if (fps < 1) fps = 1;
    msecs = 1000 / fps;
    sprintf(gmsg, "Animation uses %d fps", fps);
    return K_NOP;
  }

  if (key == XK_greater) {
    fps += (fps < 10 ? 1 : 2);
    if (fps > 20) fps = 20;
    msecs = 1000 / fps;
    sprintf(gmsg, "Animation uses %d fps", fps);
    return K_NOP;
  }

  if (key == XK_Escape) {
    if (animate > 0) {
      control_animation (0);
      return K_NOP;
    }
    return K_NOP;
  }


  if (key == XK_bracketright) {
    if (iframe == nframe-1) { sprintf(gmsg,"Last frame"); return K_NOP; }
    iframe = iframe + fstep;
    if (iframe >= nframe) iframe = nframe - 1;
    return K_RESETUP;
  }

  if (key == XK_bracketleft) {
    if (iframe == 0) {sprintf(gmsg,"First frame"); return K_NOP; }
    iframe = iframe - fstep;
    if (iframe <=0 ) iframe = 0;
    return K_RESETUP;
  }

  if (key == XK_backslash) {
    saveframe = iframe;
    sprintf (gmsg, "Marked frame %d", saveframe+1);
    return K_NOP;
  }

  if (key == XK_bar) {
    if (saveframe==-99) {sprintf(emsg,"No frame was marked"); return K_NOP; }
    if (saveframe<0 || saveframe>=nframe) {
      sprintf(emsg,"Marked frame %d does not exist", saveframe+1); return K_NOP; }
    if (iframe==saveframe) {sprintf(gmsg,"Already at frame %d", saveframe+1); return K_NOP; }
    iframe=saveframe;
    sprintf(gmsg,"Go back to frame %d", saveframe+1);
    return K_RESETUP;
  }

  if (key == XK_braceleft) {iframe=0; return K_RESETUP; }
  if (key == XK_braceright) {iframe=nframe-1; return K_RESETUP; }

  if (key == XK_Q) return K_QUIT;
  if (key == XK_U) return K_UPDATE;
  if (key == XK_h) { wrhelp=1-wrhelp; return K_REPLOT;}

  if (key == XK_i) {
    if (animate) { control_animation (0); *gmsg = 0; }
    strcpy(input, "");
    *inpmode = 1;
    svline=0;
    return K_READ_MORE; }


  *alfa=0; *ixyz=0;
  if (key == XK_Right)   {*alfa =  dalfa; *ixyz=1; return K_REPLOT;}
  if (key == XK_Left)    {*alfa = -dalfa; *ixyz=1; return K_REPLOT;}
  if (key == XK_Up)      {*alfa = -dalfa; *ixyz=2; return K_REPLOT;}
  if (key == XK_Down)    {*alfa =  dalfa; *ixyz=2; return K_REPLOT;}
  if (key == XK_comma)   {*alfa =  dalfa; *ixyz=3; return K_REPLOT;}
  if (key == XK_period)  {*alfa = -dalfa; *ixyz=3; return K_REPLOT;}

  if (key == XK_p) {pmode++; if (pmode==3) pmode=0;  return K_REPLOT;}
  if (key == XK_P) {pmode--; if (pmode==-1) pmode=2; return K_REPLOT;}

  if (key == XK_d)       { dist0 = dist0*1.05 ; return K_REPLOT;}
  if (key == XK_D)       { dist0 = dist0/1.05 ; return K_REPLOT;}
  if (key == XK_r)       { control_animation(0); return K_REPLOT;}

  if (key == XK_plus || key == XK_KP_Add)
    {scale = scale*1.05; return K_REPLOT;}
  if (key==XK_minus || key==XK_KP_Subtract)
    {scale = scale/1.05; return K_REPLOT;}

  if (key == XK_KP_6)   {taux = taux + dtaux; return K_REPLOT;}
  if (key == XK_KP_4)   {taux = taux - dtaux; return K_REPLOT;}
  if (key == XK_KP_8)   {tauy = tauy + dtauy; return K_REPLOT;}
  if (key == XK_KP_2)   {tauy = tauy - dtauy; return K_REPLOT;}
  if (key == XK_KP_7)   {taux = taux0; tauy = tauy0; return K_REPLOT;}
  if (key == XK_KP_Multiply) {
    taux0 = taux; tauy0 = tauy;
    sprintf(gmsg, "New home position: %.2f %.2f", taux0, tauy0);
    chginfo=1;
    return K_NOP;
  }

  if (key == XK_l) {
    bline=1-bline;
    if (withbonds) return K_REPLOT;
    chginfo=1;
    if (bline) sprintf (gmsg, "Bonds as lines       ");
    else       sprintf (gmsg, "Bonds as cylinders   ");
    return K_NOP;
  }

  if (key == XK_L) {
    showlines=showlines+1;
    if (showlines == 3) showlines=0;
    chginfo=1;
    if (showlines == 1) sprintf (gmsg, "Draw extra lines lines       ");
    if (showlines == 2) sprintf (gmsg, "Dash extra lines lines       ");
    if (showlines == 0) sprintf (gmsg, "Supress extra lines lines    ");
    if (nxline>0) return K_REPLOT;
  }

  if (key == XK_s) { shadow=1-shadow; return K_REPLOT; }

  if (key == XK_a) { showaxes=1-showaxes; return K_REPLOT; }

  if (key == XK_w) { wire=1-wire; return K_REPLOT; }

  if (key == XK_x) {
    chginfo=1;
    usepixmap=1-usepixmap;
    if(usepixmap) sprintf (gmsg, "Draw to pixmap buffer");
    else          sprintf (gmsg, "Draw to screen");
    chginfo=1;
    return K_NOP;
  }

  if (key == XK_space) {wrinfo=1-wrinfo; return K_REPLOT; }

  if (key == XK_b) { withbonds=1-withbonds; return K_REPLOT; }
  if (key == XK_n) {
    numbers=numbers+1; if (numbers == 3) numbers=0; return K_REPLOT;
  }
  if (key == XK_c) { coords=1-coords; return K_REPLOT; }
  if (key == XK_N) { bondnums=1-bondnums; return K_REPLOT;}
  if (key == XK_g) { grayvalues=1-grayvalues; return K_REPLOT;}
  return K_NOP;
}


/* --- wln --- */
void wln (char x[])
{
  int xx;

  if (!startup) {
    xx=xln;
    if (xx<1) xx=1;
    XDrawString (dpy, drw, labelgc,xx, 10*lnum, x, strlen(x));
    lnum++;
  }
  else
    printf ("%s\n", x);

}

/* --- WriteHelp --- */
void WriteHelp ()
{
  lnum=1;

  /* commands */
  xln=igw-250;
  wln("COMMANDS for input line:");
  wln("  help  cmd         - on-line help");
  wln("  inc   degrees     - rotation increment");
  wln("  tell  ijkl        - tell geometry");
  wln("  pos   x y         - set position");
  wln("  dpos  dxy         - shift increment");
  wln("  dist  d           - set distance");
  wln("  rfac  fac         - scale all radii");
  wln("  bfac  fac         - scale all bonds");
  wln("  scale fac         - scale plot");
  wln("  gramp slope mid   - gray ramp");
  wln("  light x y z       - light direction");
  wln("  step  n           - set frame step");
  wln("  dup   x y z       - duplicate");
  wln("  cut   x y z a b   - planar cut");
  wln("  frm   n           - go to frame");
  wln("  color pat cname   - query or set color");
  wln("  save   fname      - save data");
  wln("  update fname      - update from file");
  wln("  print  fname      - postscript output");
  wln("  close             - close print file");

  /*  input  */
  xln=igw-250;
  wln(" ");
  wln("INPUT data format:");
  wln("  atom   label x y z ");
  wln("  spec   label radius color");
  wln("  bonds  lab1 lab2 min max radius color");
  wln("  frame  str   x1 ..   - add frame");
  wln("  tmat   t1 .. t9      - viewpoint");
  wln("Also COMMANDS which set parameters");

  /*  keys  */
  lnum = 1;
  xln=10;
  if (wrinfo) lnum = 12;

  wln("KEYS (KP=Keypad):");
  wln("  i        activate input line");
  wln("  h        help");
  wln("  space    info");
  wln("  cursor   rotate R/L, U/D");
  wln("  , .      rotate in plane");
  wln("  + -      zoom");
  wln("  KP 8642  shift plot");
  wln("  KP 7     shift home");
  wln("  KP *     set home");
  wln("  r        redraw");
  wln("  n        atom numbers");
  wln("  c        coordinates");
  wln("  N        bond lengths");
  wln("  w        wire mode");
  wln("  g        gray/bw");
  wln("  b        bonds");
  wln("  l        cylinders/lines");
  wln("  p        perspective");
  wln("  a        show axes");
  wln("  s        line shadows");
  wln("  x        pixmap buffer");
  wln("  d D      distance");
  wln("  [ ]      step frames fw/bk");
  wln("  A        animate 1,2,..N,1,2");
  wln("  B        animate N,N-1,..1,N");
  wln("  C        animate 1,2,..N,N-1..");
  wln("  Ctrl-cursor  rotate animate");
  wln("  ESC      stop animation");
  wln("  < >      animation rate");
  wln("  { }      first/last frame");
  wln("  \\ |      mark/goto frame");
  wln("  Ctl-P    quick print");
  wln("  Ctl-O    animation print");
  wln("  U        update from file");
  wln("  Q        quit");


}


/* --- WriteInfo --- */
void WriteInfo (Drawable draw)
{
  int i;
  float x1,x2,y1,y2,z1,z2;
  char str[201],stt[201];
  char gmd[3][8] = {"std","ramp","light"};

  get_extent(&x1,&x2,&y1,&y2,&z1,&z2);
  sprintf(str,"%d atom%s, ", natom, natom==1?"":"s");
  sprintf(stt,"%d bond%s ", nbond, nbond==1?"":"s");
  showline(draw, 10, igh-20, "", str, stt);

  sprintf(str, "extent x %.2f to %.2f,  y %.2f to %.2f,  z %.2f to %.2f",
         x1,x2,y1,y2,z1,z2);
  showline(draw, 10, igh-30, "", str, "");
  sprintf(str, "tmat %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f",
          tmat[0][0], tmat[0][1], tmat[0][2], tmat[1][0], tmat[1][1],
          tmat[1][2], tmat[2][0], tmat[2][1], tmat[2][2]);
  showline(draw, 10, igh-40, "", str, "");
  sprintf(str, "dist %.2f, scale %.2f, rfac %.2f, bfac %.2f",
         dist0,scale,radfac,bndfac);
  showline(draw, 10, igh-50, "", str, "");
  sprintf(str, "gmode %s, ramp,slope %.2f %.2f, light %.2f %.2f %.2f",
         gmd[gmode], gslope, gz0, light[0],light[1],light[2]);
  showline(draw, 10, igh-60, "", str, "");
  sprintf(str, "pos (%.1f,%.1f), dpos %.1f, home (%.1f,%.1f)",
          taux, tauy, dtaux, taux0, tauy0);
  showline(draw, 10, igh-70, "", str, "");
  sprintf(str, "color %d, reverse %d, stippled %d, pixmap %d, gray %d, "
          "bonds %d, lines %d",
          color, reverse, stippled, usepixmap, grayvalues,
          withbonds, bline);
  showline(draw, 10, igh-80, "", str, "");
  sprintf(str, "input %s %s, save %s, print %s, nprint=%d",
         inf,inmv,outf,prf,num_print);
  showline(draw, 10, igh-90, "", str, "");
}


/* --- WriteStatus --- */
void WriteStatus (Drawable draw)
{
  char pers[3][8] = {"off","pseudo","true"};
  char str[201];

  sprintf(str," %5.2f %5.2f %5.2f   inc=%.1f"
          "   d=%.2f   p=%s",
          tmat[2][0],tmat[2][1],tmat[2][2], dalfa,
          dist0, pers[pmode]);

  showline(draw, 10, 20, "View:", str, "");
  sprintf(str,"Frame %3d of %d (step %d)  <%s>",
          iframe+1, nframe, fstep, frstr[iframe]);
  showline(draw, 10, 44, "Files: ", curf, "");
  showline(draw, 10, 32, str, "", "");

}


/* --- draw_plot --- */
void draw_plot ()
{
  int k;
  char txt[32];

  bs_transform (natom, ball);

  if (usepixmap) {
    drw = pixmap;
    XCopyArea (dpy, bgrmap, pixmap, gc, 0, 0, igw, igh, 0, 0);
  }
  else {
    drw = win;
    if (wrinfo) WriteInfo (drw);
    if (wrhelp>0) WriteHelp ();
    XCopyArea(dpy, bgrmap, win, gc, 0, 0, igw, igh, 0, 0);
  }

  if (!animate)
    showline(win, 10, 8,  "Busy                    ", " "," ");

  bs_kernel (natom, ball, nbond, stick);
  if (startup) showline (win, 10, 8, "Reading ", inmv, " ..");

  WriteStatus (drw);
  if (wrinfo)   WriteInfo (drw);
  if (wrhelp>0) WriteHelp ();

  if (usepixmap)
    XCopyArea(dpy, pixmap, win, gc, 0, 0, igw, igh, 0, 0);

  if (startup) {
    showline(win, 10, 8, "Reading ", inmv, " .. please wait");
    XFlush(dpy);
    k = nframe;
    if (readclusterdata(inmv)) {
      sprintf(frstr[0], "%s", "start ..\0");
      sprintf (gmsg, "%d frames were read from %s", nframe-k, inmv);
      strcat (curf, inmv);
      WriteStatus (win);
    }
    else sprintf (gmsg, "No frames in %s", inmv);
    startup=0;
  }

  if (! usepixmap)
    XCopyArea(dpy, win, pixmap, gc, 0, 0, igw, igh, 0, 0);

  if (!animate)
    showline(win, 10, 8, "Done                          ", ""," ");
  else {
    sprintf (txt, "with %d fps %s", fps, anim_msg);
    if (animate == 1)
      showline(win, 10, 8, "Animate forward ", txt,"            ");
    else if (animate == 2)
      showline(win, 10, 8, "Animate backward ", txt,"           ");
    else if ((animate == 3) && (framephase > 0))
      showline(win, 10, 8, "Animate forward/backward ", txt,"   ");
    else if ((animate == 3) && (framephase < 0))
      showline(win, 10, 8, "Animate backward/forward ", txt,"   ");
    else if (animate > 10)
      showline(win, 10, 8, "Rotate ", txt,"   ");
  }
}




/* --- MAIN --- */
int main (int argc, char *argv[])
{
#include "bs_icon"
  Pixmap        iconmap;
  XEvent        ev,  ev1;
  Font font;
  XSizeHints hint;
  char input[257], msg[81];
  int keytype, inpmode, finished;
  char *p, *a;
  int i, j, k, ixyz;
  float alfa, beta, gama, cut[3], cut1, cut2;
  char str[50];
  char yn[2][5] = { "no", "yes"};
  float phi;
  int argcdum = 0;
  char **argvdum = NULL;


/* --- set parameters and defaults --- */
  dist = dist0 = 15.0;
  scale = 15;
  igs = 1.0;
  taux0 = taux = tauy0 = tauy = 0;
  dtaux = 10;
  dtauy = 10;
  wire = 0;
  bline = 0;
  radfac = 1.0;
  bndfac = 1.0;
  alfa = 0; beta = 0; gama = 0;
  eumat (alfa*PI/180.0, beta*PI/180.0, gama*PI/180.0);
  dalfa = 90.0;
  nspec = 0;
  nbas = 0;
  nbonds = 0;
  nxline = 0;
  alat = 1.0;
  amp = 0.0;
  hardcopy = 0;
  usepixmap = 0;
  numbers = 0;
  pmode = 0;
  recenter = 1;
  wrhelp = 0;
  shadow = 0;
  grayvalues = 1;
  withbonds = 1;
  bondnums = 0;
  color = 1;
  autocolor = 0;
  reverse = 0;
  fstep = 1;
  wrinfo = 0;
  stippled = 0;
  showaxes = 0;
  showlines = 1;
  saveframe = -99;
  coords = 0;
  gmode = G_STD;
  center[0] = center[1] = center[2] = 0;
  startup = 1;

  /* animation */
  animate = 0;
  fps = 5;
  msecs = 200;
  framephase = 1;
  *anim_msg = 0;

  hint.x = 0;
  hint.y = 0;
  hint.width =  W_WIDTH;
  hint.height = W_HEIGHT;

/* --- parse arguments --- */
  i=1; j=0; k=-1; igeo = 0;
  while (i < argc) {
    a = argv[i];
    if (!strcmp(a, "-geo")) {
      i++;
      igeo=XParseGeometry(argv[i], &igx, &igy, &igw, &igh);
    }
    else if (abbrev(a,"-sc",3))        {i++; igs = atof(argv[i]); }
    else if (abbrev(a,"-t",2))         {i++; k = i; }
    else if (abbrev(a,"-st",3))        { stippled = 1; color = 0; }
    else if (abbrev(a,"-bw",3))        { stippled = 0; color = 0; }
    else if (abbrev(a,"-color",4))       color = 1;
    else if (abbrev(a,"-autocolor",5)) { autocolor = 1; color = 1; }
    else if (abbrev(a,"-rv",3))          reverse = 1;
    else if (abbrev(a,"-hh",3))        { WriteHelp(); exit(0); }
    else if (abbrev(a,"-h",2))           j = 1;
    else if (*a == '-') {
      printf ("Unknown flag %s\n", a) ; exit (1); }
    else strext (inf, a, "bs", 0);
    i++;
  }

  p = inf;
  for (i=0; i<strlen(inf); i++) if (inf[i]=='/') p = inf + i + 1;
  strcpy (wname, p);
  if (k > -1) strcpy (wname, argv[k]);

  if (igeo & WidthValue)  hint.width = igw;
  else                    igw = hint.width;
  if (igeo & HeightValue) hint.height = igh;
  else                    igh = hint.height;
  if (igeo & XValue)      hint.x = igx;
  if (igeo & YValue)      hint.y = igy;
  /* need next line to make +0+0 work */
  if ((igeo & XValue) && (hint.x == 0) && (hint.y == 0)) hint.x = 1;

  if (j == 1) {
    printf ("\nUsage:  xbs [flags] id   -- ball-and-sticks plotting program\n"
            "Data is read from files id.bs and id.mv\n"
            "\nFlags:  -geo gg     set window geometry   \n"
            "        -sc x       set scale factor\n"
            "        -t title    set window title\n"
            "        -color      use color\n"
            "        -bw         b/w with smooth grays\n"
            "        -st         b/w with stippled grays\n"
            "        -rv         reverse colors\n"
            "        -autocolor  chose own colors\n"
            "        -hh         long help\n"
            "\nHelp: Enter 'xbs -hh' to get an overview.\n"
            "For on-line help, press key 'h' for the overview or use\n"
            "'help <cmd>' or '<cmd> ?' in the input line for information\n"
            "on a specific command (including possible options).\n"
            "Request the input line with key 'i'.\n");
    printf ("\nSettings: geometry %dx%d%+d%+d, scale %.2f,\n"
            "color %s, autocolor %s, stippled %s, reverse %s\n",
            hint.width,hint.height,hint.x,hint.y, igs,
            yn[color], yn[autocolor], yn[stippled], yn[reverse]);
    printf ("Version %s of %s\n", VERSION, VDATE);

    exit (0);
  }

  if (!strstr(inf,".")) strcat(inf, ".bs");

/* --- read data file --- */
  sprintf (curf,"%s ", inf);
  nframe = 1;
  strext (inmv, inf, "mv", 1);
  if (!readclusterdata(inf)) {
    printf ("Could not open input file %s\n", inf);
    exit (1);
  }
  if (natom <= 80) usepixmap = 1;

/* --- setup --- */

  /* for animation: we need an application context, so we can set a timer
     with XtAppAddTimeOut, and then replace XNextEvent with XtAppNextEvent,
     because that will check for expired timers */

  app_shell = XtAppInitialize (&app_context, "xbs", NULL, 0,
                               &argcdum, argvdum, NULL, NULL, 0);

  screenptr = XtScreen (app_shell);
  depth = DefaultDepthOfScreen (screenptr);
  dpy  = XtDisplay (app_shell);
  screen = DefaultScreen (dpy);
  bground = WhitePixel (dpy, screen);
  fground = BlackPixel (dpy, screen);

  if ((igeo & XValue) && (igeo & XNegative))
    hint.x=igx+WidthOfScreen(screenptr)-hint.width-18;
  if ((igeo & YValue) && (igeo & YNegative))
    hint.y=igy+HeightOfScreen(screenptr)-hint.height-34;

  hint.flags = PPosition | PSize;

  win = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy),
                             hint.x, hint.y, hint.width, hint.height,
                             0, 0, bground);

  midx = igw/2;
  midy = igh/2-20;

  iconmap = XCreateBitmapFromData (dpy, RootWindowOfScreen(screenptr),
                                   bs_icon_bits, bs_icon_width, bs_icon_height);

  XSetStandardProperties (dpy, win, wname, wname, iconmap,
                          argv, argc, &hint);

  gc = XCreateGC (dpy, win, 0, 0);
  XSetBackground (dpy, gc, bground);
  XSetForeground (dpy, gc, fground);
  XSelectInput (dpy, win, (KeyPressMask|ExposureMask|StructureNotifyMask));
  XMapRaised (dpy, win);



  drw=win;

/* --- additional setup for graphics  --- */
  font=XLoadFont (dpy, FONT);
  XSetFont (dpy, gc, font);
  gcbg = XCreateGC (dpy, win, 0, 0);
  XSetForeground (dpy, gcbg, bground);
  shadowgc = XCreateGC (dpy, win, 0, 0);
  XSetLineAttributes (dpy, shadowgc, SHADOW,
                      LineSolid, CapRound, JoinBevel);
  XSetForeground (dpy, shadowgc, bground);
  labelgc = XCreateGC (dpy, win, 0, 0);
  XSetForeground (dpy, labelgc, fground);
  font=XLoadFont (dpy, LABFONT);
  XSetFont (dpy, labelgc, font);
  labbggc = XCreateGC (dpy, win, 0, 0);
  XSetLineAttributes (dpy, labbggc, 13,
                      LineSolid, CapRound, JoinMiter);
  XSetForeground (dpy, labbggc, bground);

  lngc = XCreateGC (dpy, win, 0, 0);
  XSetLineAttributes (dpy, lngc, 1,
                      LineOnOffDash, CapRound, JoinBevel);


  cmap = XDefaultColormap (dpy, screen);

  for (i=0;i<NPOINTS;i++) {
    phi=i*3.1415926/(NPOINTS-1.0);
    arc[i][0] = -sin(phi);
    arc[i][1] =  cos(phi);
  }

  stipl = XCreateBitmapFromData (dpy, win, stipple_bits,
                                 stipple_width, stipple_height);




/* --- allocate colors, set atoms and bonds --- */
  if (autocolor) set_auto_colors();
  parse_all_colors ();
  if (color)
    SetColors ();
  else {
    if (stippled) SetStippled4x4 ();
    else          SetSmoothGrays ();
  }

  natom = ball_list (ball, 10);
  nbond = stick_list (ball, stick);
  putframe (ball, 0);
  getframe (ball, 0);
  sprintf(frstr[0], "%s", " \0");


/* --- create pixmaps for buffering and background --- */
  depth = XDefaultDepth (dpy, screen);
  pixmap = XCreatePixmap (dpy, win, igw, igh, depth);
  bgrmap = XCreatePixmap (dpy, win, igw, igh, depth);
  XFillRectangle(dpy, bgrmap, gcbg, 0, 0, igw, igh);

/* --- main loop --- */
  inpmode = 0;
  finished = 0;
  num_print = 0;
  count = 0;
  iframe = 0;
  keytype = K_NOP;
  strcpy (prfsave, prf);

  while (finished == 0) {
/*|     XNextEvent (dpy, &ev); |*/
    XtAppNextEvent (app_context, &ev);
    ixyz = 0;
    replot = resetup = chginfo=0;
    strcpy (gmsg, "");
    strcpy (emsg, "");

    switch (ev.type) {

    case Expose:
      if (ev.xexpose.count == 0) {
        if(!startup) {
          XCopyArea(dpy, pixmap, win, gc, 0, 0, igw, igh, 0, 0);
          if (inpmode) {
            clearline (win, 10, 8);
            showline (win, 10, 8, "Input: ", input, "_ ");
          }
          if (wrhelp>0) WriteHelp ();
        }
        else
          replot=1;
      }
      break;

    case ConfigureNotify:
      if (do_ConfigureNotify (&ev)) replot=1;
      if (startup) replot=0;
      break;

/* --- process keypress events --- */
    case KeyPress:
      strcpy (gmsg, "");
      strcpy (emsg, "");
      keytype = interpret_keypress (&ev, &inpmode, input, &ixyz, &alfa);
      switch (keytype) {
      case K_QUIT:
        finished = 1;
        break;
      case K_REPLOT:
        replot=1;
        break;
      case K_RESETUP:
        resetup=1;
        break;
      case K_READ_MORE:
        clearline (win, 10, 8);
        showline  (win, 10, 8, "Input: ", input, "_           ");
        break;
      case K_READ_DONE:
        clearline(win, 10, 8);
        if (strlen(input)>0) {
          for (k=SVINP-1; k>1; k--) strcpy(svinput[k],svinput[k-1]);
          strcpy(svinput[1],input);
          nsvline++;
        }
        interpret_input (input);
        break;
      case K_UPDATE:
        update_from_file();
        break;
      }

      break;
    }  /* switch (ev.type) */

/* --- repeat the setup steps --- */
    if (resetup) {
      natom = ball_list (ball, 0);
      getframe (ball, iframe);
      nbond = stick_list (ball, stick);
      replot=1;
    }

/* --- redraw the plot ---- */
    if (replot) {
      rotmat (ixyz, alfa*PI/180.0);
      draw_plot ();

      /* Discard extra keypress events so that rotation stops after
         key is released. Leave one for smooth motion. */
      i = 0;
      while (XCheckTypedEvent(dpy, KeyPress, &ev1)) i = 1;
      if (i == 1) XPutBackEvent(dpy, &ev1);
    }

/* --- handle messages, update info if needed ---- */
    if (strlen(emsg) > 0) {
      clearline (win, 10, 8);
      showline(win, 10, 8, "+++ ", emsg, "");
      showline(pixmap, 10, 8, "+++ ", emsg, "");
      XBell (dpy, BELL_LEVEL);
    }
    else if (strlen(gmsg) > 0) {
      clearline (win, 10, 8);
      showline(win, 10, 8, gmsg, "", "");
      showline(pixmap, 10, 8, gmsg, "", "");
    }

    if ( (!replot) && wrinfo && chginfo) WriteInfo (win);

  }    /* while */


/* --- clean up and exit ---- */
  if (num_print>0) hardcopy_close();
  XFreeGC (dpy, gc);
  XDestroyWindow (dpy, win);
  XCloseDisplay (dpy);
}
