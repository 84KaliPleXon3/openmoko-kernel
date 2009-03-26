/* Smedia Glamo 336x/337x driver
 *
 * Copyright (C) 2009 Openmoko, Inc. Jorge Luis Zapata <turran@openmoko.com>
 * Copyright (C) 2008-2009 Thomas White <taw@bitwiz.org.uk>
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <drm/drmP.h>

#include "glamo-core.h"

#define DRIVER_AUTHOR           "Openmoko, Inc."
#define DRIVER_NAME             "glamo-drm"
#define DRIVER_DESC             "SMedia Glamo 3362"
#define DRIVER_DATE             "20090217"

#define RESSIZE(ressource) (((ressource)->end - (ressource)->start)+1)

struct glamodrm_handle {

	/* This device */
	struct device *dev;
	/* The parent device handle */
	struct glamo_core *glamo_core;

	/* MMIO region */
	struct resource *reg;
	char __iomem *base;

	/* VRAM region */
	struct resource *vram;
	char __iomem *vram_base;

	ssize_t vram_size;
};

int glamodrm_firstopen(struct drm_device *dev)
{
	DRM_DEBUG("\n");
	return 0;
}

int glamodrm_open(struct drm_device *dev, struct drm_file *fh)
{
	DRM_DEBUG("\n");
	return 0;
}

void glamodrm_preclose(struct drm_device *dev, struct drm_file *fh)
{
	DRM_DEBUG("\n");
}

void glamodrm_postclose(struct drm_device *dev, struct drm_file *fh)
{
	DRM_DEBUG("\n");
}

void glamodrm_lastclose(struct drm_device *dev)
{
	DRM_DEBUG("\n");
}

static int glamodrm_master_create(struct drm_device *dev, struct drm_master *master)
{
	DRM_DEBUG("\n");

        return 0;
}

static void glamodrm_master_destroy(struct drm_device *dev, struct drm_master *master)
{
	DRM_DEBUG("\n");
}

static struct drm_driver glamodrm_drm_driver = {
	.driver_features = DRIVER_IS_PLATFORM,
	.firstopen = glamodrm_firstopen,
	.open = glamodrm_open,
	.preclose = glamodrm_preclose,
	.postclose = glamodrm_postclose,
	.lastclose = glamodrm_lastclose,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.get_map_ofs = drm_core_get_map_ofs,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.master_create = glamodrm_master_create,
	.master_destroy = glamodrm_master_destroy,
	/* TODO GEM interface
	.gem_init_object = glamodrm_gem_init_object,
	.gem_free_object = glamodrm_gem_free_object,
	.gem_vm_ops = &glamodrm_gem_vm_ops,
	*/
	.fops = {
		.owner = THIS_MODULE,
		.open = drm_open,
		.release = drm_release,
		.ioctl = drm_ioctl,
		.mmap = drm_mmap,
		.poll = drm_poll,
		.fasync = drm_fasync,
	},
	.major = 0,
	.minor = 1,
	.patchlevel = 0,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
};


static int glamodrm_probe(struct platform_device *pdev)
{
	int rc;
	struct glamodrm_handle *glamodrm;

	printk(KERN_INFO "[glamo-drm] SMedia Glamo Direct Rendering Support\n");

	glamodrm = kmalloc(sizeof(*glamodrm), GFP_KERNEL);
	if ( !glamodrm )
		return -ENOMEM;
	platform_set_drvdata(pdev, glamodrm);
	glamodrm->glamo_core = pdev->dev.platform_data;

	/* Find the command queue registers */
	glamodrm->reg = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if ( !glamodrm->reg ) {
		dev_err(&pdev->dev, "Unable to find cmdq registers.\n");
		rc = -ENOENT;
		goto out_free;
	}
	glamodrm->reg = request_mem_region(glamodrm->reg->start,
					  RESSIZE(glamodrm->reg), pdev->name);
	if ( !glamodrm->reg ) {
		dev_err(&pdev->dev, "failed to request MMIO region\n");
		rc = -ENOENT;
		goto out_free;
	}
	glamodrm->base = ioremap(glamodrm->reg->start, RESSIZE(glamodrm->reg));
	if ( !glamodrm->base ) {
		dev_err(&pdev->dev, "failed to ioremap() MMIO memory\n");
		rc = -ENOENT;
		goto out_release_regs;
	}

	/* Find the working VRAM */
	glamodrm->vram = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if ( !glamodrm->vram ) {
		dev_err(&pdev->dev, "Unable to find work VRAM.\n");
		rc = -ENOENT;
		goto out_unmap_regs;
	}
	glamodrm->vram = request_mem_region(glamodrm->vram->start,
					  RESSIZE(glamodrm->vram), pdev->name);
	if ( !glamodrm->vram ) {
		dev_err(&pdev->dev, "failed to request VRAM region\n");
		rc = -ENOENT;
		goto out_unmap_regs;
	}
	glamodrm->vram_base = ioremap(glamodrm->vram->start,
						RESSIZE(glamodrm->vram));
	if ( !glamodrm->vram_base ) {
		dev_err(&pdev->dev, "failed to ioremap() MMIO memory\n");
		rc = -ENOENT;
		goto out_release_vram;
	}

	glamodrm->vram_size = GLAMO_WORK_SIZE;
	printk(KERN_INFO "[glamo-drm] %lli bytes of Glamo RAM to work with\n",
					(long long int)glamodrm->vram_size);

	/* Initialise DRM */
	drm_platform_init(&glamodrm_drm_driver, pdev);

	/* Enable 2D and 3D */
	glamo_engine_enable(glamodrm->glamo_core, GLAMO_ENGINE_3D);
	glamo_engine_reset(glamodrm->glamo_core, GLAMO_ENGINE_3D);
	msleep(5);
	glamo_engine_enable(glamodrm->glamo_core, GLAMO_ENGINE_2D);
	glamo_engine_reset(glamodrm->glamo_core, GLAMO_ENGINE_2D);
	msleep(5);

	return 0;

out_release_vram:
	release_mem_region(glamodrm->vram->start, RESSIZE(glamodrm->vram));
out_unmap_regs:
	iounmap(glamodrm->base);
out_release_regs:
	release_mem_region(glamodrm->reg->start, RESSIZE(glamodrm->reg));
out_free:
	kfree(glamodrm);
	pdev->dev.driver_data = NULL;
	return rc;
}


static int glamodrm_remove(struct platform_device *pdev)
{
	struct glamodrm_handle *glamodrm = platform_get_drvdata(pdev);
	struct glamo_core *glamocore = pdev->dev.platform_data;

	glamo_engine_disable(glamocore, GLAMO_ENGINE_2D);
	glamo_engine_disable(glamocore, GLAMO_ENGINE_3D);

	drm_exit(&glamodrm_drm_driver);

	platform_set_drvdata(pdev, NULL);

	/* Release registers */
	iounmap(glamodrm->base);
	release_mem_region(glamodrm->reg->start, RESSIZE(glamodrm->reg));

	/* Release VRAM */
	iounmap(glamodrm->vram_base);
	release_mem_region(glamodrm->vram->start, RESSIZE(glamodrm->vram));

	kfree(glamodrm);

	return 0;
}

static struct platform_driver glamodrm_driver = {
	.probe          = glamodrm_probe,
	.remove         = glamodrm_remove,
	.driver         = {
		.name   = "glamo-cmdq",
		.owner  = THIS_MODULE,
	},
};

static int __devinit glamodrm_init(void)
{
	return platform_driver_register(&glamodrm_driver);
}

static void __exit glamodrm_exit(void)
{
	platform_driver_unregister(&glamodrm_driver);
}

module_init(glamodrm_init);
module_exit(glamodrm_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

