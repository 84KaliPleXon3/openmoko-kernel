/* glamo_drm.h -- Public header for the Glamo driver
 *
 * Copyright 2009 Thomas White
 * Copyright 2000 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Fremont, California.
 * Copyright 2002 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Thomas White <taw@bitwiz.org.uk>
 *    Kevin E. Martin <martin@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __GLAMO_DRM_H__
#define __GLAMO_DRM_H__

#include "drm.h"

#define GLAMO_GEM_DOMAIN_VRAM (0x1)

/* Glamo specific ioctls */
#define DRM_GLAMO_CMDBUF     0x01
#define DRM_GLAMO_SWAP       0x02

#define DRM_GLAMO_GEM_INFO     0x1c
#define DRM_GLAMO_GEM_CREATE   0x1d
#define DRM_GLAMO_GEM_MMAP     0x1e
#define DRM_GLAMO_GEM_PIN      0x1f
#define DRM_GLAMO_GEM_UNPIN    0x20
#define DRM_GLAMO_GEM_PREAD    0x21
#define DRM_GLAMO_GEM_PWRITE   0x22
#define DRM_GLAMO_GEM_WAIT_RENDERING 0x24

#define DRM_IOCTL_GLAMO_CMDBUF     DRM_IOW( DRM_COMMAND_BASE + DRM_GLAMO_CMDBUF, drm_glamo_cmd_buffer_t)
#define DRM_IOCTL_GLAMO_SWAP       DRM_IO(  DRM_COMMAND_BASE + DRM_GLAMO_SWAP)

#define DRM_IOCTL_GLAMO_GEM_INFO   DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_INFO, struct drm_glamo_gem_info)
#define DRM_IOCTL_GLAMO_GEM_CREATE DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_CREATE, struct drm_glamo_gem_create)
#define DRM_IOCTL_GLAMO_GEM_MMAP   DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_MMAP, struct drm_glamo_gem_mmap)
#define DRM_IOCTL_GLAMO_GEM_PIN    DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_PIN, struct drm_glamo_gem_pin)
#define DRM_IOCTL_GLAMO_GEM_UNPIN  DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_UNPIN, struct drm_glamo_gem_unpin)
#define DRM_IOCTL_GLAMO_GEM_PREAD  DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_PREAD, struct drm_glamo_gem_pread)
#define DRM_IOCTL_GLAMO_GEM_PWRITE DRM_IOWR(DRM_COMMAND_BASE + DRM_GLAMO_GEM_PWRITE, struct drm_glamo_gem_pwrite)
#define DRM_IOCTL_GLAMO_GEM_WAIT_RENDERING DRM_IOW(DRM_COMMAND_BASE + DRM_GLAMO_GEM_WAIT_RENDERING, struct drm_glamo_gem_wait_rendering)

typedef struct drm_glamo_cmd_buffer {
	int bufsz;
	char __user *buf;
	int nbox;
	struct drm_clip_rect __user *boxes;
} drm_glamo_cmd_buffer_t;

/* return to userspace start/size of gtt and vram apertures */
struct drm_glamo_gem_info {
	uint64_t gart_start;
	uint64_t gart_size;
	uint64_t vram_start;
	uint64_t vram_size;
	uint64_t vram_visible;
};

struct drm_glamo_gem_create {
	uint64_t size;
	uint64_t alignment;
	uint32_t handle;
	uint32_t initial_domain; // to allow VRAM to be created
	uint32_t no_backing_store; // for VRAM objects - select whether they need backing store
	// pretty much front/back/depth don't need it - other things do
};

struct drm_glamo_gem_mmap {
	uint32_t handle;
	uint32_t pad;
	uint64_t offset;
	uint64_t size;
	uint64_t addr_ptr;
};

struct drm_glamo_gem_wait_rendering {
	uint32_t handle;
};

struct drm_glamo_gem_pin {
	uint32_t handle;
	uint32_t pin_domain;
	uint64_t alignment;
	uint64_t offset;
};

struct drm_glamo_gem_unpin {
	uint32_t handle;
	uint32_t pad;
};

struct drm_glamo_gem_pread {
	/** Handle for the object being read. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to read from */
	uint64_t offset;
	/** Length of data to read */
	uint64_t size;
	/** Pointer to write the data into. */
	uint64_t data_ptr;	/* void *, but pointers are not 32/64 compatible */
};

struct drm_glamo_gem_pwrite {
	/** Handle for the object being written to. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to write to */
	uint64_t offset;
	/** Length of data to write */
	uint64_t size;
	/** Pointer to read the data from. */
	uint64_t data_ptr;	/* void *, but pointers are not 32/64 compatible */
};

#endif
