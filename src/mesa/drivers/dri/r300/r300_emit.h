/*
 * Copyright (C) 2005 Vladimir Dergachev.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Vladimir Dergachev <volodya@mindspring.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 *   Aapo Tahkola <aet@rasterburn.org>
 *   Ben Skeggs <darktama@iinet.net.au>
 *   Jerome Glisse <j.glisse@gmail.com>
 */

/* This files defines functions for accessing R300 hardware.
 */
#ifndef __R300_EMIT_H__
#define __R300_EMIT_H__

#include "main/glheader.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "radeon_reg.h"

/* TODO: move these defines (and the ones from DRM) into r300_reg.h and sync up
 * with DRM */
#define CP_PACKET2  (2 << 30)
#define CP_PACKET0(reg, n)	(RADEON_CP_PACKET0 | ((n)<<16) | ((reg)>>2))
#define CP_PACKET3( pkt, n )						\
	(RADEON_CP_PACKET3 | (pkt) | ((n) << 16))

static INLINE uint32_t cmdpacket0(struct radeon_screen *rscrn,
                                  int reg, int count)
{
    if (!rscrn->kernel_mm) {
	    drm_r300_cmd_header_t cmd;

    	cmd.packet0.cmd_type = R300_CMD_PACKET0;
	    cmd.packet0.count = count;
    	cmd.packet0.reghi = ((unsigned int)reg & 0xFF00) >> 8;
	    cmd.packet0.reglo = ((unsigned int)reg & 0x00FF);

    	return cmd.u;
    }
    if (count) {
        return CP_PACKET0(reg, count - 1);
    }
    return CP_PACKET2;
}

static INLINE uint32_t cmdvpu(struct radeon_screen *rscrn, int addr, int count)
{
	drm_r300_cmd_header_t cmd;

	cmd.vpu.cmd_type = R300_CMD_VPU;
	cmd.vpu.count = count;
	cmd.vpu.adrhi = ((unsigned int)addr & 0xFF00) >> 8;
	cmd.vpu.adrlo = ((unsigned int)addr & 0x00FF);

	return cmd.u;
}

static INLINE uint32_t cmdr500fp(struct radeon_screen *rscrn,
                                 int addr, int count, int type, int clamp)
{
	drm_r300_cmd_header_t cmd;

	cmd.r500fp.cmd_type = R300_CMD_R500FP;
	cmd.r500fp.count = count;
	cmd.r500fp.adrhi_flags = ((unsigned int)addr & 0x100) >> 8;
	cmd.r500fp.adrhi_flags |= type ? R500FP_CONSTANT_TYPE : 0;
	cmd.r500fp.adrhi_flags |= clamp ? R500FP_CONSTANT_CLAMP : 0;
	cmd.r500fp.adrlo = ((unsigned int)addr & 0x00FF);

	return cmd.u;
}

static INLINE uint32_t cmdpacket3(struct radeon_screen *rscrn, int packet)
{
	drm_r300_cmd_header_t cmd;

	cmd.packet3.cmd_type = R300_CMD_PACKET3;
	cmd.packet3.packet = packet;

	return cmd.u;
}

static INLINE uint32_t cmdcpdelay(struct radeon_screen *rscrn,  
                                  unsigned short count)
{
	drm_r300_cmd_header_t cmd;

	cmd.delay.cmd_type = R300_CMD_CP_DELAY;
	cmd.delay.count = count;

	return cmd.u;
}

static INLINE uint32_t cmdwait(struct radeon_screen *rscrn,
                               unsigned char flags)
{
	drm_r300_cmd_header_t cmd;

	cmd.wait.cmd_type = R300_CMD_WAIT;
	cmd.wait.flags = flags;

	return cmd.u;
}

static INLINE uint32_t cmdpacify(struct radeon_screen *rscrn)
{
	drm_r300_cmd_header_t cmd;

	cmd.header.cmd_type = R300_CMD_END3D;

	return cmd.u;
}

/**
 * Write the header of a packet3 to the command buffer.
 * Outputs 2 dwords and expects (num_extra+1) additional dwords afterwards.
 */
#define OUT_BATCH_PACKET3(packet, num_extra) do {\
    if (!b_l_rmesa->radeonScreen->kernel_mm) { \
    	OUT_BATCH(cmdpacket3(b_l_rmesa->radeonScreen,\
                  R300_CMD_PACKET3_RAW)); \
    }\
	OUT_BATCH(CP_PACKET3((packet), (num_extra))); \
	} while(0)

/**
 * Must be sent to switch to 2d commands
 */
void static INLINE end_3d(r300ContextPtr rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);

    if (!rmesa->radeon.radeonScreen->kernel_mm) {
    	BEGIN_BATCH(1);
	    OUT_BATCH(cmdpacify(rmesa->radeon.radeonScreen));
    	END_BATCH();
    }
}

void static INLINE cp_delay(r300ContextPtr rmesa, unsigned short count)
{
	BATCH_LOCALS(&rmesa->radeon);

    if (!rmesa->radeon.radeonScreen->kernel_mm) {
    	BEGIN_BATCH(1);
	    OUT_BATCH(cmdcpdelay(rmesa->radeon.radeonScreen, count));
    	END_BATCH();
    }
}

void static INLINE cp_wait(r300ContextPtr rmesa, unsigned char flags)
{
	BATCH_LOCALS(&rmesa->radeon);
	uint32_t wait_until;

	if (!rmesa->radeon.radeonScreen->kernel_mm) {
		BEGIN_BATCH_NO_AUTOSTATE(1);
		OUT_BATCH(cmdwait(rmesa->radeon.radeonScreen, flags));
		END_BATCH();
	} else {
		switch(flags) {
		case R300_WAIT_2D:
			wait_until = (1 << 14);
			break;
		case R300_WAIT_3D:
			wait_until = (1 << 15);
			break;
		case R300_NEW_WAIT_2D_3D:
			wait_until = (1 << 14) | (1 << 15);
			break;
		case R300_NEW_WAIT_2D_2D_CLEAN:
			wait_until = (1 << 14) | (1 << 16) | (1 << 18);
			break;
		case R300_NEW_WAIT_3D_3D_CLEAN:
			wait_until = (1 << 15) | (1 << 17) | (1 << 18);
			break;
		case R300_NEW_WAIT_2D_2D_CLEAN_3D_3D_CLEAN:
			wait_until  = (1 << 14) | (1 << 16) | (1 << 18);
			wait_until |= (1 << 15) | (1 << 17) | (1 << 18);
			break;
		default:
			return;
		}
		BEGIN_BATCH_NO_AUTOSTATE(2);
		OUT_BATCH(CP_PACKET0(RADEON_WAIT_UNTIL, 0));
		OUT_BATCH(wait_until);
		END_BATCH();
	}
}

extern int r300EmitArrays(GLcontext * ctx);

extern void r300ReleaseArrays(GLcontext * ctx);
extern int r300PrimitiveType(r300ContextPtr rmesa, int prim);
extern int r300NumVerts(r300ContextPtr rmesa, int num_verts, int prim);

extern void r300EmitCacheFlush(r300ContextPtr rmesa);

extern GLuint r300VAPInputRoute0(uint32_t * dst, GLvector4f ** attribptr,
				 int *inputs, GLint * tab, GLuint nr);
extern GLuint r300VAPInputRoute1(uint32_t * dst, int swizzle[][4], GLuint nr);
extern GLuint r300VAPInputCntl0(GLcontext * ctx, GLuint InputsRead);
extern GLuint r300VAPInputCntl1(GLcontext * ctx, GLuint InputsRead);
extern GLuint r300VAPOutputCntl0(GLcontext * ctx, GLuint OutputsWritten);
extern GLuint r300VAPOutputCntl1(GLcontext * ctx, GLuint OutputsWritten);

#endif
