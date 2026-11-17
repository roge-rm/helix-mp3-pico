/* Changed for midiTracker, from arduino-libhelix's copy:
 * 2026-11-16: rewritten, the decoder's state comes from one block given
 *   with MP3SetPool(), no heap.
 * 2026-11-16: with MP3_POLY_SPLIT, MP3InitDecoder() builds the polyphase
 *   table in that block.
 * 2026-11-16: for helix-mp3-pico, the RealNetworks licence block put back
 *   at the top.
 * See VENDORED.txt. */
/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * buffers.c - allocation and freeing of global MP3 decoder state
 **************************************************************************************/

/* midiTracker replacement for Helix's buffers.c: the decoder's state is
 * carved from one caller-owned block (MP3SetPool()) instead of the heap.
 * MP3FreeDecoder() only forgets it, the caller owns the memory.
 * With MP3_POLY_SPLIT it also builds the polyphase table there. */

#include <string.h>
#include "coder.h"

static unsigned char *poolMem;
static int poolBytes;

#define ALIGN4(n) (((n) + 3) & ~3)

int MP3PoolBytes(void)
{
	return ALIGN4(sizeof(MP3DecInfo)) + ALIGN4(sizeof(FrameHeader)) + ALIGN4(sizeof(SideInfo)) +
	       ALIGN4(sizeof(ScaleFactorInfo)) + ALIGN4(sizeof(HuffmanInfo)) + ALIGN4(sizeof(DequantInfo)) +
	       ALIGN4(sizeof(IMDCTInfo)) + ALIGN4(sizeof(SubbandInfo));
}

void MP3SetPool(void *mem, int nBytes)
{
	poolMem = (unsigned char *)mem;
	poolBytes = nBytes;
}

MP3DecInfo *AllocateBuffers(void)
{
	MP3DecInfo *d;
	unsigned char *p = poolMem;

	if (!p || poolBytes < MP3PoolBytes())
		return 0;
	/* DSP primitives assume their state starts at 0 */
	memset(p, 0, MP3PoolBytes());

	d = (MP3DecInfo *)p;                p += ALIGN4(sizeof(MP3DecInfo));
	d->FrameHeaderPS = p;               p += ALIGN4(sizeof(FrameHeader));
	d->SideInfoPS = p;                  p += ALIGN4(sizeof(SideInfo));
	d->ScaleFactorInfoPS = p;           p += ALIGN4(sizeof(ScaleFactorInfo));
	d->HuffmanInfoPS = p;               p += ALIGN4(sizeof(HuffmanInfo));
	d->DequantInfoPS = p;               p += ALIGN4(sizeof(DequantInfo));
	d->IMDCTInfoPS = p;                 p += ALIGN4(sizeof(IMDCTInfo));
	d->SubbandInfoPS = p;
#ifdef MP3_POLY_SPLIT
	xmp3_PolyInitTable(((SubbandInfo *)p)->polyK, polyCoef);
#endif
	return d;
}

void FreeBuffers(MP3DecInfo *mp3DecInfo)
{
	(void)mp3DecInfo;
}
