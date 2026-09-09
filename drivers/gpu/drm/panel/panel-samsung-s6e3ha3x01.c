// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>

#include <video/mipi_display.h>

#include <drm/drm_connector.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct s6e3ha3x01 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi[2];
	struct gpio_desc *reset_gpio;
	const struct s6e3ha3x01_panel_desc *desc;
};

struct s6e3ha3x01_panel_desc {
	unsigned int panel_type;
	void (*init_func)(struct s6e3ha3x01 *ctx, int *err);
	void (*off_func)(struct s6e3ha3x01 *ctx, int *err);
	const struct drm_display_mode *drm_mode;
	unsigned long mode_flags;
	u32 bus_flags;
	u32 width_mm;
	u32 height_mm;
};

enum s6e3ha3x01_panels {
	SAM_PANEL_S6E3HA3X01,
};

static inline struct s6e3ha3x01 *panel_to_s6e3ha3x01(struct drm_panel *panel)
{
	return container_of(panel, struct s6e3ha3x01, panel);
}

static void s6e3ha3x01_dcs_write_buf_multi(struct s6e3ha3x01 *ctx,
					  int *accum_err,
					  const void *data, size_t len)
{
	int i, ret;

	if (*accum_err)
		return;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (!ctx->dsi[i])
			continue;

		ret = mipi_dsi_dcs_write_buffer(ctx->dsi[i], data, len);
		if (ret < 0) {
			dev_err(&ctx->dsi[0]->dev,
				"failed to tx cmd to dsi [%d], err: %d\n", i, ret);
			*accum_err = ret;
			return;
		}
	}
}

#define s6e3ha3x01_write_seq_multi(ctx, accum_err, seq...)			\
	do {									\
		static const u8 d[] = { seq };					\
		s6e3ha3x01_dcs_write_buf_multi(ctx, accum_err, d, ARRAY_SIZE(d));\
	} while (0)

static void s6e3ha3x01_dcs_cmd_multi(struct s6e3ha3x01 *ctx, int *accum_err, u8 cmd)
{
	s6e3ha3x01_dcs_write_buf_multi(ctx, accum_err, &cmd, 1);
}

static void s6e3ha3x01_reset(struct s6e3ha3x01 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

/* Initialization and off routines for Samsung S6E3HA3X01 panel */

static void s6e3ha3x01_panel_init(struct s6e3ha3x01 *ctx, int *err)
{
	s6e3ha3x01_dcs_cmd_multi(ctx, err, MIPI_DCS_EXIT_SLEEP_MODE);
	usleep_range(5000, 6000);

	s6e3ha3x01_write_seq_multi(ctx, err, MIPI_DCS_SET_COLUMN_ADDRESS,
				  0x00, 0x00, 0x05, 0x9f);
	s6e3ha3x01_write_seq_multi(ctx, err, MIPI_DCS_SET_PAGE_ADDRESS,
				  0x00, 0x00, 0x09, 0xff);

	s6e3ha3x01_write_seq_multi(ctx, err, 0xf0, 0x5a, 0x5a);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xb0, 0x10);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xb5, 0xa0);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xfc, 0x5a, 0x5a);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xb0, 0x28);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xd7, 0x4b);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xfe, 0xb1);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xfe, 0x31);
	usleep_range(5000, 6000);
	
	s6e3ha3x01_write_seq_multi(ctx, err, 0xc4, 0x03);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xf9, 0x03);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xc2,
				  0x00, 0x00, 0xd8, 0xd8, 0x00, 0x80,
				  0x2b, 0x05, 0x08, 0x0e, 0x07, 0x0b,
				  0x05, 0x0d, 0x0a, 0x15, 0x13, 0x20,
				  0x1e);
	s6e3ha3x01_write_seq_multi(ctx, err, 0xf0, 0xa5, 0xa5);
	msleep(80);
	s6e3ha3x01_write_seq_multi(ctx, err, 0x35, 0x00);
	s6e3ha3x01_write_seq_multi(ctx, err, 0x36, 0x08);
	s6e3ha3x01_write_seq_multi(ctx, err, 0x53, 0x20);

	s6e3ha3x01_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_ON);
	usleep_range(5000, 6000);
}

static void s6e3ha3x01_panel_off(struct s6e3ha3x01 *ctx, int *err)
{
	s6e3ha3x01_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_OFF);
	msleep(60);

	s6e3ha3x01_dcs_cmd_multi(ctx, err, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(180);
}

/* Panel display mode */

static const struct drm_display_mode s6e3ha3x01_mode = {
	.clock = (1440 + 100 + 40 + 100) * (2560 + 30 + 8 + 31) * 60 / 1000,
	.hdisplay = 1440,
	.hsync_start = 1440 + 100,
	.hsync_end = 1440 + 100 + 40,
	.htotal = 1440 + 100 + 40 + 100,
	.vdisplay = 2560,
	.vsync_start = 2560 + 30,
	.vsync_end = 2560 + 30 + 8,
	.vtotal = 2560 + 30 + 8 + 31,
};

/* Panel Descriptors */

static const struct s6e3ha3x01_panel_desc s6e3ha3x01_desc = {
	.panel_type = SAM_PANEL_S6E3HA3X01,
	.init_func = s6e3ha3x01_panel_init,
	.off_func = s6e3ha3x01_panel_off,
	.drm_mode = &s6e3ha3x01_mode,
	.mode_flags = MIPI_DSI_MODE_NO_EOT_PACKET |
		      MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.bus_flags = 0,
	.width_mm = 68,
	.height_mm = 121,
};

/* Power management routines */

static int s6e3ha3x01_on(struct s6e3ha3x01 *ctx)
{
	int accum_err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags |= MIPI_DSI_MODE_LPM;
	}

	ctx->desc->init_func(ctx, &accum_err);

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;
	}

	return accum_err;
}

static void s6e3ha3x01_off(struct s6e3ha3x01 *ctx)
{
	int accum_err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;
	}

	ctx->desc->off_func(ctx, &accum_err);
}

static int s6e3ha3x01_prepare(struct drm_panel *panel)
{
	struct s6e3ha3x01 *ctx = panel_to_s6e3ha3x01(panel);
	int ret;

	s6e3ha3x01_reset(ctx);

	ret = s6e3ha3x01_on(ctx);
	if (ret < 0) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	return 0;
}

static int s6e3ha3x01_disable(struct drm_panel *panel)
{
	struct s6e3ha3x01 *ctx = panel_to_s6e3ha3x01(panel);

	s6e3ha3x01_off(ctx);

	return 0;
}

static int s6e3ha3x01_unprepare(struct drm_panel *panel)
{
	struct s6e3ha3x01 *ctx = panel_to_s6e3ha3x01(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static int s6e3ha3x01_get_modes(struct drm_panel *panel,
			       struct drm_connector *connector)
{
	struct s6e3ha3x01 *ctx = panel_to_s6e3ha3x01(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, ctx->desc->drm_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = ctx->desc->width_mm;
	connector->display_info.height_mm = ctx->desc->height_mm;
	connector->display_info.bus_flags = ctx->desc->bus_flags;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs s6e3ha3x01_panel_funcs = {
	.disable = s6e3ha3x01_disable,
	.prepare = s6e3ha3x01_prepare,
	.unprepare = s6e3ha3x01_unprepare,
	.get_modes = s6e3ha3x01_get_modes,
};

static int s6e3ha3x01_bl_update_status(struct backlight_device *bl)
{
	struct s6e3ha3x01 *ctx = bl_get_data(bl);
	u8 brightness = backlight_get_brightness(bl);
	int accum_err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++)
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;

	s6e3ha3x01_dcs_write_buf_multi(ctx, &accum_err,
				      &(u8[]){ MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
				      brightness }, 2);

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++)
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags |= MIPI_DSI_MODE_LPM;

	return accum_err;
}

static const struct backlight_ops s6e3ha3x01_bl_ops = {
	.update_status = s6e3ha3x01_bl_update_status,
};

static struct backlight_device *
s6e3ha3x01_create_backlight(struct s6e3ha3x01 *ctx)
{
	struct device *dev = &ctx->dsi[0]->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, ctx,
					      &s6e3ha3x01_bl_ops, &props);
}

static int s6e3ha3x01_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct s6e3ha3x01 *ctx;
	struct mipi_dsi_device *dsi1_device;
	struct device_node *dsi1;
	struct mipi_dsi_host *dsi1_host;
	struct mipi_dsi_device *dsi_dev;
	int ret = 0;
	int i;

	const struct mipi_dsi_device_info info = {
		.type = "s6e3ha3x01",
		.channel = 0,
		.node = NULL,
	};

	ctx = devm_drm_panel_alloc(dev, struct s6e3ha3x01, panel,
				   &s6e3ha3x01_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->desc = of_device_get_match_data(dev);
	if (!ctx->desc)
		return -ENODEV;

	dsi1 = of_graph_get_remote_node(dev->of_node, 1, -1);
	if (!dsi1) {
		dev_err(dev, "failed to get remote node for dsi1\n");
		return -ENODEV;
	}

	dsi1_host = of_find_mipi_dsi_host_by_node(dsi1);
	of_node_put(dsi1);
	if (!dsi1_host)
		return dev_err_probe(dev, -EPROBE_DEFER, "failed to find dsi1 host\n");

	dsi1_device = devm_mipi_dsi_device_register_full(dev, dsi1_host, &info);
	if (IS_ERR(dsi1_device))
		return dev_err_probe(dev, PTR_ERR(dsi1_device),
				     "failed to register dsi1 device\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dsi[0] = dsi;
	ctx->dsi[1] = dsi1_device;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		dsi_dev = ctx->dsi[i];
		dsi_dev->lanes = 4;
		dsi_dev->format = MIPI_DSI_FMT_RGB888;
		dsi_dev->mode_flags = MIPI_DSI_MODE_LPM | ctx->desc->mode_flags;
	}

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = s6e3ha3x01_create_backlight(ctx);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		ret = mipi_dsi_attach(ctx->dsi[i]);
		if (ret < 0) {
			dev_err(dev, "failed to attach dsi [%d]: %d\n", i, ret);
			drm_panel_remove(&ctx->panel);
			return ret;
		}
	}

	return 0;
}

static void s6e3ha3x01_remove(struct mipi_dsi_device *dsi)
{
	struct s6e3ha3x01 *ctx = mipi_dsi_get_drvdata(dsi);
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			mipi_dsi_detach(ctx->dsi[i]);
	}

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id s6e3ha3x01_of_match[] = {
	{ 
		.compatible = "mdss,s6e3ha3x01",
		.data = &s6e3ha3x01_desc,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, s6e3ha3x01_of_match);

static struct mipi_dsi_driver s6e3ha3x01_driver = {
	.probe = s6e3ha3x01_probe,
	.remove = s6e3ha3x01_remove,
	.driver = {
		.name = "panel-s6e3ha3x01",
		.of_match_table = s6e3ha3x01_of_match,
	},
};
module_mipi_dsi_driver(s6e3ha3x01_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>");
MODULE_DESCRIPTION("DRM driver for SAMSUNG_S6E3HA3X01_5P7_1440P_CMD_DUAL0");
MODULE_LICENSE("GPL");