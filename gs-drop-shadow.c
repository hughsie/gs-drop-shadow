//gcc -o test test.c `pkg-config --libs --cflags cairo glib-2.0` -lm && ./test

#include <cairo.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <glib.h>

static cairo_surface_t *
gs_cairo_surface_blur (cairo_surface_t *surface, gint radius)
{
	cairo_surface_t *tmp;
	gint width, height;
	gint src_stride, dst_stride;
	gint x, y, z, w;
	guint8 *src, *dst;
	guint32 a = 0;
	guint32 p;
	gint size = (radius * 2) + 1;
	g_autofree guint8 *kernel = g_new0 (guint8, size);

	/* check valid */
	if (cairo_image_surface_get_format (surface) != CAIRO_FORMAT_RGB24 &&
	    cairo_image_surface_get_format (surface) != CAIRO_FORMAT_ARGB32)
		return NULL;

	/* make copy */
	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	tmp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	if (cairo_surface_status (tmp) != CAIRO_STATUS_SUCCESS)
		return NULL;

	/* get data */
	src = cairo_image_surface_get_data (surface);
	src_stride = cairo_image_surface_get_stride (surface);
	dst = cairo_image_surface_get_data (tmp);
	dst_stride = cairo_image_surface_get_stride (tmp);

	/* scale alpha */
	for (gint i = 0; i < size; i++) {
		gdouble f = i - radius;
		a += kernel[i] = exp (-f * f / 30.0) * 80;
	}

	/* horizontal */
	for (gint i = 0; i < height; i++) {
		guint32 *s = (guint32 *) (src + i * src_stride);
		guint32 *d = (guint32 *) (dst + i * dst_stride);
		for (gint j = 0; j < width; j++) {
			x = y = z = w = 0;
			for (gint k = 0; k < size; k++) {
				/* detect edge */
				if (j - radius + k < 0 || j - radius + k >= width)
					continue;
				p = s[j - radius + k];
				x += ((p >> 24) & 0xff) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >>  8) & 0xff) * kernel[k];
				w += ((p >>  0) & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
	}

	/* vertical */
	for (gint i = 0; i < height; i++) {
		guint32 *s;// = (guint32 *) (src + i * dst_stride);
		guint32 *d = (guint32 *) (dst + i * src_stride);
		for (gint j = 0; j < width; j++) {
			x = y = z = w = 0;
			for (gint k = 0; k < size; k++) {
				/* detect edge */
				if (i - radius + k < 0 || i - radius + k >= height)
					continue;
				s = (guint32 *) (dst + (i - radius + k) * dst_stride);
				p = s[j];
				x += ((p >> 24) & 0xff) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >>  8) & 0xff) * kernel[k];
				w += ((p >>  0) & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
	}

	cairo_surface_mark_dirty (tmp);
	return tmp;
}

static cairo_surface_t *
gs_cairo_surface_make_blurred_shadow (cairo_surface_t *surface, gint radius)
{
	cairo_surface_t *tmp;
	gint width, height, stride;
	guint8 *p;

	/* blur into new surface */
	tmp = gs_cairo_surface_blur (surface, radius);
	if (tmp == NULL)
		return NULL;

	/* make black */
	stride = cairo_image_surface_get_stride (tmp);
	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	p = cairo_image_surface_get_data (tmp);
	for (gint i = 0; i < height; i++) {
		guint32 *d = (guint32 *) (p + i * stride);
		for (gint j = 0; j < width; j++)
			d[j] = d[j] & 0xff000000;
	}
	cairo_surface_mark_dirty (tmp);
	return tmp;
}

typedef struct __attribute__((packed)) {
	guint8	r;
	guint8	g;
	guint8	b;
	guint8	a;
} cairo_format_argb_t;

static guint
gs_cairo_surface_get_shadow_pixels (cairo_surface_t *surface)
{
	guint cnt = 0;
	gint width, height, stride;
	guint8 *p;

	/* count pixels that are 100% black, and >50% transparent */
	stride = cairo_image_surface_get_stride (surface);
	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	p = cairo_image_surface_get_data (surface);
	for (gint i = 0; i < height; i++) {
		guint32 *d = (guint32 *) (p + i * stride);
		for (gint j = 0; j < width; j++) {
			cairo_format_argb_t *tmp = (cairo_format_argb_t *) d + j;
			if (tmp->a > 0x00 && tmp->a < 0xff &&
			    tmp->r == 0x00 && tmp->g == 0x00 && tmp->b == 0x00) {
//				g_debug ("R:%02x, G:%02x, B:%02x, A:%02x",
//					 tmp->r, tmp->g, tmp->b, tmp->a);
				cnt++;
			}
		}
	}

	return cnt;
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(cairo_surface_t, cairo_surface_destroy)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(cairo_t, cairo_destroy)

static cairo_surface_t *
gs_cairo_surface_add_drop_shadow (cairo_surface_t *surface_src)
{
	g_autoptr(cairo_surface_t) surface_blur1 = NULL;
	g_autoptr(cairo_surface_t) surface_blur2 = NULL;
	g_autoptr(cairo_surface_t) surface_dst = NULL;
	g_autoptr(cairo_t) cr = NULL;

	/* create new surface to draw into */
	surface_dst = cairo_surface_create_similar (surface_src,
						    CAIRO_CONTENT_COLOR_ALPHA,
						    64, 64);
	cr = cairo_create (surface_dst);

	/* paint shadows */
	surface_blur1 = gs_cairo_surface_make_blurred_shadow (surface_src, 12);
	cairo_set_source_surface (cr, surface_blur1, 0, 2);
	cairo_paint_with_alpha (cr, 0.2);
	surface_blur2 = gs_cairo_surface_make_blurred_shadow (surface_src, 2);
	cairo_set_source_surface (cr, surface_blur2, 0, 1);
	cairo_paint_with_alpha (cr, 0.7);

	/* paint original image on top */
	cairo_set_source_surface (cr, surface_src, 0, 0);
	cairo_paint (cr);
	return g_steal_pointer (&surface_dst);
}

int
main (int argc, char **argv)
{
	const guint shadow_pixels_limit = 50;

	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
	if (argc == 1) {
		g_printerr ("need filename(s) to overwrite\n");
		return 1;
	}
	for (gint i = 1; i < argc; i++) {
		cairo_status_t rc;
		guint shadow_pixels;
		g_autoptr(cairo_surface_t) surface_src = NULL;
		g_autoptr(cairo_surface_t) surface_dst = NULL;

		/* load image */
		surface_src = cairo_image_surface_create_from_png (argv[i]);
		if (surface_src == NULL) {
			g_printerr ("cannot load: %s\n", argv[i]);
			continue;
		}
		shadow_pixels = gs_cairo_surface_get_shadow_pixels (surface_src);
		if (shadow_pixels > shadow_pixels_limit) {
			g_print ("skipping as already has %u shadow pixels: %s\n",
				 shadow_pixels, argv[i]);
			continue;
		}
		surface_dst = gs_cairo_surface_add_drop_shadow (surface_src);
		g_print ("adding drop shadow to %s as has %u shadow pixels\n",
			 argv[i], shadow_pixels);
		rc = cairo_surface_write_to_png (surface_dst, argv[i]);
		if (rc != CAIRO_STATUS_SUCCESS) {
			g_printerr ("failed to write: %s\n", cairo_status_to_string (rc));
		}
	}
	return 0;
}
