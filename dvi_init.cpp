/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NOTE:
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */

#define DEBUG 0

#include "dviwin.h"
#include <kdebug.h>

#include <qbitmap.h> 
#include <qfileinfo.h>

extern "C" {
#include <kpathsea/config.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/magstep.h>
#include <kpathsea/tex-glyph.h>
#include "dvi.h"
}

#include "glyph.h"
#include "oconfig.h"

extern char *xmalloc (unsigned, const char *);

#ifndef	DVI_BUFFER_LEN
#define	DVI_BUFFER_LEN	512
#endif

extern unsigned char	dvi_buffer[DVI_BUFFER_LEN];
extern struct frame	*current_frame;
extern struct frame	frame0;	/* dummy head of list */

#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE

#define	dvi_oops(str)	(dvi_oops_msg = (str), longjmp(dvi_env, 1))

static	Boolean	font_not_found;
QDateTime dvi_time;

/*
 * DVI preamble and postamble information.
 */
static	char	job_id[300];
static	long	numerator, denominator;

/*
 * Offset in DVI file of last page, set in read_postamble().
 */
static	long	last_page_offset;



/** Release all shrunken bitmaps for all fonts.  */

extern void reset_fonts(void)
{
#ifdef DEBUG_FONTS
  kdDebug() << "Reset Fonts" << endl;
#endif

  struct font *f;
  struct glyph *g;

  for (f = font_head; f != NULL; f = f->next)
    if ((f->flags & font::FONT_LOADED) && !(f->flags & font::FONT_VIRTUAL))
      for (g = f->glyphtable; g < f->glyphtable + font::max_num_of_chars_in_font; ++g)
	g->clearShrunkCharacter();
}



/*
 *	MAGSTEPVALUE - If the given magnification is close to a \magstep
 *	or a \magstephalf, then return twice the number of \magsteps.
 *	Otherwise return NOMAGSTP.
 */

#define	NOMAGSTP (-29999)
#define	NOBUILD	29999


static	int magstepvalue(float *mag)
{
  int m_ret;
  unsigned dpi_ret = kpse_magstep_fix ((unsigned) *mag, (unsigned) pixels_per_inch, &m_ret);
  *mag = (float) dpi_ret; /* MAG is actually a dpi.  */
  return m_ret ? m_ret : NOMAGSTP;
}


/** define_font reads the rest of the fntdef command and then reads in
 *  the specified pixel file, adding it to the global linked-list
 *  holding all of the fonts used in the job.  */

font *define_font(FILE *file, unsigned int cmnd, font *vfparent, QIntDict<struct font> *TeXNumberTable)
  //	FILE		*file;
  //	unsigned int	cmnd;
  //	struct font	*vfparent;	/* vf parent of this font, or NULL */
  //	struct font	**tntable;	/* table for low TeXnumbers */
  //	unsigned int	tn_table_len;	/* length of table for TeXnumbers */
  //	struct tn	**tn_headpp;	/* addr of head of list of TeXnumbers */
{
  struct font *fontp;
  int	magstepval;
  int	size;

  int   TeXnumber = num(file, (int) cmnd - FNTDEF1 + 1);
  long  checksum  = four(file);
  int   scale     = four(file);
  int   design    = four(file);
  int   len       = one(file) + one(file); /* sequence point in the middle */
  char *fontname  = xmalloc((unsigned) len + 1, "font name");
  Fread(fontname, sizeof(char), len, file);
  fontname[len] = '\0';

#ifdef DEBUG_FONTS
  kdDebug() << "Define font \"" << fontname << "\" scale=" << scale << " design=" << design << endl;
#endif

  float  fsize;
  double scale_dimconv;

  if (vfparent == NULL) {
    fsize = 0.001 * scale / design * magnification * pixels_per_inch;
    scale_dimconv = dimconv;
  } else {
    /*
     *	The scaled size is given in units of vfparent->scale * 2 ** -20
     *	SPELL units, so we convert it into SPELL units by multiplying by
     *		vfparent->dimconv.
     *	The design size is given in units of 2 ** -20 pt, so we convert
     *	into SPELL units by multiplying by
     *		(pixels_per_inch * 2**16) / (72.27 * 2**20).
     */
    fsize = (72.27 * (1<<4)) * vfparent->dimconv * scale / design;
    scale_dimconv = vfparent->dimconv;
  }
  magstepval = magstepvalue(&fsize);
  size       = (int)(fsize + 0.5);

  // reuse font if possible
  for (fontp = font_head;; fontp = fontp->next) {
    if (fontp == NULL) {		/* if font doesn't exist yet */
      fontp = new font(fontname, fsize, checksum, magstepval, scale * scale_dimconv / (1<<20));

      fontp->set_char_p = &dviWindow::load_n_set_char;
      /* With virtual fonts, we might be opening another font
	 (pncb.vf), instead of what we just allocated for
	 (rpncb), thus leaving garbage in the structure for
	 when close_a_file comes along looking for something.  */
      if (vfparent == NULL)
	font_not_found |= fontp->load_font();
      fontp->next = font_head;
      font_head   = fontp;
      break;
    }

    if (strcmp(fontname, fontp->fontname) == 0 && size == (int)(fontp->fsize + 0.5)) {
      /* if font already in use */
      fontp->mark_as_used();
      free(fontname);
      break;
    }
  }

  // Insert font in dictionary and make sure the dictionary is big
  // enough
  if (TeXNumberTable->size()-2 <= TeXNumberTable->count())
    // Not quite optimal. The size of the dict. should be a prime. I
    // don't care
    TeXNumberTable->resize(TeXNumberTable->size()*2); 
  TeXNumberTable->insert(TeXnumber, fontp);
  return fontp;
}


/*
 *      process_preamble reads the information in the preamble and stores
 *      it into global variables for later use.
 */
static void process_preamble()
{
  unsigned char   k;

  if (one(dvi_file) != PRE)
    dvi_oops("DVI file doesn't start with preamble");
  if (one(dvi_file) != 2)
    dvi_oops("Wrong version of DVI output for this program");

  numerator     = four(dvi_file);
  denominator   = four(dvi_file);
  magnification = four(dvi_file);
  dimconv       = (((double) numerator * magnification) / ((double) denominator * 1000.));
  dimconv       = dimconv * (((long) pixels_per_inch)<<16) / 254000;
  tpic_conv     = pixels_per_inch * magnification / 1000000.0;
  k             = one(dvi_file);
  Fread(job_id, sizeof(char), (int) k, dvi_file);
  job_id[k] = '\0';
}

/*
 *      find_postamble locates the beginning of the postamble
 *	and leaves the file ready to start reading at that location.
 */
#define	TMPSIZ	516	/* 4 trailer bytes + 512 junk bytes allowed */
static	void find_postamble()
{
  long	pos;
  unsigned char	temp[TMPSIZ];
  unsigned char	*p;
  unsigned char	*p1;
  unsigned char	byte;

  Fseek(dvi_file, (long) 0, 2);
  pos = ftell(dvi_file) - TMPSIZ;
  if (pos < 0)
    pos = 0;
  Fseek(dvi_file, pos, 0);
  p = temp + fread((char *) temp, sizeof(char), TMPSIZ, dvi_file);
  for (;;) {
    p1 = p;
    while (p1 > temp && *(--p1) != TRAILER)
      ;
    p = p1;
    while (p > temp && *(--p) == TRAILER)
      ;
    if (p <= p1 - 4)
      break;	/* found 4 TRAILER bytes */
    if (p <= temp)
      dvi_oops("DVI file corrupted");
  }
  pos += p - temp;
  byte = *p;
  while (byte == TRAILER) {
    Fseek(dvi_file, --pos, 0);
    byte = one(dvi_file);
  }
  if (byte != 2)
    dvi_oops("Wrong version of DVI output for this program");
  Fseek(dvi_file, pos - 4, 0);
  Fseek(dvi_file, sfour(dvi_file), 0);
}


/*
 *      read_postamble reads the information in the postamble,
 *	storing it into global variables.
 *      It also takes care of reading in all of the pixel files for the fonts
 *      used in the job.
 */
void dviWindow::read_postamble(void)
{
  unsigned char   cmnd;
  struct font	*fontp;
  struct font	**fontpp;

  if (one(dvi_file) != POST)
    dvi_oops("Postamble doesn't begin with POST");
  last_page_offset = four(dvi_file);
  if (numerator != four(dvi_file)
      || denominator != four(dvi_file)
      || magnification != four(dvi_file))
    dvi_oops("Postamble doesn't match preamble");
  /* read largest box height and width */
  unshrunk_page_h = (spell_conv(sfour(dvi_file)) >> 16) + basedpi;
  if (unshrunk_page_h < unshrunk_paper_h)
    unshrunk_page_h = unshrunk_paper_h;
  unshrunk_page_w = (spell_conv(sfour(dvi_file)) >> 16) + basedpi;
  if (unshrunk_page_w < unshrunk_paper_w)
    unshrunk_page_w = unshrunk_paper_w;
  (void) two(dvi_file);	/* max stack size */
  total_pages = two(dvi_file);
  font_not_found = False;
  while ((cmnd = one(dvi_file)) >= FNTDEF1 && cmnd <= FNTDEF4)
    (void) define_font(dvi_file, cmnd, (struct font *) NULL, &tn_table);
  if (cmnd != POSTPOST)
    dvi_oops("Non-fntdef command found in postamble");
  if (font_not_found)
    dvi_oops("Not all pixel files were found");

  // free up fonts no longer in use
  fontpp = &font_head;
  while ((fontp = *fontpp) != NULL)
    if (fontp->flags & font::FONT_IN_USE)  // Question: Is this ever false?
      fontpp = &fontp->next;
    else
      delete fontp;
}

static void prepare_pages()
{
  page_offset = (long *) xmalloc((unsigned) total_pages * sizeof(long), "page directory");
  int i = total_pages;
  page_offset[--i] = last_page_offset;
  Fseek(dvi_file, last_page_offset, 0);
  /*
   * Follow back pointers through pages in the DVI file,
   * storing the offsets in the page_offset table.
   */
  while (i > 0) {
    Fseek(dvi_file, (long) (1+4+(9*4)), 1);
    Fseek(dvi_file, page_offset[--i] = four(dvi_file), 0);
  }
}

/** init_dvi_file is the main subroutine for reading the startup
 *  information from the dvi file.  Returns True on success.  */

Boolean dviWindow::init_dvi_file()
{
  if (QFileInfo(dvi_name).isDir())
    return False;

  tn_table.clear();
  
  dvi_time = QFileInfo(dvi_name).lastModified();
  process_preamble();
  find_postamble();
  read_postamble();
  prepare_pages();
  page_w = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;
  if (current_page >= total_pages)
    current_page = total_pages - 1;

  // Extract PostScript from the DVI file, and store the PostScript
  // specials in PostScriptDirectory, and the headers in the
  // PostScriptHeaderString.

  // First, delete the old vector, if there is any
  if (PostScriptDirectory) {
    PostScriptDirectory->clear();
    delete PostScriptDirectory;
  }    
  
  PostScriptDirectory = new QVector<QString>(total_pages+1);
  PostScriptDirectory->setAutoDelete(TRUE);
  PostScriptHeaderString.truncate(0);
  for(int i=0; i<total_pages; i++) {
    PostScriptOutPutString = new QString();

    (void) lseek(fileno(dvi_file), page_offset[i], SEEK_SET);
    bzero((char *) &currinf.data, sizeof(currinf.data));
    currinf.fonttable = tn_table;
    currinf.end       = dvi_buffer;
    currinf.pos       = dvi_buffer;
    currinf._virtual  = NULL;
    draw_part(current_frame = &frame0, dimconv);
    
    PostScriptDirectory->insert(i,PostScriptOutPutString);
  }
  PostScriptOutPutString = NULL;	

  return True;
}


/**
 **	Check for changes in dvi file.
 **/

Boolean dviWindow::check_dvi_file(void)
{
  if (dvi_file == NULL || (dvi_time != QFileInfo(dvi_name).lastModified())) {
    if (dvi_file) {
      Fclose(dvi_file);
      dvi_file = NULL;
    }
    dvi_file = fopen(dvi_name, OPEN_MODE);
    if (dvi_file == NULL || !init_dvi_file())
      dvi_oops("Cannot reopen dvi file.");
    return False;
  }
  return True;
}
