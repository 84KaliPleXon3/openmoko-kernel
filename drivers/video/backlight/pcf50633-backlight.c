/*
 *  Copyright (C) 2009, Lars-Peter Clausen <lars@metafoo.de>
 *  	PCF50633 backlight device driver
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/mfd/pcf50633/core.h>
#include <linux/mfd/pcf50633/backlight.h>

struct pcf50633_bl {
	struct pcf50633 *pcf;
	struct backlight_device *bl;

	unsigned int brightness;
	unsigned int brightness_limit;
};

/*
 * pcf50633_bl_set_brightness_limit
 *
 * Update the brightness limit for the pc50633 backlight. The actuall brightness
 * will not go above the limit. This is useful to limit power drain for example
 * on low battery.
 *
 * @dev: Pointer to a pcf50633 device
 * @limit: The brightness limit. Valid values are 0-63
 */
int pcf50633_bl_set_brightness_limit(struct pcf50633 *pcf, unsigned int limit)
{
	struct pcf50633_bl *pcf_bl = platform_get_drvdata(pcf->bl_pdev);

	if (!pcf_bl)
		return -ENODEV;

	pcf_bl->brightness_limit = limit & 0x3f;
	backlight_update_status(pcf_bl->bl);

    return 0;
}

static int pcf50633_bl_update_status(struct backlight_device *bl)
{
	struct pcf50633_bl *pcf_bl = bl_get_data(bl);
	unsigned int new_brightness;


	if (bl->props.state & (BL_CORE_SUSPENDED | BL_CORE_FBBLANK) ||
		bl->props.power != FB_BLANK_UNBLANK)
		new_brightness = 0;
	else if (bl->props.brightness < pcf_bl->brightness_limit)
		new_brightness = bl->props.brightness;
	else
		new_brightness = pcf_bl->brightness_limit;


	if (pcf_bl->brightness == new_brightness)
		return 0;

	if (new_brightness) {
		pcf50633_reg_write(pcf_bl->pcf, PCF50633_REG_LEDOUT,
					new_brightness);
		if (!pcf_bl->brightness)
			pcf50633_reg_write(pcf_bl->pcf, PCF50633_REG_LEDENA, 1);
	} else {
		pcf50633_reg_write(pcf_bl->pcf, PCF50633_REG_LEDENA, 0);
	}

	pcf_bl->brightness = new_brightness;

	return 0;
}

static int pcf50633_bl_get_brightness(struct backlight_device *bl)
{
	struct pcf50633_bl *pcf_bl = bl_get_data(bl);
	return pcf_bl->brightness;
}

static struct backlight_ops pcf50633_bl_ops = {
	.get_brightness = pcf50633_bl_get_brightness,
	.update_status	= pcf50633_bl_update_status,
	.options	= BL_CORE_SUSPENDRESUME,
};

static int __devinit pcf50633_bl_probe(struct platform_device *pdev)
{
	int ret;
	struct pcf50633_bl *pcf_bl;
	struct pcf50633_platform_data *pcf50633_data = pdev->dev.parent->platform_data;
	struct pcf50633_bl_platform_data *pdata = pcf50633_data->backlight_data;

	pcf_bl = kzalloc(sizeof(*pcf_bl), GFP_KERNEL);
	if (!pcf_bl)
		return -ENOMEM;

	pcf_bl->pcf = dev_to_pcf50633(pdev->dev.parent);

	pcf_bl->bl = backlight_device_register(pdev->name, &pdev->dev, pcf_bl,
						&pcf50633_bl_ops);

	if (IS_ERR(pcf_bl->bl)) {
		ret = PTR_ERR(pcf_bl->bl);
		goto err_free;
	}

	platform_set_drvdata(pdev, pcf_bl);

	pcf_bl->bl->props.max_brightness = 0x3f;
	pcf_bl->bl->props.power = FB_BLANK_UNBLANK;

	if (pdata) {
		pcf_bl->bl->props.brightness = pdata->default_brightness;
		pcf_bl->brightness_limit = pdata->default_brightness_limit;
	} else {
		pcf_bl->bl->props.brightness = 0x3f;
		pcf_bl->brightness_limit = 0x3f;
	}

	pcf50633_reg_write(pcf_bl->pcf, PCF50633_REG_LEDDIM, pdata->ramp_time);

	/* Should be different from props.brightness, so we don't
	 * update_status early the first time it's called */
	pcf_bl->brightness = pcf_bl->bl->props.brightness + 1;

	backlight_update_status(pcf_bl->bl);

	return 0;

err_free:
	kfree(pcf_bl);

	return ret;
}

static int __devexit pcf50633_bl_remove(struct platform_device *pdev)
{
	struct pcf50633_bl *pcf_bl = platform_get_drvdata(pdev);

	backlight_device_unregister(pcf_bl->bl);

	platform_set_drvdata(pdev, NULL);

	kfree(pcf_bl);

	return 0;
}

static struct platform_driver pcf50633_bl_driver = {
	.probe =	pcf50633_bl_probe,
	.remove =	__devexit_p(pcf50633_bl_remove),
	.driver = {
		.name = "pcf50633-backlight",
	},
};

static int __init pcf50633_bl_init(void)
{
	return platform_driver_register(&pcf50633_bl_driver);
}
module_init(pcf50633_bl_init);

static void __exit pcf50633_bl_exit(void)
{
	platform_driver_unregister(&pcf50633_bl_driver);
}
module_exit(pcf50633_bl_exit);

MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_DESCRIPTION("PCF50633 backlight driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pcf50633-backlight");