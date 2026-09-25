// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation, Inc. All rights reserved. (FIXME)
//
// Samsung S6E3HA3X01 1440x2560 command-mode panel driven by two bonded
// (dual) DSI links, as on the Huawei Nexus 6P (angler).

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct dualmipi0 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi[2];
	struct regulator *supply;
	struct gpio_desc *reset_gpio;
	bool inherit_splash;
};

static inline struct dualmipi0 *to_dualmipi0(struct drm_panel *panel)
{
	return container_of_const(panel, struct dualmipi0, panel);
}

static void dualmipi0_set_lpm(struct dualmipi0 *ctx, bool enable)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (enable)
			ctx->dsi[i]->mode_flags |= MIPI_DSI_MODE_LPM;
		else
			ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;
	}
}

/* Send the same DCS payload over both DSI links of the bonded pair. */
static void dualmipi0_dcs_write_multi(struct dualmipi0 *ctx, int *err,
				      const void *data, size_t len)
{
	int i, ret;

	if (*err)
		return;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		ret = mipi_dsi_dcs_write_buffer(ctx->dsi[i], data, len);
		if (ret < 0) {
			dev_err(&ctx->dsi[i]->dev,
				"failed to tx cmd to dsi [%d], err: %d\n", i, ret);
			*err = ret;
			return;
		}
	}
}

#define dualmipi0_write_seq_multi(ctx, err, seq...)				\
	do {									\
		static const u8 d[] = { seq };					\
		dualmipi0_dcs_write_multi(ctx, err, d, ARRAY_SIZE(d));		\
	} while (0)

static void dualmipi0_dcs_cmd_multi(struct dualmipi0 *ctx, int *err, u8 cmd)
{
	dualmipi0_dcs_write_multi(ctx, err, &cmd, 1);
}

static void dualmipi0_reset(struct dualmipi0 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int dualmipi0_on(struct dualmipi0 *ctx)
{
	int err = 0;

	dualmipi0_set_lpm(ctx, true);

	dualmipi0_dcs_cmd_multi(ctx, &err, MIPI_DCS_EXIT_SLEEP_MODE);
	usleep_range(5000, 6000);

	dualmipi0_write_seq_multi(ctx, &err,
				  MIPI_DCS_SET_COLUMN_ADDRESS,
				  0x00, 0x00, 0x05, 0x9f);
	dualmipi0_write_seq_multi(ctx, &err,
				  MIPI_DCS_SET_PAGE_ADDRESS,
				  0x00, 0x00, 0x09, 0xff);
	dualmipi0_write_seq_multi(ctx, &err, 0xf0, 0x5a, 0x5a);
	dualmipi0_write_seq_multi(ctx, &err, 0xb0, 0x10);
	dualmipi0_write_seq_multi(ctx, &err, 0xb5, 0xa0);
	dualmipi0_write_seq_multi(ctx, &err, 0xc4, 0x03);
	dualmipi0_write_seq_multi(ctx, &err, 0xf6,
				  0x42, 0x57, 0x37, 0x00, 0xaa, 0xcc,
				  0xd0, 0x00, 0x00);
	dualmipi0_write_seq_multi(ctx, &err, 0xf9, 0x03);
	dualmipi0_write_seq_multi(ctx, &err, 0xc2,
				  0x00, 0x00, 0xd8, 0xd8, 0x00, 0x80,
				  0x2b, 0x05, 0x08, 0x0e, 0x07, 0x0b,
				  0x05, 0x0d, 0x0a, 0x15, 0x13, 0x20,
				  0x1e);
	dualmipi0_write_seq_multi(ctx, &err, 0xf0, 0xa5, 0xa5);
	msleep(80);
	dualmipi0_write_seq_multi(ctx, &err, 0x35, 0x00);
	dualmipi0_write_seq_multi(ctx, &err, 0x36, 0x08);
	dualmipi0_write_seq_multi(ctx, &err, 0x53, 0x20);
	dualmipi0_dcs_cmd_multi(ctx, &err, MIPI_DCS_SET_DISPLAY_ON);
	usleep_range(5000, 6000);

	return err;
}

static int dualmipi0_off(struct dualmipi0 *ctx)
{
	int err = 0;

	dualmipi0_set_lpm(ctx, false);

	dualmipi0_dcs_cmd_multi(ctx, &err, MIPI_DCS_SET_DISPLAY_OFF);
	msleep(60);

	dualmipi0_dcs_cmd_multi(ctx, &err, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(180);

	return err;
}

static int dualmipi0_prepare(struct drm_panel *panel)
{
	struct dualmipi0 *ctx = to_dualmipi0(panel);
	struct device *dev = &ctx->dsi[0]->dev;
	int ret;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulator: %d\n", ret);
		return ret;
	}

	/*
	 * Cont-splash inherit (samsung,inherit-splash): the bootloader left
	 * the panel initialized and displaying, so skip reset + init and only
	 * claim it.  Without the property the normal reset/init sequence below
	 * runs instead.
	 */
	if (ctx->inherit_splash)
		return 0;

	dualmipi0_reset(ctx);

	ret = dualmipi0_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_disable(ctx->supply);
		return ret;
	}

	return 0;
}

static int dualmipi0_unprepare(struct drm_panel *panel)
{
	struct dualmipi0 *ctx = to_dualmipi0(panel);
	struct device *dev = &ctx->dsi[0]->dev;
	int ret;

	ret = dualmipi0_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_disable(ctx->supply);

	return 0;
}

/* Full panel width: each of the two bonded DSI links carries 720 columns. */
static const struct drm_display_mode dualmipi0_mode = {
	.clock = (1440 + 100 + 40 + 100) * (2560 + 30 + 8 + 31) * 60 / 1000,
	.hdisplay = 1440,
	.hsync_start = 1440 + 100,
	.hsync_end = 1440 + 100 + 40,
	.htotal = 1440 + 100 + 40 + 100,
	.vdisplay = 2560,
	.vsync_start = 2560 + 30,
	.vsync_end = 2560 + 30 + 8,
	.vtotal = 2560 + 30 + 8 + 31,
	.width_mm = 71,
	.height_mm = 126,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int dualmipi0_get_modes(struct drm_panel *panel,
			       struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &dualmipi0_mode);
}

static const struct drm_panel_funcs dualmipi0_panel_funcs = {
	.prepare = dualmipi0_prepare,
	.unprepare = dualmipi0_unprepare,
	.get_modes = dualmipi0_get_modes,
};

static int dualmipi0_bl_update_status(struct backlight_device *bl)
{
	struct dualmipi0 *ctx = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	const u8 data[] = {
		MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
		brightness & 0xff,
		brightness >> 8,
	};
	int err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++)
		ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;

	dualmipi0_dcs_write_multi(ctx, &err, data, ARRAY_SIZE(data));

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++)
		ctx->dsi[i]->mode_flags |= MIPI_DSI_MODE_LPM;

	return err;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int dualmipi0_bl_get_brightness(struct backlight_device *bl)
{
	struct dualmipi0 *ctx = bl_get_data(bl);
	struct mipi_dsi_device *dsi = ctx->dsi[0];
	u16 brightness = 0;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static const struct backlight_ops dualmipi0_bl_ops = {
	.update_status = dualmipi0_bl_update_status,
	.get_brightness = dualmipi0_bl_get_brightness,
};

static struct backlight_device *
dualmipi0_create_backlight(struct dualmipi0 *ctx)
{
	struct device *dev = &ctx->dsi[0]->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, ctx,
					      &dualmipi0_bl_ops, &props);
}

static int dualmipi0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct dualmipi0 *ctx;
	struct mipi_dsi_device *dsi1_device;
	struct device_node *dsi1_node;
	struct mipi_dsi_host *dsi1_host;
	int ret;
	int i;

	const struct mipi_dsi_device_info info = {
		.type = "dualmipi0",
		.channel = 0,
		.node = NULL,
	};

	ctx = devm_drm_panel_alloc(dev, struct dualmipi0, panel,
				   &dualmipi0_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->supply = devm_regulator_get(dev, "vci");
	if (IS_ERR(ctx->supply))
		return dev_err_probe(dev, PTR_ERR(ctx->supply),
				     "Failed to get vci regulator\n");

	/*
	 * GPIOD_OUT_* selects the logical level, and the reset line is active
	 * low: GPIOD_OUT_HIGH would drive it physical-low (asserted) from probe
	 * onwards, putting a continuously-booted panel back into reset.
	 * GPIOD_OUT_LOW keeps it deasserted, exactly as the bootloader left it.
	 */
	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->inherit_splash = of_property_read_bool(dev->of_node,
						    "samsung,inherit-splash");

	/*
	 * The panel is wired to two DSI controllers. The primary device probes
	 * on the dsi0 host; register a second device on the dsi1 host found via
	 * the panel's second graph port.
	 */
	dsi1_node = of_graph_get_remote_node(dsi->dev.of_node, 1, -1);
	if (!dsi1_node)
		return dev_err_probe(dev, -ENODEV,
				     "Failed to get remote node for dsi1\n");

	dsi1_host = of_find_mipi_dsi_host_by_node(dsi1_node);
	of_node_put(dsi1_node);
	if (!dsi1_host)
		return dev_err_probe(dev, -EPROBE_DEFER,
				     "Failed to find dsi1 host\n");

	dsi1_device = devm_mipi_dsi_device_register_full(dev, dsi1_host, &info);
	if (IS_ERR(dsi1_device))
		return dev_err_probe(dev, PTR_ERR(dsi1_device),
				     "Failed to register dsi1 device\n");

	ctx->dsi[0] = dsi;
	ctx->dsi[1] = dsi1_device;

	mipi_dsi_set_drvdata(dsi, ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		ctx->dsi[i]->lanes = 4;
		ctx->dsi[i]->format = MIPI_DSI_FMT_RGB888;
		ctx->dsi[i]->mode_flags = MIPI_DSI_CLOCK_NON_CONTINUOUS;
	}

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = dualmipi0_create_backlight(ctx);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		ret = mipi_dsi_attach(ctx->dsi[i]);
		if (ret < 0) {
			dev_err(dev, "Failed to attach to DSI host %d: %d\n",
				i, ret);
			drm_panel_remove(&ctx->panel);
			return ret;
		}
	}

	return 0;
}

static void dualmipi0_remove(struct mipi_dsi_device *dsi)
{
	struct dualmipi0 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		ret = mipi_dsi_detach(ctx->dsi[i]);
		if (ret < 0)
			dev_err(&ctx->dsi[i]->dev,
				"Failed to detach from DSI host: %d\n", ret);
	}

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id dualmipi0_of_match[] = {
	{ .compatible = "mdss,dualmipi0" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dualmipi0_of_match);

static struct mipi_dsi_driver dualmipi0_driver = {
	.probe = dualmipi0_probe,
	.remove = dualmipi0_remove,
	.driver = {
		.name = "panel-dualmipi0",
		.of_match_table = dualmipi0_of_match,
	},
};
module_mipi_dsi_driver(dualmipi0_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for SAMSUNG_S6E3HA3X01_5P7_1440P_CMD_DUAL0");
MODULE_LICENSE("GPL");
