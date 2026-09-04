// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung AMS567DJ06 / AMS5208R08 dual-DSI 1440p command-mode drm_panel driver.
 *
 * Modeled after Samsung S6D7AA0 panel driver pattern using panel descriptors,
 * explicit per-panel init/off callbacks, and match data[cite: 4].
 */

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

struct samsung_ams {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi[2];
	struct gpio_desc *reset_gpio;
	const struct samsung_ams_panel_desc *desc;
};

struct samsung_ams_panel_desc {
	unsigned int panel_type;
	void (*init_func)(struct samsung_ams *ctx, int *err);
	void (*off_func)(struct samsung_ams *ctx, int *err);
	const struct drm_display_mode *drm_mode;
	unsigned long mode_flags;
	u32 bus_flags;
	u32 width_mm;
	u32 height_mm;
};

enum samsung_ams_panels {
	SAM_PANEL_AMS567DJ06,
	SAM_PANEL_AMS5208R08,
};

static inline struct samsung_ams *panel_to_samsung_ams(struct drm_panel *panel)
{
	return container_of(panel, struct samsung_ams, panel);
}

static void ams_dcs_write_buf_multi(struct samsung_ams *ctx,
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
			dev_err(&ctx->dsi[0]->dev, "failed to tx cmd to dsi [%d], err: %d\n", i, ret);
			*accum_err = ret;
			return;
		}
	}
}

#define ams_write_seq_multi(ctx, accum_err, seq...)				\
	do {									\
		static const u8 d[] = { seq };					\
		ams_dcs_write_buf_multi(ctx, accum_err, d, ARRAY_SIZE(d));	\
	} while (0)

static void ams_dcs_cmd_multi(struct samsung_ams *ctx, int *accum_err, u8 cmd)
{
	ams_dcs_write_buf_multi(ctx, accum_err, &cmd, 1);
}

static void samsung_ams_reset(struct samsung_ams *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

/* Initialization and off routines for Samsung AMS567DJ06 panel */

static void ams567dj06_panel_init(struct samsung_ams *ctx, int *err)
{
	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_EXIT_SLEEP_MODE);
	msleep(5);

	ams_write_seq_multi(ctx, err, 0xf0, 0x5a, 0x5a);
	ams_write_seq_multi(ctx, err, 0xf2, 0x63);
	ams_write_seq_multi(ctx, err, 0xbd, 0x11, 0x01, 0x02, 0x16, 0x02, 0x16);
	ams_write_seq_multi(ctx, err, 0xf0, 0xa5, 0xa5);
	msleep(23);
	ams_write_seq_multi(ctx, err, 0x53, 0x20);
	ams_write_seq_multi(ctx, err, 0x51, 0x80);
	ams_write_seq_multi(ctx, err, 0x35, 0x00);

	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_ON);
}

static void ams567dj06_panel_off(struct samsung_ams *ctx, int *err)
{
	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_OFF);
	msleep(33);

	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(120);
}

/* Initialization and off routines for Samsung AMS5208R08 panel */

static void ams5208r08_panel_init(struct samsung_ams *ctx, int *err)
{
	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_EXIT_SLEEP_MODE);
	msleep(120);

	ams_write_seq_multi(ctx, err, 0xf0, 0x5a, 0x5a);
	ams_write_seq_multi(ctx, err, 0xbd, 0x05, 0x02, 0x16);
	ams_write_seq_multi(ctx, err, 0xff, 0x02);
	ams_write_seq_multi(ctx, err, 0xf0, 0xa5, 0xa5);
	ams_write_seq_multi(ctx, err, 0x53, 0x20);
	ams_write_seq_multi(ctx, err, 0x51, 0x80);
	ams_write_seq_multi(ctx, err, 0x35, 0x00);

	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_ON);
}

static void ams5208r08_panel_off(struct samsung_ams *ctx, int *err)
{
	ams_dcs_cmd_multi(ctx, err, MIPI_DCS_SET_DISPLAY_OFF);
	msleep(33);
}

/* Panel display mode */

static const struct drm_display_mode samsung_ams_mode = {
	.clock = 240981,
	.hdisplay = 1440,
	.hsync_start = 1440 + 76,
	.hsync_end = 1440 + 76 + 16,
	.htotal = 1440 + 76 + 16 + 32,
	.vdisplay = 2560,
	.vsync_start = 2560 + 4,
	.vsync_end = 2560 + 4 + 2,
	.vtotal = 2560 + 4 + 2 + 2,
};

/* Panel Descriptors */

static const struct samsung_ams_panel_desc ams567dj06_desc = {
	.panel_type = SAM_PANEL_AMS567DJ06,
	.init_func = ams567dj06_panel_init,
	.off_func = ams567dj06_panel_off,
	.drm_mode = &samsung_ams_mode,
	.mode_flags = MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.bus_flags = 0,
	.width_mm = 71,
	.height_mm = 125,
};

static const struct samsung_ams_panel_desc ams5208r08_desc = {
	.panel_type = SAM_PANEL_AMS5208R08,
	.init_func = ams5208r08_panel_init,
	.off_func = ams5208r08_panel_off,
	.drm_mode = &samsung_ams_mode,
	.mode_flags = MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.bus_flags = 0,
	.width_mm = 65,
	.height_mm = 115,
};

/* Power management routines */

static int samsung_ams_on(struct samsung_ams *ctx)
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

static void samsung_ams_off(struct samsung_ams *ctx)
{
	int accum_err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			ctx->dsi[i]->mode_flags &= ~MIPI_DSI_MODE_LPM;
	}

	ctx->desc->off_func(ctx, &accum_err);
}

static int samsung_ams_prepare(struct drm_panel *panel)
{
	struct samsung_ams *ctx = panel_to_samsung_ams(panel);
	int ret;

	samsung_ams_reset(ctx);

	ret = samsung_ams_on(ctx);
	if (ret < 0) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	return 0;
}

static int samsung_ams_disable(struct drm_panel *panel)
{
	struct samsung_ams *ctx = panel_to_samsung_ams(panel);

	samsung_ams_off(ctx);

	return 0;
}

static int samsung_ams_unprepare(struct drm_panel *panel)
{
	struct samsung_ams *ctx = panel_to_samsung_ams(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static int samsung_ams_get_modes(struct drm_panel *panel,
				  struct drm_connector *connector)
{
	struct samsung_ams *ctx = panel_to_samsung_ams(panel);
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

static const struct drm_panel_funcs samsung_ams_panel_funcs = {
	.disable = samsung_ams_disable,
	.prepare = samsung_ams_prepare,
	.unprepare = samsung_ams_unprepare,
	.get_modes = samsung_ams_get_modes,
};

static int samsung_ams_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct samsung_ams *ctx;
	struct mipi_dsi_device *dsi1_device;
	struct device_node *dsi1;
	struct mipi_dsi_host *dsi1_host;
	struct mipi_dsi_device *dsi_dev;
	int ret = 0;
	int i;

	const struct mipi_dsi_device_info info = {
		.type = "ams567dj06",
		.channel = 0,
		.node = NULL,
	};

	ctx = devm_drm_panel_alloc(dev, struct samsung_ams, panel,
				   &samsung_ams_panel_funcs,
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
		return dev_err_probe(dev, PTR_ERR(dsi1_device), "failed to register dsi1 device\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio), "failed to get reset-gpio\n");

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

static void samsung_ams_remove(struct mipi_dsi_device *dsi)
{
	struct samsung_ams *ctx = mipi_dsi_get_drvdata(dsi);
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->dsi); i++) {
		if (ctx->dsi[i])
			mipi_dsi_detach(ctx->dsi[i]);
	}

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id samsung_ams_of_match[] = {
	{
		.compatible = "samsung,ams567dj06",
		.data = &ams567dj06_desc,
	},
	{
		.compatible = "samsung,ams5208r08",
		.data = &ams5208r08_desc,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, samsung_ams_of_match);

static struct mipi_dsi_driver samsung_ams_driver = {
	.probe = samsung_ams_probe,
	.remove = samsung_ams_remove,
	.driver = {
		.name = "panel-samsung-ams",
		.of_match_table = samsung_ams_of_match,
	},
};
module_mipi_dsi_driver(samsung_ams_driver);

MODULE_AUTHOR("Artur Weber <aweber.kernel@gmail.com>");
MODULE_DESCRIPTION("Samsung AMS567DJ06 / AMS5208R08 dual-DSI LCD controller driver");
MODULE_LICENSE("GPL");