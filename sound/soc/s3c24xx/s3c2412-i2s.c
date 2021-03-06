/* sound/soc/s3c24xx/s3c2412-i2s.c
 *
 * ALSA Soc Audio Layer - S3C2412 I2S driver
 *
 * Copyright (c) 2006 Wolfson Microelectronics PLC.
 *	Graeme Gregory graeme.gregory@wolfsonmicro.com
 *	linux@wolfsonmicro.com
 *
 * Copyright (c) 2007, 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <mach/hardware.h>

#include <plat/regs-s3c2412-iis.h>

#include <plat/regs-gpio.h>
#include <plat/audio.h>
#include <mach/dma.h>

#include "s3c24xx-pcm.h"
#include "s3c2412-i2s.h"

#define S3C2412_I2S_DEBUG 0

#if S3C2412_I2S_DEBUG
#define DBG(x...) printk(KERN_INFO x)
#else
#define DBG(x...) do { } while (0)
#endif

static struct s3c2410_dma_client s3c2412_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c2412_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct s3c24xx_pcm_dma_params s3c2412_i2s_pcm_stereo_out = {
	.client		= &s3c2412_dma_client_out,
	.channel	= DMACH_I2S_OUT,
	.dma_addr	= S3C2410_PA_IIS + S3C2412_IISTXD,
	.dma_size	= 4,
};

static struct s3c24xx_pcm_dma_params s3c2412_i2s_pcm_stereo_in = {
	.client		= &s3c2412_dma_client_in,
	.channel	= DMACH_I2S_IN,
	.dma_addr	= S3C2410_PA_IIS + S3C2412_IISRXD,
	.dma_size	= 4,
};

static struct s3c_i2sv2_info s3c2412_i2s;

/*
 * Set S3C2412 Clock source
 */
static int s3c2412_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
				  int clk_id, unsigned int freq, int dir)
{
	u32 iismod = readl(s3c2412_i2s.regs + S3C2412_IISMOD);

	DBG("%s(%p, %d, %u, %d)\n", __func__, cpu_dai, clk_id,
	    freq, dir);

	switch (clk_id) {
	case S3C2412_CLKSRC_PCLK:
		s3c2412_i2s.master = 1;
		iismod &= ~S3C2412_IISMOD_MASTER_MASK;
		iismod |= S3C2412_IISMOD_MASTER_INTERNAL;
		break;
	case S3C2412_CLKSRC_I2SCLK:
		s3c2412_i2s.master = 0;
		iismod &= ~S3C2412_IISMOD_MASTER_MASK;
		iismod |= S3C2412_IISMOD_MASTER_EXTERNAL;
		break;
	default:
		return -EINVAL;
	}

	writel(iismod, s3c2412_i2s.regs + S3C2412_IISMOD);
	return 0;
}


struct clk *s3c2412_get_iisclk(void)
{
	return s3c2412_i2s.iis_clk;
}
EXPORT_SYMBOL_GPL(s3c2412_get_iisclk);


static int s3c2412_i2s_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	int ret;

	DBG("Entered %s\n", __func__);

	ret = s3c_i2sv2_probe(pdev, dai, &s3c2412_i2s, S3C2410_PA_IIS);
	if (ret)
		return ret;

	s3c2412_i2s.dma_capture = &s3c2412_i2s_pcm_stereo_in;
	s3c2412_i2s.dma_playback = &s3c2412_i2s_pcm_stereo_out;

	s3c2412_i2s.iis_cclk = clk_get(&pdev->dev, "i2sclk");
	if (s3c2412_i2s.iis_cclk == NULL) {
		DBG("failed to get i2sclk clock\n");
		iounmap(s3c2412_i2s.regs);
		return -ENODEV;
	}

	/* Set MPLL as the source for IIS CLK */

	clk_set_parent(s3c2412_i2s.iis_cclk, clk_get(NULL, "mpll"));
	clk_enable(s3c2412_i2s.iis_cclk);

	s3c2412_i2s.iis_cclk = s3c2412_i2s.iis_pclk;

	/* Configure the I2S pins in correct mode */
	s3c2410_gpio_cfgpin(S3C2410_GPE0, S3C2410_GPE0_I2SLRCK);
	s3c2410_gpio_cfgpin(S3C2410_GPE1, S3C2410_GPE1_I2SSCLK);
	s3c2410_gpio_cfgpin(S3C2410_GPE2, S3C2410_GPE2_CDCLK);
	s3c2410_gpio_cfgpin(S3C2410_GPE3, S3C2410_GPE3_I2SSDI);
	s3c2410_gpio_cfgpin(S3C2410_GPE4, S3C2410_GPE4_I2SSDO);

	return 0;
}

#define S3C2412_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

struct snd_soc_dai s3c2412_i2s_dai = {
	.name		= "s3c2412-i2s",
	.id		= 0,
	.probe		= s3c2412_i2s_probe,
	.playback = {
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= S3C2412_I2S_RATES,
		.formats	= SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= S3C2412_I2S_RATES,
		.formats	= SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = {
		.set_sysclk	= s3c2412_i2s_set_sysclk,
	},
};
EXPORT_SYMBOL_GPL(s3c2412_i2s_dai);

static int __init s3c2412_i2s_init(void)
{
	return  s3c_i2sv2_register_dai(&s3c2412_i2s_dai);
}
module_init(s3c2412_i2s_init);

static void __exit s3c2412_i2s_exit(void)
{
	snd_soc_unregister_dai(&s3c2412_i2s_dai);
}
module_exit(s3c2412_i2s_exit);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("S3C2412 I2S SoC Interface");
MODULE_LICENSE("GPL");
