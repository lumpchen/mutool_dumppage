#include "mupdf/fitz.h"

typedef struct fz_dumped_path_s fz_dumped_path;
/* fz_path_command_e
	FZ_MOVETO = 'M',
	FZ_LINETO = 'L',
	FZ_DEGENLINETO = 'D',
	FZ_CURVETO = 'C',
	FZ_CURVETOV = 'V',
	FZ_CURVETOY = 'Y',
	FZ_HORIZTO = 'H',
	FZ_VERTTO = 'I',
	FZ_QUADTO = 'Q',
	FZ_RECTTO = 'R',
	FZ_MOVETOCLOSE = 'm',
	FZ_LINETOCLOSE = 'l',
	FZ_DEGENLINETOCLOSE = 'd',
	FZ_CURVETOCLOSE = 'c',
	FZ_CURVETOVCLOSE = 'v',
	FZ_CURVETOYCLOSE = 'y',
	FZ_HORIZTOCLOSE = 'h',
	FZ_VERTTOCLOSE = 'i',
	FZ_QUADTOCLOSE = 'q',
*/
struct fz_dumped_path_s
{
	int8_t refs;
	uint8_t packed;
	int cmd_len, cmd_cap;
	unsigned char *cmds;
	int coord_len, coord_cap;
	float *coords;
	fz_point current;
	fz_point begin;
};

static int
fz_get_str_trim_size(char *str, int size)
{
	int len = 0;
	while (str[len])
	{
		len++;
		if (len == size - 1)
			break;
	}
	return len;
}

static void 
fz_dump_colorspace_attr(FILE *fp, fz_colorspace *colorspace)
{
	if (!colorspace)
	{
		return;
	}
	fprintf(fp, "colorspace=\"");
	fwrite(colorspace->name, sizeof(char), fz_get_str_trim_size(colorspace->name, 16), fp);
	fprintf(fp, "\"");
}

static void 
fz_dump_colorspace(FILE *fp, fz_colorspace *colorspace, float *color, float alpha)
{
	int i = 0;
	if (!colorspace)
	{
		return;
	}
	fprintf(fp, "<color colorspace=\"");
	fwrite(colorspace->name, sizeof(char), fz_get_str_trim_size(colorspace->name, 16), fp);
	fprintf(fp, "\"");
	if (color)
	{
		fprintf(fp, " value=\"");
		for (i = 0; i < colorspace->n; i++)
		{
			if (i != 0)
			{
				fprintf(fp, " ");
			}
			fprintf(fp, "%f", color[i]);
		}
		fprintf(fp, "\"");
	}

	fprintf(fp, " alpha=\"%f\"", alpha);
	fprintf(fp, "/>\r\n");
}

static void 
fz_dump_ctm_attr(FILE *fp, const fz_matrix *ctm)
{
	if (!ctm)
	{
		return;
	}
	fprintf(fp, "ctm=\"%f %f %f %f %f %f\"", ctm->a, ctm->b, ctm->c, ctm->d, ctm->e, ctm->f);
}

static void
fz_dump_path(FILE *fp, void *fz_path)
{
	int i = 0;
	fz_dumped_path *path = (fz_dumped_path *)fz_path;
	if (!path)
		return;
	fprintf(fp, "<path packed=\"%c\">\r\n", path->packed ? '0' : '1');
	fprintf(fp, "<begin x=\"%f\" y=\"%f\"/>\r\n", path->begin.x, path->begin.y);
	fprintf(fp, "<current x=\"%f\" y=\"%f\"/>\r\n", path->current.x, path->current.y);
	
	if (path->cmds)
	{
		fprintf(fp, "<cmds size=\"%d\" cap=\"%d\" ", path->cmd_len, path->cmd_cap);	
		fprintf(fp, " cmd=\"");
		for (i = 0; i < path->cmd_len; i++)
		{
			if (i != 0)
			{
				fprintf(fp, " ");
			}
			fprintf(fp, "%c", path->cmds[i]);
		}
		fprintf(fp, "\"");
		fprintf(fp, "/>\r\n");
	}

	if (path->coords)
	{
		fprintf(fp, "<coords size=\"%d\" cap=\"%d\" ", path->coord_len, path->coord_cap);	
		fprintf(fp, " coord=\"");
		for (i = 0; i < path->coord_len; i++)
		{
			if (i != 0)
			{
				fprintf(fp, " ");
			}
			fprintf(fp, "%f", path->coords[i]);
		}
		fprintf(fp, "\"");
		fprintf(fp, "/>\r\n");
	}

	fprintf(fp, "</path>\r\n");
}

static const char *LINE_CAP[] = {"BUTT", "ROUND", "SQUARE", "TRIANGLE"};
static const char *LINE_JOIN[] = {"MITTER", "ROUND", "BEVEL", "MITTER_XPS"};

static void
fz_dump_stroke_state(FILE *fp, const fz_stroke_state *stroke)
{
	int i = 0;
	if (!stroke)
	{
		return;
	}
	
	fprintf(fp, "<stroke_state");
	fprintf(fp, " refs=\"%d\"", stroke->refs);
	fprintf(fp, " start_cap=\"%s\"", LINE_CAP[stroke->start_cap]);
	fprintf(fp, " dash_cap=\"%s\"", LINE_CAP[stroke->dash_cap]);
	fprintf(fp, " end_cap=\"%s\"", LINE_CAP[stroke->end_cap]);
	fprintf(fp, " linejoin=\"%s\"", LINE_JOIN[stroke->linejoin]);

	fprintf(fp, " linewidth=\"%f\"", stroke->linewidth);
	fprintf(fp, " miterlimit=\"%f\"", stroke->miterlimit);
	fprintf(fp, " dash_phase=\"%f\"", stroke->dash_phase);
	
	if (stroke->dash_len > 0)
	{
		fprintf(fp, " dash_len=\"%d\"", stroke->dash_len);
		fprintf(fp, " dash_list=\"");
		for (i = 0; i < stroke->dash_len; i++)
		{
			fprintf(fp, " %f", stroke->dash_list[i]);
		}
		fprintf(fp, "\"");
	}

	fprintf(fp, "/>\r\n");
}


fz_dumped_page *fz_new_dumped_page(fz_context *ctx, int page_number, char *filename)
{
	fz_dumped_page *dumped_page = fz_malloc(ctx, sizeof(fz_dumped_page));

	FILE *file = fopen(filename, "wb");
	if (!file)
	{
		fz_throw(ctx, FZ_ERROR_GENERIC, "cannot open file '%s': %s", filename, strerror(errno));
	}
	dumped_page->fp = file;
	dumped_page->fz_begin_dump_fill_text = fz_begin_dump_fill_text;
	dumped_page->fz_end_dump_fill_text = fz_end_dump_fill_text;
	dumped_page->fz_begin_dump_stroke_text = fz_begin_dump_stroke_text;
	dumped_page->fz_end_dump_stroke_text = fz_end_dump_stroke_text;
	dumped_page->fz_dump_char = fz_dump_char;
	dumped_page->fz_begin_dump_fill_image = fz_begin_dump_fill_image;
	dumped_page->fz_end_dump_fill_image = fz_end_dump_fill_image;
	dumped_page->fz_begin_dump_stroke_path = fz_begin_dump_stroke_path;
	dumped_page->fz_end_dump_stroke_path = fz_end_dump_stroke_path;
	dumped_page->fz_begin_dump_fill_path = fz_begin_dump_fill_path;
	dumped_page->fz_end_dump_fill_path = fz_end_dump_fill_path;

	fprintf(dumped_page->fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n");
	fprintf(dumped_page->fp, "<page page_number=\"%d\">\r\n", page_number);
	return dumped_page;
}

void 
fz_drop_dumped_page(fz_context *ctx, fz_dumped_page *dumped_page)
{
	if (dumped_page->fp)
	{
		fprintf(dumped_page->fp, "</page>\r\n");
		fclose(dumped_page->fp);
	}
	fz_free(ctx, dumped_page);
}

void 
fz_begin_dump_fill_text(fz_context *ctx, fz_dumped_page *dumped_page, fz_text *text, const fz_matrix *ctm,
	fz_colorspace *colorspace, float *color, float alpha)
{
	int i = 0;
	if (text->len > 0)
	{
		fprintf(dumped_page->fp, "<text_object cmd=\"fill text\" size=\"%d\" ", text->len);
		fz_dump_ctm_attr(dumped_page->fp, &text->trm);
		fprintf(dumped_page->fp, ">\r\n");
	}
	else 
	{
		return;
	}

	if (text->font)
	{
		if (text->font->name)
		{
			fprintf(dumped_page->fp, "<font name=\"");

			fwrite(text->font->name, sizeof(char), fz_get_str_trim_size(text->font->name, 32), dumped_page->fp);
			fprintf(dumped_page->fp, "\"");
			fprintf(dumped_page->fp, "/>\r\n");
		}
	}

	if (colorspace)
	{
		fz_dump_colorspace(dumped_page->fp, colorspace, color, alpha);
	}
}

void
fz_end_dump_fill_text(fz_context *ctx, fz_dumped_page *dumped_page)
{
	fprintf(dumped_page->fp, "</text_object>\r\n");
}

void
fz_dump_char(fz_context *ctx, fz_dumped_page *dumped_page, int x, int y, int ucs, int gid)
{
	fprintf(dumped_page->fp, "<char x=\"%d\" y=\"%d\" ucs=\"%d\" gid=\"%d\"/>\r\n", x, y, ucs, gid);
}

void
fz_begin_dump_stroke_text(fz_context *ctx, fz_dumped_page *dumped_page, fz_text *text, fz_stroke_state *stroke,
	const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha)
{
	int i = 0;
	if (text->len > 0)
	{
		fprintf(dumped_page->fp, "<text_object cmd=\"stroke text\" size=\"%d\" ", text->len);
		fz_dump_ctm_attr(dumped_page->fp, &text->trm);
		fprintf(dumped_page->fp, ">\r\n");
	}
	else 
	{
		return;
	}

	if (text->font)
	{
		if (text->font->name)
		{
			fprintf(dumped_page->fp, "<font name=\"");

			fwrite(text->font->name, sizeof(char), fz_get_str_trim_size(text->font->name, 32), dumped_page->fp);
			fprintf(dumped_page->fp, "\"");
			fprintf(dumped_page->fp, "/>\r\n");
		}
	}

	if (colorspace)
	{
		fz_dump_colorspace(dumped_page->fp, colorspace, color, alpha);
	}

	if (stroke)
	{
		fz_dump_stroke_state(dumped_page->fp, stroke);
	}
}

void 
fz_end_dump_stroke_text(fz_context *ctx, fz_dumped_page *dumped_page)
{
	fprintf(dumped_page->fp, "</text_object>\r\n");
}

void 
fz_begin_dump_fill_image(fz_context *ctx, fz_dumped_page *dumped_page, fz_pixmap *pixmap, 
	fz_image *image, fz_matrix *local_ctm)
{
	if (!pixmap)
	{
		return;
	}
	fprintf(dumped_page->fp, "<image_object cmd=\"fill image\" ");
	fprintf(dumped_page->fp, "x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\" n=\"%d\" xres=\"%d\" yres=\"%d\"", 
		pixmap->x, pixmap->y, pixmap->w, pixmap->h, pixmap->n, pixmap->xres, pixmap->yres);
	if (pixmap->colorspace)
	{
		fprintf(dumped_page->fp, " ");
		fz_dump_colorspace_attr(dumped_page->fp, pixmap->colorspace);
	}
	if (local_ctm)
	{
		fprintf(dumped_page->fp, " ");
		fz_dump_ctm_attr(dumped_page->fp, local_ctm);
	}
	fprintf(dumped_page->fp, ">\r\n");

	if (image)
	{
		fprintf(dumped_page->fp, "<image ");
		fprintf(dumped_page->fp, "w=\"%d\" h=\"%d\" bpc=\"%d\" xres=\"%d\" yres=\"%d\"", 
			image->w, image->h, image->bpc, image->xres, image->yres);
		if (image->colorspace)
		{
			fprintf(dumped_page->fp, " ");
			fz_dump_colorspace_attr(dumped_page->fp, image->colorspace);
		}

		if (image->buffer)
		{
			fprintf(dumped_page->fp, " length=\"%d\"", image->buffer->buffer->len);
		}
		fprintf(dumped_page->fp, "/>\r\n");
	}
}

void 
fz_end_dump_fill_image(fz_context *ctx, fz_dumped_page *dumped_page)
{
	fprintf(dumped_page->fp, "</image_object>\r\n");
}

void 
fz_begin_dump_stroke_path(fz_context *ctx, fz_dumped_page *dumped_page, fz_path *path, 
	const fz_stroke_state *stroke, const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha)
{
	fprintf(dumped_page->fp, "<graphics_object cmd=\"stroke path\" ");
	fz_dump_ctm_attr(dumped_page->fp, ctm);
	fprintf(dumped_page->fp, ">\r\n");

	fprintf(dumped_page->fp, "<gstate>\r\n");
	fz_dump_colorspace(dumped_page->fp, colorspace, color, alpha);
	if (stroke)
	{
		fz_dump_stroke_state(dumped_page->fp, stroke);
	}
	fprintf(dumped_page->fp, "</gstate>\r\n");

	if (path)
	{
		fz_dump_path(dumped_page->fp, path);
	}
}

void 
fz_end_dump_stroke_path(fz_context *ctx, fz_dumped_page *dumped_page)
{
	fprintf(dumped_page->fp, "</graphics_object>\r\n");
}

void
fz_begin_dump_fill_path(fz_context *ctx, fz_dumped_page *dumped_page, fz_path *path, int even_odd, 
	const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha)
{
	fprintf(dumped_page->fp, "<graphics_object cmd=\"fill path\" ");
	fz_dump_ctm_attr(dumped_page->fp, ctm);
	fprintf(dumped_page->fp, ">\r\n");

	fprintf(dumped_page->fp, "<gstate>\r\n");
	fz_dump_colorspace(dumped_page->fp, colorspace, color, alpha);
	fprintf(dumped_page->fp, "<even_odd>%f</even_odd>\r\n", even_odd);
	fprintf(dumped_page->fp, "</gstate>\r\n");

	if (path)
	{
		fz_dump_path(dumped_page->fp, path);
	}
}

void fz_end_dump_fill_path(fz_context *ctx, fz_dumped_page *dumped_page)
{
	fprintf(dumped_page->fp, "</graphics_object>\r\n");
}